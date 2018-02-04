#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <paths.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
//#include "tlpi_hdr.h"
#include <unistd.h>
#include <utmpx.h>

using namespace std;

#define DAEMON_NAME "proceso_s_o"
#define _GNU_SOURCE 

int pid_global = 0; // PID Proceso
int num_usuarios = 3; // Despues de 3 usuarios saca a los demas
int valor_inicial = 100;
int variable_aleatoria = valor_inicial; // Variable inicial 100, valor maximo 200
int valor_maximo = 200;
string usuario_actual = "root"; //Debe ser root para sacar los demas usuarios

//Defino las funciones de cada hilo
void *thread1_function( void *ptr );
void *thread2_function( void *ptr );
void *thread3_function( void *ptr );

/*int usuarios_sistema(){
    // Estructura para cada usuario en el archivo /etc/passwd
    struct passwd* user_info;
    int conteo_usuarios = 0;
    //printf("List of user names:\n");
    for (user_info = getpwent(); user_info; user_info = getpwent() ) {
        //printf("  %s\n", user_info->pw_name);
        //printf("  %i\n", user_info->pw_gid);
        conteo_usuarios = conteo_usuarios +  1;
    }

    //Cierro la conección, sinLo despues de la segunda vez siempre me traera 0 usuarios
    endpwent();
    
    return conteo_usuarios;
}
*/

void process(){
    //syslog(LOG_NOTICE, "Escribo en el Log del Sistema");
    // Instancio los Hilos
    pthread_t thread1, thread2, thread3;

    int  iret1, iret2, iret3;

    // Mensajes del proceso (No aplica)
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";
    char *message3 = "Thread 3";

    //Creo los hilos
    iret1 = pthread_create( &thread1, NULL, thread1_function, (void*) message1);
    iret2 = pthread_create( &thread2, NULL, thread2_function, (void*) message2);
    iret3 = pthread_create( &thread3, NULL, thread3_function, (void*) message3);

    //Los Agrego al proceso o programa actual
    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL);
    pthread_join( thread3, NULL);

}

//Implementacion hilo 1//monitoreo de proceso
void *thread1_function( void *ptr )
{
    //char *message;
    //message = (char *) ptr;
    //syslog(LOG_NOTICE, message);

    while(true){
        int tamano = 0, residente = 0, compartida = 0;
        // /proc/ tiene una carpeta con el process id, ejm (/proc/1002), si le digo que self,
        // el ya sabe que por defecto se refiere a el mismo
        ifstream buffer("/proc/self/statm");
            //self se refiere al numero del proceso que lo llamo.
        // leo los datos
        buffer >> tamano >> residente >> compartida;
        buffer.close();
        cout << "\n\n:..................HILO 1....................:\n\n";

        // Calculo tamaño de pagina
        long tam_pag = sysconf(_SC_PAGE_SIZE) / 2048; // 1024 para X86 y 2048 para X86-X64
        double rss = residente * tam_pag;
        cout << "Tamaño Conjunto Residente - " << rss << " KB\n";

        double memoria_compartida = compartida * tam_pag;
        cout << "Memoria Compartida - " << memoria_compartida << " KB\n";

        cout << "Memoria Privada - " << rss - memoria_compartida << " KB\n";

        sleep(2); // Duermo durante 2 Segundos
    }
    return 0;

}

/*
//Implementacion hilo 2
void *thread2_function( void *ptr )
{
    //char *message;
    //message = (char *) ptr;
    //syslog(LOG_NOTICE, message);
    while(true){
        int conteo_usuarios = usuarios_sistema();
        cout << "\n\n:..................HILO 2....................:\n\n";

        if(conteo_usuarios > num_usuarios){
            cout << "Se deben eliminar usuarios, solo pueden haber " << num_usuarios << " y el sistema cuenta " << conteo_usuarios;
        }
        else{
            cout << "Número correcto de usuarios en el sistema. Usuarios =" << conteo_usuarios << ", Cantidad Máxima  " << num_usuarios;
        }

        sleep(2); // Duermo durante 2 Segundos
    }
}
*/

