#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h"
#include <string.h>
#include <fcntl.h>   
#include <errno.h>
#include <signal.h>

#define MAX_JOBS 64

typedef struct {
    pid_t pid;        // pid del proceso
    int njob;         // número de job
    char cmd[1024];   // línea de comando ejecutada
} Job;

Job jobs[MAX_JOBS];  // array de jobs en background
int njobs = 0;       // número de jobs actuales

/* Manejador de señal para SIGINT: muestra el prompt de nuevo */
void manejador_senal(int sig) {
    (void)sig; // evitamos warning de parámetro no usado
    printf("\nmsh> ");
    fflush(stdout);
}

/* Maneja la redirección de entrada estándar desde un fichero.
   Redirige el descriptor 0 (stdin) al fichero indicado en redirect_input */
void manejar_entrada(tline *line){
    if(line->redirect_input != NULL){  
        int fde = open(line->redirect_input, O_RDONLY); //Abrimos el fichero para solo lectura
        if(fde == -1){
            fprintf(stderr, "%s: Error. %s\n", line->redirect_input, strerror(errno));
            exit(1);
        }
        dup2(fde, 0); //Redirigimos stdin al fichero
        close(fde);   //Cerramos el descriptor, stdin ya apunta al fichero
    }
}

/* Maneja la redirección de salida estándar a un fichero.
   Redirige el descriptor 1 (stdout) al fichero indicado en redirect_output */
void manejar_salida(tline *line){
    if(line->redirect_output != NULL) { 
        int fds = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644); //Abrimos el fichero para poder escribir dentro
        if(fds == -1) {
            fprintf(stderr, "%s: Error. %s\n", line->redirect_output, strerror(errno));
            exit(1);
        }
        dup2(fds, 1); //Redirigimos stdout al fichero
        close(fds);   //Cerramos el descriptor, stdout ya apunta al fichero
    }
}

/* Elimina un job del array de jobs desplazando los siguientes */
void eliminar_job(int idx){
    int i;
    for(i = idx; i < njobs - 1; i++) {
        jobs[i] = jobs[i+1];
    }
    njobs--;
}

/* Comprueba si algún proceso en background ha terminado y lo elimina del array */
void actualizar_jobs(){
    int i;
    int status;
    for(i = 0; i < njobs; i++) {
        if(waitpid(jobs[i].pid, &status, WNOHANG) > 0) {
            // el proceso ha terminado, lo eliminamos
            eliminar_job(i);
            i--; // ajustamos el índice tras eliminar
        }
    }
}

