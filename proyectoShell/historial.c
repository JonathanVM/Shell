#include <stdlib.h>
#include <string.h>
#define MAXHISTORIAL 10

void agregarHistorial(char *comando);
int contComandos = 0, cant = 0;

char *comandosRecientes[MAXHISTORIAL];

void agregarHistorial(char *comando){
	int i = 0;
	++contComandos;
	if(cant < MAXHISTORIAL){
		comandosRecientes[cant] = (char*)realloc(comandosRecientes[cant],strlen(comando)*sizeof(char));
		strcpy(comandosRecientes[cant],comando);
		cant++;
	}
	else{
		for(i = 0; i < (MAXHISTORIAL-1); ++i){
			//Se descarta el primer comando de los ultimos 10.
			strcpy(comandosRecientes[i],comandosRecientes[i+1]);
		}
		//Se agrega el ultimo comando.
		comandosRecientes[cant] = (char*)realloc(comandosRecientes[cant],strlen(comando)*sizeof(char));
		strcpy(comandosRecientes[(MAXHISTORIAL-1)],comando);
	}
}

char* obtenerComando(int numComando){
	return comandosRecientes[(cant-1)-(contComandos-numComando)];
}

void imprimirHistorial(){
	int i,j;
	for(i = cant-1, j = 0; i >= 0 && j < 10; --i, j++){
		printf("%d. %s\n",(contComandos-j),comandosRecientes[i]);
	}
}