void *thread2_function( void *ptr )
{
    //char *message;
    //message = (char *) ptr;
    //syslog(LOG_NOTICE, message);
    
    while(true){
        cout << "\n\n:..................HILO 2....................:\n\n";

        // Estructura (Clase) donde se almacenan los usuarios, ojo es un puntero
        struct utmpx *ut;
        // Abro el archivo utmp (registra usuarios actualmente logueados)
        setutxent();
        printf("Usuario   \t Tipo   \t PID   \t id   \t fecha/hora   \n\n");
        int usuarios_actuales = 0;
        bool actual = false;

        // Obtengo datos del archio utmp
        while ((ut = getutxent()) != NULL) { /* Sequential scan to EOF */
            // Existen varios tipos de usuarios, solo necesitamos los de tipo USER_PROCESS
            if(ut->ut_type != USER_PROCESS || (actual && usuario_actual == ut->ut_user)){
                continue;
            }

            //Imprimo datos usuarios logueados, solo muestro una vez cada uno,
            //Para eso es la variable actual y usuario_actual
            printf("%-8s \t", ut->ut_user);
            printf("%-9.9s \t",
            (ut->ut_type == EMPTY) ? "EMPTY" :
            (ut->ut_type == USER_PROCESS) ? "USER_PR" : "???");
            printf("%5ld\t %-3.5s\t", (long) ut->ut_pid, ut->ut_id);
            printf("%s\n", ctime((time_t *) &(ut->ut_tv.tv_sec)));
            usuarios_actuales = usuarios_actuales + 1;
            actual = true;
        }

        //Cierro el archivo utmp, para leerlo si aplica la eliminacion
        endutxent();

        cout << "\nUsuarios Actuales : " << usuarios_actuales << "\n";


        //Si hay mas usuarios de los que se pueden
        if(usuarios_actuales > num_usuarios){
            int tmp_cont = 0;
            bool actual = false;
            /* Sequential scan to EOF */
            // Leo nuevamente el archivo, recordad que este se lee de menor fecha / hora entrada a mayor
            while ((ut = getutxent()) != NULL) {
                if(ut->ut_type != USER_PROCESS || (actual && usuario_actual == ut->ut_user)){
                    continue;
                }
                else{
                    if(tmp_cont >= num_usuarios){
                        kill(ut->ut_pid, SIGTERM); // Mato los procesos referentes al ultimo usuario logueado
                        // Cuando un usuario ingresa a linux, se crea un proceso asociado a este
                        cout << "\n-- Matando proceso : " << ut->ut_pid << ", Con usuario : " << ut->ut_user << "\n";
                    }
                   tmp_cont = tmp_cont + 1;
                }
                actual = true;
            }
            // Cierro nuevamente el archivo
            endutxent();
        }

        sleep(2); // Duermo durante 2 Segundos
    }
}

//Implementacion hilo 3
void *thread3_function( void *ptr )
{
    //char *message;
    //message = (char *) ptr;
    //syslog(LOG_NOTICE, message);
    while(true){
        cout << "\n\n:..................HILO 3....................:\n\n";
        // Calculo el valor de la variable hasta valor_maximo
        variable_aleatoria = rand() % (valor_maximo + 1);
        cout << "Nuevo Valor de la variable: " << variable_aleatoria << "\n";

        // Valido y reasigno si aplica
        if(variable_aleatoria > valor_inicial){
            cout << "Se supero el valor inicial (" << valor_inicial << "), reasignando variable\n";
            variable_aleatoria = valor_inicial;
        }
        sleep(2); // Duermo durante 2 Segundos
    }
}

int main(int argc, char *argv[]) {

    //Mascara de login y abro el log
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

    syslog(LOG_NOTICE, "Entrando al Proceso");
//definicion de variable del tipo pid_t
    pid_t pid, sid;
// los pid_t tipos de datos que representan los id de los procesos.

   //Fork al proceso padre
    pid = fork();


    // Muestro proceso actual y evito que muestre pid = 0 del padre al matarlo
    if((int)pid != 0){
        cout << "PID = " << pid << " \n";
        pid_global = pid;
    }

    if (pid < 0) { exit(EXIT_FAILURE); }

    //Si poseemos un proceso hijo, cerramos el proceso padre
    if (pid > 0) { exit(EXIT_SUCCESS); }

    //Cambio la mascara de archivos
    umask(0);
// el programa tiene permisos de ejcucion otorgador por el programa y el usuario 

    //Crea una firma para el proceso hijo
    sid = setsid();
    if (sid < 0) { exit(EXIT_FAILURE); }

    //Cambio el directorio
    
    //Si no encuentra la ruta, abandona.
    
    if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

    //Cierro los descriptores de archivos (No permite mostrar las impresiones en consola)
    
    //close(STDIN_FILENO);
    //close(STDOUT_FILENO);
    //close(STDERR_FILENO);

    //----------------
    //Main Process
    //----------------
    while(true){
        process();
        //sleep(5); // Duermo durante 5 Segundos
    }

    // Cierro el syslog
    closelog ();
}