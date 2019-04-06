#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "historial.c"
#define PROMPT "usuario$"
#define SALIR "exit\0"
#define HISTORIAL "historial\0"
#define largoPrompt 8
#define STDIN 0
#define arriba 500
#define abajo 501
#define derecha 502
#define izquierda 503
#define backspace 127
#define suprimir 505
#define enter 10

typedef enum {false,true} bool;

struct comando{
	char* cadena;
	struct comando * siguiente;
	struct comando * anterior;
};
struct comando* primero;
struct comando* ultimo;
struct comando* actual;
struct comando* nuevo;

struct nuevos{
	char* cadenaN;
	struct nuevos * siguiente;
};
struct nuevos *primeroN;
struct nuevos *actualN;
struct nuevos *nuevoN;


int identificarCaso();
void agregarCaracter(char ca, int pos);
void borrarCaracter(int pos);
void actualizarListas(bool salir,bool esNuevo);
void validarObtenerComandoHistorial();
void darFormato(char* cadena, char** comando);
void procesarComando(char** comando,int segundoPlano);
bool leerArchivo(FILE *archivoC);
bool guardarArchivo(FILE *archivoC);
void liberarMemoria();

int main() {
	int pos, largoComando, segundoPlano = 0;
	bool cmndCompleto = false, salir = false;
	int guardaStdout = dup(1), guardaStdin = dup(0), caso, i = 0, j = 0;
	FILE* archivoC;
	primero = (struct comando*)malloc(sizeof(struct comando));
	//primeroN = (struct nuevos*)malloc(sizeof(struct comando));
	//primeroN->siguiente = NULL;
	//actualN = primeroN;
	primero->anterior = primero->siguiente = NULL;
	ultimo = nuevo = actual = primero;
	leerArchivo(archivoC);
	while(!salir){
		pos = largoComando = largoPrompt+1;
		dup(guardaStdin);
		printf(PROMPT);
		while(!cmndCompleto){
			caso = identificarCaso();
			switch (caso) {
				case enter:
						pos = largoComando;
						agregarCaracter('\0',(largoComando-largoPrompt-1));
						cmndCompleto = true;
						printf("\n");
						break;
				case arriba:
						if(actual->anterior != NULL){
							actual = actual->anterior;
							printf("\033[%dD\033[K%s%s",pos,PROMPT,actual->cadena);
							pos = largoComando = strlen(actual->cadena)+largoPrompt+1;
						}
						break;
				case abajo:
						if(actual->siguiente != NULL){
							actual = actual->siguiente;
							printf("\033[%dD\033[K%s",pos-1,PROMPT);
							pos = largoComando = largoPrompt+1;
							if(actual->cadena != NULL){
								printf("%s",actual->cadena);
								pos = largoComando += strlen(actual->cadena);
							}
						}
						break;
				case izquierda:
						if(pos > largoPrompt+1){
							printf("\033[1D");
							pos--;
						}
						break;
				case derecha:
						if(pos < largoComando){
							printf("\033[1C");
							pos++;
						}
						break;
				case suprimir:
						if(pos < largoComando){
							printf("\033[1P");
							borrarCaracter((pos-largoPrompt-1));
							largoComando--;
						}
						break;
				case backspace:
						if(pos>largoPrompt+1){
							printf("\b\033[1P");
							pos--;
							largoComando--;
							borrarCaracter((pos-largoPrompt-1));
						}
						break;
				default:
						printf("\033[1@%c",caso);
						char c = caso+0;
						agregarCaracter(c,(largoComando-largoPrompt-1)-(largoComando-pos));
						pos++;
						largoComando++;
			}
		}
		if(strlen(actual->cadena) > 0){
			agregarHistorial(actual->cadena);

			if((strcmp(actual->cadena,SALIR) == 0)){
				salir = true;
				actualizarListas(salir,true);
				guardarArchivo(archivoC);
			}else {
				cmndCompleto = false; //Se actualiza para un nuevo comando
				int i = 0,pos = 0;
				while(actual->cadena[i] != '\0'){
					if(actual->cadena[i] == '&' && i == strlen(actual->cadena)-1){
						pos = i;
						actual->cadena[pos] = '\0';
						segundoPlano = 1;
					}
					i++;
				}

				if(strcmp(actual->cadena,HISTORIAL) == 0){
					imprimirHistorial();
				}else if(actual->cadena[0] == '!'){
					validarObtenerComandoHistorial();
				}else{
					char *comando[strlen(actual->cadena)];
					darFormato(actual->cadena,comando);
					procesarComando(comando,segundoPlano);
				}
				actualizarListas(salir,true);
				actual->cadena = NULL;
			}
		} else{
			salir = false;
			cmndCompleto = false;
		}
	}
	liberarMemoria();
	printf("\n");
	return 0;
}

void agregarCaracter(char ca, int pos){
	if(actual->cadena == NULL){
		actual->cadena = (char*)realloc(actual->cadena,sizeof(char));
	}else{
		int limite = strlen(actual->cadena);
		actual->cadena = (char*)realloc(actual->cadena,(limite+1)*sizeof(char));
		while(limite >= pos)
			actual->cadena[limite--] = actual->cadena[limite-1];
	}
	actual->cadena[pos] = ca;
}

void borrarCaracter(int pos){
	int k = pos;
	if(actual->cadena != NULL){
		int limite = strlen(actual->cadena);
		while(k < limite){
			actual->cadena[k++] = actual->cadena[k+1];
		}
		actual->cadena = (char*)realloc(actual->cadena,(limite-1)*sizeof(char));
	}
}

