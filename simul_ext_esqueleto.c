#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
             EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           char *nombre,  FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main()
{
	char *comando[LONGITUD_COMANDO];
	char *orden[LONGITUD_COMANDO];
	char *argumento1[LONGITUD_COMANDO];
	char *argumento2[LONGITUD_COMANDO];
	 
	int i,j;
	unsigned long int m;
	int comandoDesconocido;
    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps[MAX_INODOS];
    EXT_BLQ_INODOS ext_blq_inodos[MAX_NUMS_BLOQUE_INODO*SIZE_BLOQUE];
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
    int entradadir;
    int grabardatos;
    FILE *fent;
	 
    // Lectura del fichero completo de una sola vez  
    fent = fopen("particion.bin","r+b");
    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);        
     
    memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
	 
    // Buce de tratamiento de comandos
    for (;;){
		do{
		printf ("\n>> ");
		fflush(stdin);
		fgets(comando, LONGITUD_COMANDO, stdin);
		}while (ComprobarComando(comando, orden, argumento1, argumento2) != 0);
		 
		comandoDesconocido = 0;		 
	    if (strcmp(orden,"dir")==0){
            Directorio(directorio, ext_blq_inodos);
			comandoDesconocido = 1;
            continue;
		}
		 
		else if(strcmp(orden,"info")==0){
			LeeSuperBloque(&ext_superblock);
			comandoDesconocido = 1;
			continue;
		}
		 
        else if(strcmp(orden,"bytemaps")==0){
			Printbytemaps(&ext_bytemaps);
			comandoDesconocido = 1;
			continue;
		}
		 
		else if(strcmp(orden,"rename")==0){
			Renombrar(directorio, ext_blq_inodos, argumento1, argumento2);
			grabardatos = 1;
			comandoDesconocido = 1;
			continue;
		}
		 
		else if(strcmp(orden,"imprimir")==0){
			Imprimir(directorio, ext_blq_inodos, memdatos, argumento1);
			comandoDesconocido = 1;
			continue;
		}
		 
		else if(strcmp(orden,"remove")==0){
			Borrar(directorio, ext_blq_inodos,ext_bytemaps, &ext_superblock, argumento1, fent);
			grabardatos = 1;
			comandoDesconocido = 1;
			continue;
		}
		 
		else if(strcmp(orden,"copy")==0){
			Copiar(directorio, ext_blq_inodos,ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2,  fent);
			grabardatos = 1;
			comandoDesconocido = 1;
			continue;
		}
		 
		// Escritura de metadatos en comandos rename, remove, copy  
        if (grabardatos){
			fclose(fent);
			fent = fopen("particion.bin","w+b");
			
			GrabarSuperBloque(&ext_superblock,fent);
			GrabarByteMaps(&ext_bytemaps,fent);
			Grabarinodosydirectorio(&directorio,&ext_blq_inodos,fent);	
			GrabarDatos(&memdatos,fent);
			
			grabardatos = 0;
		}
		 
		if (strcmp(orden,"salir")==0){
           fclose(fent);
           return 0;
        }
		 
		if(comandoDesconocido == 0){
			printf("ERROR: Comando ilegal [info, bytemaps, dir, rename, imprimir, remove, copy, salir]\n");
		}
    }
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2){
	char* token;
	
	//En el caso de que el usuario realice una entrada vacia, esta no se tiene en cuenta
	if(strcmp(strcomando, "\n") == 0){
		return -1;
	}
	
	//Inicializamos todos los argumentos para que esten vacios
	memcpy(orden, "\0", LONGITUD_COMANDO);
	memcpy(argumento1, "\0", LONGITUD_COMANDO); 
	memcpy(argumento2, "\0", LONGITUD_COMANDO); 
	
	//Quitamos el salto de linea que deja fgets para que no nos de problemas posteriormente
	token = strtok(strcomando, "\n");
	memcpy(strcomando, token, LONGITUD_COMANDO); 
	
	//Extremos la orden con strtok
	token = strtok(strcomando, " ");
	memcpy(orden, token, LONGITUD_COMANDO); 
	
	//En el caso de existir, realizamos lo mismo para el argumento1 y argumento2
	token = strtok(NULL, " ");
	if(token != NULL){
		memcpy(argumento1, token, LONGITUD_COMANDO); 
		token = strtok(NULL, " ");
		if(token != NULL){
			memcpy(argumento2, token, LONGITUD_COMANDO); 
		}
	}

	return 0;
}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock){
	printf("Bloque: %d bytes\nInodosParticion: %d\nInodos libres: %d\nBloques Particion: %d\nBloques libres: %d\nPrimer bloque de datos: %d\n",SIZE_BLOQUE,ext_superblock->s_inodes_count, ext_superblock->s_free_inodes_count, ext_superblock->s_blocks_count, ext_superblock->s_free_blocks_count, ext_superblock->s_first_data_block);
}
	
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps){
	int i = 0;
	printf("Inodos : ");
	for(i=0; i < MAX_INODOS;i++){
		printf("%d ", ext_bytemaps->bmap_inodos[i]);
	}
	printf("\nBloques [0-25] :");
	for(i=0; i < 25;i++){
		printf("%d ", ext_bytemaps->bmap_bloques[i]);
	}
	printf("\n");
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos){
	int i = 0;
	int j = 0;
	int a = 0;
	int nada = 0xffff;
	for(i=1; i < MAX_FICHEROS; i++){
		if(directorio[i].dir_inodo!= nada){
			printf("%s\t",directorio[i].dir_nfich);
			printf("Tamaño: %d\t", inodos->blq_inodos[directorio[i].dir_inodo].size_fichero);
			printf("Inodo: %d\t", directorio[i].dir_inodo);
			printf("Bloques: ");
			for(j =0; j < MAX_NUMS_BLOQUE_INODO && inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j] != nada; j++){
					printf("%d ",inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j]);
			}
			printf("\n");
		}

	}
	
}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo){
	int i,j;//iteradores
	int error = 1;
	int iFichero = 0;
	//Comprobar que el fichero introducido existe
	for(i = 1; i < MAX_FICHEROS; i++){
		if(strcmp(directorio[i].dir_nfich,nombreantiguo)==0){
			error = 0; 
			iFichero = i;
		}
	}
	for(j =0; j < MAX_FICHEROS; j++){
		if(strcmp(directorio[j].dir_nfich,nombrenuevo)==0){
			error = 2; 
		}
	}
	if(error == 1){
		printf("ERROR: Fichero %s no encontrado\n", nombreantiguo);
		return -1;
	}
	if(error == 2){
		printf("ERROR: Ya existe un fichero con ese nombre\n",nombrenuevo);
		return -1;
	}
	strcpy(&directorio[iFichero].dir_nfich, nombrenuevo);
	return 0;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre){
	//Variables
	int i,j;//iteradores
	int error = 1;
	int iFichero; //indice del fichero en el directorio
	EXT_DATOS texto[7];
	
	//Comprobar que el fichero introducido existe
	for(i = 1; i < MAX_FICHEROS; i++){
		if(strcmp(directorio[i].dir_nfich,nombre)==0){
			error = 0; 
			iFichero=i;
		}
	}
	
	//Mostras mensaje de error si el fichero no exite
	if(error==1){
		printf("ERROR: Fichero %s no encontrado\n", nombre);
		return -1;
	}
	
	//Copiamos en la variable texto los datos de los bloques que ocupa el fichero en memdatos
	for(i = 0; i < MAX_NUMS_BLOQUE_INODO && inodos->blq_inodos[directorio[iFichero].dir_inodo].i_nbloque[i] != 0xffff; i++){
		memcpy(&texto[i], &memdatos[inodos->blq_inodos[directorio[iFichero].dir_inodo].i_nbloque[i] - PRIM_BLOQUE_DATOS], SIZE_BLOQUE);
	}
	
	//Colocamos un \0 al final del texto
	memcpy(&texto[i], "\0", 1);
	
	//Imprimimos el texto por pantalla
	puts(texto);
	return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre,  FILE *fich){
	int i;//iteradores
	int error = 1;
	int iFichero; //indice del fichero en el directorio
	//Comprobar que el fichero introducido existe
	for(i = 1; directorio[i].dir_inodo!= 0xffff; i++){
		if(strcmp(directorio[i].dir_nfich,nombre)==0){
			error = 0; 
			iFichero=i;
		}
	}
	if(error == 1){
		printf("ERROR: Fichero %s no encontrado\n", nombre);
		return -1;
	}
	directorio[iFichero].dir_inodo = 0xffff;
	strcpy(&directorio[iFichero].dir_nfich, "");
	inodos->blq_inodos[directorio[iFichero].dir_inodo].size_fichero = 0;
	for(i = 0; i < MAX_NUMS_BLOQUE_INODO && inodos->blq_inodos[directorio[iFichero].dir_inodo].i_nbloque[i] != 0xffff; i++){
		inodos->blq_inodos[directorio[iFichero].dir_inodo].i_nbloque[i] = 0xffff;	
	}
	return 0;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich){
	//Variables
	int i,j;//iteradores
	int error = 1;
	int iFicheroOrigen, iFicheroDestino; //indice de los ficheros en el directorio
	
	//Comprobar que el fichero origen existe
	for(i = 1; i < MAX_FICHEROS; i++){
		if(strcmp(directorio[i].dir_nfich,nombreorigen)==0){
			error = 0; 
			iFicheroOrigen=i;
		}
	}
	if(error==1){
		printf("El nombre del fichero origen no existe\n");
		return -1;
	}
	//Comprobar que el destino no existe
	for(i = 1; i < MAX_FICHEROS; i++){
		if(strcmp(directorio[i].dir_nfich,nombredestino)==0){
			printf("El nombre del fichero destino ya existe\n");
			return -1;
		}
	} 
	iFicheroDestino = i;//Si no ha habido errores i sera igual al numero total de ficheros en el directorio

	strcpy(directorio[iFicheroDestino].dir_nfich, nombredestino);
		
	//Asignarle el primer inodo libre
	for(i=0; i < MAX_INODOS; i++){
		if(ext_bytemaps->bmap_inodos[i]==0){
			directorio[iFicheroDestino].dir_inodo = i; //Actualizamos el directorio
			inodos->blq_inodos[i].size_fichero = inodos->blq_inodos[directorio[iFicheroOrigen].dir_inodo].size_fichero; //Actualizamos su tamaño
			ext_bytemaps->bmap_inodos[i]=1;//Marcar el inodo como ocupado
			ext_superblock->s_free_inodes_count--;//Actualizar el superbloque
			break;
		}
	}
		
	
	for(j=0; inodos->blq_inodos[directorio[iFicheroOrigen].dir_inodo].i_nbloque[j]!= 0xffff;j++){	
		for(i=0; i < MAX_BLOQUES_PARTICION; i++){
			if(ext_bytemaps->bmap_bloques[i]==0){
				memcpy(&memdatos[i-PRIM_BLOQUE_DATOS],&memdatos[inodos->blq_inodos[directorio[iFicheroOrigen].dir_inodo].i_nbloque[j]-PRIM_BLOQUE_DATOS] , SIZE_BLOQUE ); //Copiar los bloques 

				inodos->blq_inodos[directorio[iFicheroDestino].dir_inodo].i_nbloque[j] = i; //Asignar los bloques al inodo destino
				
				ext_bytemaps->bmap_bloques[i]=1;//Marcar los bloques como ocupados
				
				//Actualizar el superbloque
				ext_superblock->s_free_blocks_count--;
				if(ext_superblock->s_first_data_block > i) ext_superblock->s_first_data_block = i;
				
				break;
			}
		}
	}		   
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich){
	fwrite(inodos, sizeof(unsigned char), SIZE_BLOQUE, fich);
	fwrite(directorio, sizeof(unsigned char), SIZE_BLOQUE, fich);
}
	
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich){
	fwrite(ext_bytemaps, sizeof(unsigned char), SIZE_BLOQUE, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich){
	fwrite(ext_superblock, sizeof(unsigned char), SIZE_BLOQUE, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich){
	fwrite(memdatos, sizeof(unsigned char), MAX_BLOQUES_DATOS*SIZE_BLOQUE, fich);
}
