# Minishell en C - Intérprete de Comandos Linux

Este proyecto consiste en el desarrollo de un intérprete de comandos personalizado (**minishell**) programado en **Lenguaje C** para sistemas operativos basados en Unix/Linux. La shell es capaz de ejecutar comandos del sistema, gestionar flujos de entrada/salida y administrar procesos tanto en primer plano (*foreground*) como en segundo plano (*background*).

---

##  Características Principales

* **Ejecución de Mandatos:** Clonación de procesos mediante la llamada al sistema `fork()` y sustitución de la imagen de ejecución con `execvp()`.
* **Tuberías Dinámicas (Pipes):** Soporte para el encadenamiento de $N$ mandatos concurrentes mediante la creación dinámica de estructuras de comunicación (`pipe()`) y la redirección de descriptores de archivo con `dup2()`.
* **Redirecciones de I/O:** Desvío de la entrada estándar (`<`) y salida estándar (`>`) hacia o desde ficheros utilizando la llamada al sistema `open()`.
* **Gestión de Procesos en Background:** Soporte para la ejecución asíncrona de tareas mediante el operador `&`, manteniendo un registro dinámico de *jobs* activos.
* **Comandos Internos (Built-ins):** Implementación nativa de comandos esenciales para la navegación y el control del entorno:
  * `cd [ruta]`: Cambio de directorio de trabajo interactivo empleando `chdir()` y `getcwd()`.
  * `jobs`: Listado y monitorización de los procesos que se están ejecutando en segundo plano.
  * `fg [%id]`: Recuperación y sincronización de procesos desde el *background* al *foreground* mediante `waitpid()`.
* **Manejo de Señales:** Control del comportamiento de la terminal ante interrupciones del teclado, configurando el sistema para ignorar `SIGQUIT` (Ctrl+\\) y capturando `SIGINT` (Ctrl+C) de forma limpia para evitar el cierre inesperado de la shell principal.

---

##  Tecnologías y Conceptos de Sistemas Aplicados

* **Lenguaje:** C (Estándar POSIX)
* **Gestión de Procesos:** `fork`, `execvp`, `waitpid`, `WNOHANG`.
* **Comunicación Interna y Flujos:** Descriptores de archivo, `pipe`, `dup2`, `O_RDONLY`, `O_WRONLY`, `O_CREAT`, `O_TRUNC`.
* **Manejo de Señales del SO:** `signal` (`SIGINT`, `SIGQUIT`, `SIG_DFL`, `SIG_IGN`).
* **Administración de Memoria:** Reserva dinámica mediante `malloc()` y liberación con `free()` para el control riguroso de las estructuras de comandos y tuberías.

---

##  Instalación y Uso

### 1. Clonar el repositorio

### 2. Compilación

gcc -Wall -Wextra test.c libparser.a –o test -static

### 3. Ejecución
Inicia tu nuevo intérprete de comandos:
./msh