int main() {
    char buf[1024];  //Buffer para almacenar la línea introducida por el usuario
    tline *line;     //Estructura con la información de los mandatos
    int i;

    /* El minishell ignora SIGQUIT y maneja SIGINT para mostrar el prompt */
    signal(SIGINT, manejador_senal);
    signal(SIGQUIT, SIG_IGN);

    while(1) {
        /* Comprobamos si algún proceso en background ha terminado */
        actualizar_jobs();

        printf("msh> "); //Muestra el prompt
        fflush(stdout);  //Fuerza la impresión del prompt antes de leer

        if(fgets(buf, 1024, stdin) == NULL) break; //Si se pulsa Ctrl+D salimos del bucle

        line = tokenize(buf); //Divide el string en distintos mandatos y argumentos

        if(line->ncommands == 0) continue; //Si no hay mandatos volvemos a mostrar el prompt

        if(strcmp(line->commands[0].argv[0], "cd") == 0) { //Comprobación si el mandato es cd
            char *dir;
            char ruta[1024];
            if(line->commands[0].argc == 1) {
                dir = getenv("HOME"); // sin argumentos, vamos a HOME
            } else {
                dir = line->commands[0].argv[1]; // con argumento, vamos a esa ruta
            }
            if(chdir(dir) == -1) {
                fprintf(stderr, "%s: Error. %s\n", dir, strerror(errno));
            } else {
                getcwd(ruta, sizeof(ruta)); // obtenemos la ruta actual
                printf("%s\n", ruta);       // la mostramos
            }
            continue;
        }

        if(strcmp(line->commands[0].argv[0], "jobs") == 0) { //Comprobación si el mandato es jobs
            for(i = 0; i < njobs; i++) {
                printf("[%d]+ Running %s", jobs[i].njob, jobs[i].cmd); //Mostramos cada job
            }
            continue;
        }

        if(strcmp(line->commands[0].argv[0], "fg") == 0) { //Comprobación si el mandato es fg
            int njob;
            int status;
            int idx;

            if(njobs == 0) { //Si no hay jobs en background
                fprintf(stderr, "fg: No hay trabajos en background\n");
                continue;
            }

            if(line->commands[0].argc == 1) {
                /* Sin argumentos, traemos el último job */
                idx = njobs - 1;
            } else {
                /* Con argumento %n, buscamos el job con ese número */
                njob = atoi(line->commands[0].argv[1] + 1); // saltamos el '%'
                idx = -1;
                for(i = 0; i < njobs; i++) {
                    if(jobs[i].njob == njob) {
                        idx = i;
                        break;
                    }
                }
                if(idx == -1) {
                    fprintf(stderr, "fg: Job no encontrado\n");
                    continue;
                }
            }

            /* Mostramos el comando que se trae a foreground */
            printf("%s", jobs[idx].cmd);

            /* Esperamos a que termine el proceso */
            waitpid(jobs[idx].pid, &status, 0);

            /* Eliminamos el job del array */
            eliminar_job(idx);
            continue;
        }

        if(line->ncommands == 1){ //Un solo mandato, no necesita pipes

            pid_t pid = fork(); //Creamos un proceso hijo
            if(pid == 0) {
                /* Los procesos en foreground responden a las señales */
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);

                manejar_entrada(line); //Redirige stdin si hay redirección de entrada
                manejar_salida(line);  //Redirige stdout si hay redirección de salida

                if(line->commands[0].filename == NULL) { //Si el mandato no existe
                    fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
                    exit(1);
                }
                execvp(line->commands[0].filename, line->commands[0].argv); //Ejecutamos el mandato
                
            } else {
                if(line->background == 1) {
                    jobs[njobs].pid = pid;
                    jobs[njobs].njob = njobs + 1;
                    strncpy(jobs[njobs].cmd, buf, 1024);
                    njobs++;
                    printf("[%d] %d\n", njobs, pid); //Mostramos el pid y no esperamos
                } else {
                    int status;
                    waitpid(pid, &status, 0); //El padre espera a que el hijo termine
                }
            }

        } else {
            /* Creamos N-1 pipes dinámicamente para N comandos */
            int j;
            pid_t pids[line->ncommands];
            int **pipes = malloc((line->ncommands - 1) * sizeof(int *));
            for(i = 0; i < line->ncommands - 1; i++) {
                pipes[i] = malloc(2 * sizeof(int));
                pipe(pipes[i]);
            }

            /* Creamos un hijo por cada comando */
            for(i = 0; i < line->ncommands; i++) {
                pids[i] = fork();
                if(pids[i] == 0) {
                    /* Los procesos en foreground responden a las señales */
                    if(line->background == 0) {
                        signal(SIGINT, SIG_DFL);
                        signal(SIGQUIT, SIG_DFL);
                    }

                    /* Si no es el primero, lee del pipe anterior */
                    if(i > 0) {
                        dup2(pipes[i-1][0], 0);
                    }
                    /* Si no es el último, escribe en el pipe siguiente */
                    if(i < line->ncommands - 1) {
                        dup2(pipes[i][1], 1);
                    }
                    /* Cerramos todos los pipes en el hijo */
                    for(j = 0; j < line->ncommands - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    /* Redirecciones solo en primero y último */
                    if(i == 0) manejar_entrada(line);
                    if(i == line->ncommands - 1) manejar_salida(line);

                    /* Comprobamos si el mandato existe */
                    if(line->commands[i].filename == NULL) {
                        fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
                        exit(1);
                    }
                    execvp(line->commands[i].filename, line->commands[i].argv);
                }
            }

            /* Padre: cierra todos los pipes */
            for(i = 0; i < line->ncommands - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            /* Padre: espera o no según background */
            if(line->background == 1) {
                jobs[njobs].pid = pids[line->ncommands-1];
                jobs[njobs].njob = njobs + 1;
                strncpy(jobs[njobs].cmd, buf, 1024);
                njobs++;
                printf("[%d] %d\n", njobs, pids[line->ncommands-1]);
            } else {
                int status;
                for(i = 0; i < line->ncommands; i++) {
                    waitpid(pids[i], &status, 0); //Esperamos a todos los hijos
                }
            }

            /* Liberamos la memoria dinámica */
            for(i = 0; i < line->ncommands - 1; i++) {
                free(pipes[i]);
            }
            free(pipes);
        }
    }
    return 0;
}