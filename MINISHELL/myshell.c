#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include "parser.h"

tline *line;

void redirect(); // función para la redirección
void cd (); // función para el mandato cd

int main(void)
{
	char buf[1024]; // almacenamos los input en un buffer
	int pid;
	printf("msh >> ");

	// ignoramos las señales SIGINT y SIGQUIT para la minishell 
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	while(fgets(buf,1024,stdin)){ // almacenamos la entrada por teclado en buffer

		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);

		line = tokenize(buf); // tokenizamos lo que hay en el buffer y lo guardamos en line
		 if (line->ncommands == 1){ //solo un mandato

		 	//los procesos en primer plano no ignoran las señales SIGINT y SIGQUIT
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);

			if (strcmp(line->commands[0].argv[0],"cd")==0){ // si el mandato es cd
				cd();
			}else{ // ejecutamos el mandato correspondiente
				pid = fork();
				if (pid < 0){ // error en el fork
					fprintf(stderr, "Error en el fork %s\n", strerror(errno));
				}else if (pid == 0){ // proceso hijo
					redirect(); // comprobamos si hay redirecciones
					execvp(line->commands[0].argv[0],line->commands[0].argv); // ejecutamos mandato
					fprintf(stderr, "%s No se encuentra el mandato\n", line->commands[0].argv[0]); // imprir error en caso de no exec 
					exit(1);									
				}else{ // proceso padre
					wait(NULL);
				}	
			}
			
		}else if (line->ncommands == 2){// 2 mandatos 1 pipe
			int pipe_des[2];
			pipe(pipe_des);

			for (int i = 0; i < 2; ++i){

				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);

				pid = fork();
				if (i == 0){ // primer mandato
					if (pid < 0){
						fprintf(stderr, "Error en el fork %s\n", strerror(errno));
					}else if (pid == 0){
						close(pipe_des[0]); // cerramos la parte del pipe que no se usa
						redirect();
						dup2(pipe_des[1],fileno(stdout)); // escritura del pipe es la salida estandar
						close(pipe_des[1]);
						execvp(line->commands[i].filename, line->commands[i].argv);
						fprintf(stderr, "%s No se encuentra el mandato\n", line->commands[0].argv[0]);
						exit(1);
					}else{
						close(pipe_des[1]);
						wait(NULL);
					}
				}else if (i == 1){ // segundo mandato
					if (pid < 0){
						fprintf(stderr, "Error en el fork %s\n", strerror(errno));
					}else if (pid == 0){
						close(pipe_des[1]);
						redirect();
						dup2(pipe_des[0],fileno(stdin));
						close(pipe_des[0]);
						execvp(line->commands[i].filename, line->commands[i].argv);
						fprintf(stderr, "%s No se encuentra el mandato\n", line->commands[0].argv[0]);
						exit(2);
					}else{
						close(pipe_des[0]);
						wait(NULL);
					}
				}
			}
		}else{ // varios mandatos con varios pipes
			int npipes = line->ncommands - 1; // numero de pipes

			int **pipes = (int**) malloc(npipes*sizeof(int*)); // reservo en mem. dinamica una matriz con los pipes
			for (int i = 0; i < npipes; ++i){
				pipes[i] = (int *) malloc(2 * sizeof(int));
				pipe(pipes[i]);
			}

			for (int i = 0; i < line->ncommands; ++i){

				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);

				pid = fork();

				if (pid < 0){
					fprintf(stderr, "Error en el fork %s\n", strerror(errno));
				}else if (pid == 0){ // proceso hijo
					if (i == 0){ // primer mandato
						close(pipes[i][0]);
						redirect();
						dup2(pipes[i][1],fileno(stdout));
						close(pipes[i][1]);
						execvp(line->commands[i].filename, line->commands[i].argv);
						fprintf(stderr, "%s No se encuentra el mandato\n", line->commands[0].argv[0]);
						exit(1);
					}else if (i == line->ncommands - 1){// ultimo mandato
						close(pipes[i-1][1]);
						redirect();
						dup2(pipes[i-1][0],fileno(stdin));
						close(pipes[i-1][0]);
						execvp(line->commands[i].filename, line->commands[i].argv);
						fprintf(stderr, "%s No se encuentra el mandato\n", line->commands[0].argv[0]);
						exit(1);
					}else{ // resto de mandatos
						close(pipes[i-1][1]);
						close(pipes[i][0]);
						dup2(pipes[i-1][0],fileno(stdin));
						dup2(pipes[i][1],fileno(stdout));
						close(pipes[i-1][0]);
						close(pipes[i][1]);
						execvp(line->commands[i].filename, line->commands[i].argv);
						fprintf(stderr, "%s No se encuentra el mandato\n", line->commands[0].argv[0]);
						exit(1);
					}
				}else{ // proceso padre
					if(!(i==(line->ncommands-1))){ 
							close(pipes[i][1]);
					}
					wait(NULL);
				}
			}

			// liberamos mem. dinamica
			for (int i = 0; i < npipes; ++i){
				free(pipes[i]);
			}
			free(pipes);
		}
		printf("msh >> ");
	}
	return 0;
}

void redirect(){
	if (line->redirect_input != NULL){ // si hay redireccion de entrada
		int entrada = open(line->redirect_input, O_RDONLY); // abrimos el fichero de redireccion
		if (entrada == -1){
			fprintf(stderr, "%s: Error %s\n", line->redirect_input, strerror(errno));
			exit(1);
		}else{
			dup2(entrada,fileno(stdin));
		}
	}

	if (line->redirect_output != NULL){ // si hay redireccion de salida
		int salida = creat(line->redirect_output, S_IWUSR | S_IRUSR); // creamos o sobreescribimos el fichero de redireccion
		if (salida == -1){
			fprintf(stderr, "%s: Error %s\n", line->redirect_output, strerror(errno));
			exit(1);
		}else{
			dup2(salida,fileno(stdout));
		}	
	}

	if (line->redirect_error != NULL){ // si hay redireccion de error
		int error = creat(line->redirect_error, S_IWUSR | S_IRUSR);
		if (error == -1){
			fprintf(stderr, "%s: Error %s\n", line->redirect_error, strerror(errno));
			exit(1);
		}else{
			dup2(error,fileno(stderr));
		}
	}
}

void cd (){
	
	char *dir; // variable de directorios 
	char buffer[512];
	
	if(line->commands[0].argc > 2)  {
		fprintf(stderr,"Uso: %s directorio\n", line->commands[0].argv[0]);
	}
	if (line->commands[0].argc == 1) { // si ejecutamos solo cd
		dir = getenv("HOME");
		if(dir == NULL){
			fprintf(stderr,"No existe la variable $HOME\n");	
		}
	}else { // si ejecutamos cd con un directorio
		dir = line->commands[0].argv[1];
	}
	
	if (chdir(dir) != 0) { // comprobamos si es un directorio.
		fprintf(stderr,"Error al cambiar de directorio: %s\n", strerror(errno)); 
	}
	printf( "El directorio actual es: %s\n", getcwd(buffer,-1));
}