void actualizarListas(bool salir,bool esNuevo){
	if(ultimo != actual){
		ultimo->cadena = (char*)realloc(ultimo->cadena, strlen(actual->cadena)*sizeof(char));
		strcpy(ultimo->cadena,actual->cadena);
		actual = ultimo;
	}
	if(esNuevo){
		nuevoN = (struct nuevos*) malloc (sizeof(struct nuevos));
		nuevoN->cadenaN = (char*)malloc(strlen(actual->cadena)*sizeof(char));
		strcpy(nuevoN->cadenaN, actual->cadena);
		nuevoN->siguiente = NULL;
		if(primeroN != NULL)
			actualN = actualN->siguiente = nuevoN;
		else{
			actualN = primeroN = nuevoN;
		}
	}

	if(!salir){
		nuevo = NULL;
		nuevo = (struct comando*)malloc(sizeof(struct comando));
		nuevo->siguiente = NULL;
		nuevo->anterior = ultimo;
		ultimo->siguiente = nuevo;
		actual = ultimo = nuevo;
	}
}

void validarObtenerComandoHistorial(){
	char *numero = NULL;
	int k = 0, num;
	while(actual->cadena[++k] != '\0'){
		numero = (char*)realloc(numero,k*sizeof(char));
		numero[k-1] = actual->cadena[k];
	}
	numero = (char*)realloc(numero,(k+1)*sizeof(char));
	numero[k] = '\0';
	if((num = atoi(numero)) != 0){
		if((num <= contComandos) && (num > (contComandos-10))){
			printf("%s\n",obtenerComando(num));
		}else{
			printf("Error\n");
		}
	}else{
		printf("Error\n");
	}
}

//El metodo divide la cadena en un formato aceptable para execvp()
void darFormato(char* cadena, char* comando[]){
	int j = 0;
	comando[j] = strtok( cadena, " ");    // Primera llamada -> Primer token
	strcat(comando[j++],"\0");
	while( (comando[j] = strtok( NULL, " ")) != NULL )  {  // Posteriores llamadas
		strcat(comando[j++],"\0");
	}
}

	//Dentro del metodo se procesa el comando por medio del proceso hijo.
void procesarComando(char** comando,int segundoP){
	pid_t pid;
	int estado = 0;
	pid = fork();
	if(pid < 0){ //Fallo la llamada a fork
		perror("Error en la llamada a fork().");
	} else if (pid == 0){ //Hijo
		execvp(comando[0], comando);
		perror("Error");
		exit(0);
	} else { //Padre
		if(segundoP == 0)
		pid = wait(&estado);
	}
}

int identificarCaso(){
	int digito = 0;
	struct termios parametro_anterior, parametro;
	tcgetattr( STDIN_FILENO, &parametro_anterior);
	parametro = parametro_anterior;
	parametro.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &parametro);
	digito = getchar();
	if(digito == 27){
		digito = getchar();
		if(digito == 91){
			digito = getchar();
			switch (digito){
				case 65: //Arriba
					digito = arriba;
					break;
				case 66: //Abajo
					digito = abajo;
					break;
				case 67: //Derecha
					digito = derecha;
					break;
				case 68: //Izquierda
					digito = izquierda;
					break;
				case 51:
					digito = getchar();
					if(digito == 126) //delete o suprimir
						digito = suprimir;
					break;
				break;
			}
		}
	}
	tcsetattr( STDIN_FILENO, TCSANOW, &parametro_anterior);
	fflush(stdin);
	return digito;
}

bool leerArchivo(FILE *archivoC){
	archivoC = fopen("./comandos.txt","a+");
	if(archivoC == NULL){
		return false;
	}else{
		char c;
		int i = 0;
		char *cadena = NULL;
		while((c = fgetc(archivoC)) != EOF){
			if(c != '\n'){
				i++;
				cadena = (char*)realloc(cadena,i*sizeof(char));
				cadena[i - 1] = c;
			}else if(c == '\n' && cadena != NULL){
				cadena = (char*)realloc(cadena,i+1*sizeof(char));
				cadena[i] = '\0';
				actual->cadena = (char*)realloc(actual->cadena,strlen(cadena)*sizeof(char));
				strcpy(actual->cadena,cadena);
				agregarHistorial(cadena);
				actualizarListas(false,false);
				cadena = NULL;
				i = 0;
			}
		}
	}
	fclose(archivoC);
	return true;
}

bool guardarArchivo(FILE *archivoC){
	archivoC = fopen("./comandos.txt","a");
	actualN = primeroN;
	if(archivoC == NULL){
		return false;
	}else{
		while(actualN != NULL){
			fputs(actualN->cadenaN,archivoC);
			fputc('\n',archivoC);
			actualN = actualN->siguiente;
		}
	}
	fclose(archivoC);
	return true;
}
void liberarMemoria(){
	
	for(int i=0;i<cant;i++){
		free(comandosRecientes[i]);
		comandosRecientes[i] = NULL;
	}
	
	actual = primero;
	while(actual != NULL){
		primero = primero->siguiente;
		if(actual->cadena != NULL){
			free(actual->cadena);
			actual->cadena = NULL;
		}
		free(actual);
		actual = primero;
	}
	ultimo = NULL;
	nuevo = NULL;
	actualN = primeroN;
	while(actualN != NULL){
		primeroN = primeroN->siguiente;
		if(actualN->cadenaN != NULL){
			free(actualN->cadenaN);
			actualN->cadenaN = NULL;
		}
		free(actualN);
		actualN = primeroN;
	}
	nuevoN = NULL;
}
