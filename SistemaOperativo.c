#include <stdio.h>

#include <stdlib.h>

#include <string.h>



// Constantes para definir los tamaños del sistema

#define SIZE_BLOQUE 512  // Tamaño de un bloque en bytes

#define MAX_BLOQUES_PARTICION 100  // Número máximo de bloques en la partición

#define MAX_INODOS 24  // Número máximo de inodos

#define MAX_NUMS_BLOQUE_INODO 7  // Número máximo de bloques asociados a un inodo

#define LEN_NFICH 32  // Longitud máxima del nombre de un archivo

#define FFFFH 0xFFFF  // Valor especial para indicar un bloque o inodo vacío



/* Estructuras */

typedef struct {

    unsigned int s_inodes_count;  // Número total de inodos

    unsigned int s_blocks_count;  // Número total de bloques

    unsigned int s_free_blocks_count;  // Número de bloques libres

    unsigned int s_free_inodes_count;  // Número de inodos libres

    unsigned int s_first_data_block;  // Primer bloque de datos

    unsigned int s_block_size;  // Tamaño de cada bloque

    unsigned char s_relleno[SIZE_BLOQUE - 6 * sizeof(unsigned int)];  // Espacio reservado para alinear la estructura

} EXT_SIMPLE_SUPERBLOCK;



typedef struct {

    unsigned char bmap_bloques[MAX_BLOQUES_PARTICION];  // Mapa de bits de los bloques

    unsigned char bmap_inodos[MAX_INODOS];  // Mapa de bits de los inodos

    unsigned char bmap_relleno[SIZE_BLOQUE - (MAX_BLOQUES_PARTICION + MAX_INODOS) * sizeof(char)];  // Espacio reservado

} EXT_BYTE_MAPS;



typedef struct {

    unsigned int size_fichero;  // Tamaño del archivo

    unsigned short int i_nbloque[MAX_NUMS_BLOQUE_INODO];  // Bloques asignados al archivo

} EXT_SIMPLE_INODE;



typedef struct {

    char dir_nfich[LEN_NFICH];  // Nombre del archivo o directorio

    unsigned short int dir_inodo;  // Inodo asociado a este archivo o directorio

} EXT_ENTRADA_DIR;



/* Variables globales */

unsigned char particion[MAX_BLOQUES_PARTICION * SIZE_BLOQUE];  // Simulación de la partición en memoria

EXT_SIMPLE_SUPERBLOCK *superbloque;  // Puntero al superbloque

EXT_BYTE_MAPS *bytemaps;  // Puntero al mapa de bits

EXT_SIMPLE_INODE *lista_inodos;  // Puntero a la lista de inodos

EXT_ENTRADA_DIR *directorio;  // Puntero al directorio raíz



/* Prototipos */

void cargar_particion(const char *nombre_fichero);

void inicializar_particion();

void comando_info();

void comando_bytemaps();

void comando_dir();

void comando_rename(const char *origen, const char *destino);

void comando_imprimir(const char *nombre);

void comando_remove(const char *nombre);

void comando_copy(const char *origen, const char *destino);

void crear_archivo_prueba(const char *nombre, unsigned int tamano, unsigned short int bloques[]);



int main() {

    // Cargar la partición desde el archivo "particion.bin"

    cargar_particion("particion.bin");

    inicializar_particion();



    // Crear un archivo de prueba "origen.txt" para demostración

    unsigned short int bloques_origen[] = {5, 6, 7, FFFFH};

    crear_archivo_prueba("origen.txt", 1536, bloques_origen);



    char comando[256];

    while (1) {

        printf(">> ");

        if (fgets(comando, sizeof(comando), stdin) == NULL) {

            break;  // Salir del bucle si no se puede leer entrada

        }



        char *args[3] = {NULL, NULL, NULL};

        int argc = 0;

        char *token = strtok(comando, " \n");

        while (token != NULL && argc < 3) {

            args[argc++] = token;

            token = strtok(NULL, " \n");

        }



        // Interpretar y ejecutar el comando ingresado

        if (strcmp(args[0], "salir") == 0) {

            break;

        } else if (strcmp(args[0], "info") == 0) {

            comando_info();

        } else if (strcmp(args[0], "bytemaps") == 0) {

            comando_bytemaps();

        } else if (strcmp(args[0], "dir") == 0) {

            comando_dir();

        } else if (strcmp(args[0], "rename") == 0 && args[1] && args[2]) {

            comando_rename(args[1], args[2]);

        } else if (strcmp(args[0], "imprimir") == 0 && args[1]) {

            comando_imprimir(args[1]);

        } else if (strcmp(args[0], "remove") == 0 && args[1]) {

            comando_remove(args[1]);

        } else if (strcmp(args[0], "copy") == 0 && args[1] && args[2]) {

            comando_copy(args[1], args[2]);

        } else {

            printf("Comando desconocido\n");

        }

    }



    return 0;

}



// Carga la partición desde un archivo binario

void cargar_particion(const char *nombre_fichero) {

    FILE *f = fopen(nombre_fichero, "rb");

    if (f == NULL) {

        perror("Error al abrir el archivo de partición");

        exit(EXIT_FAILURE);

    }



    // Leer el contenido del archivo "particion.bin" en memoria

    fread(particion, 1, sizeof(particion), f);

    fclose(f);



    // Inicializar los punteros a las estructuras clave dentro de la partición

    superbloque = (EXT_SIMPLE_SUPERBLOCK *)&particion[0];

    bytemaps = (EXT_BYTE_MAPS *)&particion[SIZE_BLOQUE];

    lista_inodos = (EXT_SIMPLE_INODE *)&particion[3 * SIZE_BLOQUE];

    directorio = (EXT_ENTRADA_DIR *)&particion[4 * SIZE_BLOQUE];

}



// Inicializa la partición en memoria

void inicializar_particion() {

    memset(particion, 0, sizeof(particion));  // Limpia toda la partición



    // Configurar el superbloque con valores iniciales

    superbloque->s_inodes_count = MAX_INODOS;

    superbloque->s_blocks_count = MAX_BLOQUES_PARTICION;

    superbloque->s_free_blocks_count = MAX_BLOQUES_PARTICION - 10; // Reservar 10 bloques

    superbloque->s_free_inodes_count = MAX_INODOS - 2;             // Reservar 2 inodos

    superbloque->s_first_data_block = 4;

    superbloque->s_block_size = SIZE_BLOQUE;



    // Configurar mapas de bits

    memset(bytemaps->bmap_bloques, 0, MAX_BLOQUES_PARTICION);

    memset(bytemaps->bmap_inodos, 0, MAX_INODOS);



    // Reservar bloques e inodos iniciales

    for (int i = 0; i < 10; i++) {

        bytemaps->bmap_bloques[i] = 1;

    }

    bytemaps->bmap_inodos[0] = 1;  // Reservar inodo 0

    bytemaps->bmap_inodos[1] = 1;  // Reservar inodo 1



    // Inicializar el directorio raíz con el nombre "origen"

    directorio[0].dir_inodo = 0;

    strncpy(directorio[0].dir_nfich, "origen", LEN_NFICH);



    // Inicializar el inodo raíz

    EXT_SIMPLE_INODE *inodo_raiz = &lista_inodos[0];

    inodo_raiz->size_fichero = 0; // Tamaño inicial

    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {

        inodo_raiz->i_nbloque[i] = FFFFH;  // Sin bloques asignados inicialmente

    }



    // Limpiar el resto de las entradas del directorio

    for (int i = 1; i < 20; i++) {

        directorio[i].dir_inodo = FFFFH; // Marca como vacío

        memset(directorio[i].dir_nfich, 0, LEN_NFICH);

    }

}



// Muestra la información del superbloque

void comando_info() {

    printf("Información del superbloque:\n");

    printf("Inodos totales: %u\n", superbloque->s_inodes_count);

    printf("Bloques totales: %u\n", superbloque->s_blocks_count);

    printf("Bloques libres: %u\n", superbloque->s_free_blocks_count);

    printf("Inodos libres: %u\n", superbloque->s_free_inodes_count);

    printf("Primer bloque de datos: %u\n", superbloque->s_first_data_block);

    printf("Tamaño del bloque: %u bytes\n", superbloque->s_block_size);

}



// Muestra los mapas de bits de bloques e inodos

void comando_bytemaps() {

    printf("Bytemap de bloques: ");

    for (int i = 0; i < 25; i++) {

        printf("%u ", bytemaps->bmap_bloques[i]);

    }

    printf("\n");



    printf("Bytemap de inodos: ");

    for (int i = 0; i < MAX_INODOS; i++) {

        printf("%u ", bytemaps->bmap_inodos[i]);

    }

    printf("\n");

}



// Lista los archivos en el directorio

void comando_dir() {

    printf("Directorio:\n");

    for (int i = 0; i < 20; i++) {

        if (directorio[i].dir_inodo != FFFFH) { // Solo muestra entradas válidas

            EXT_SIMPLE_INODE *inodo = &lista_inodos[directorio[i].dir_inodo];

            printf("Nombre: %s, Tamaño: %u, Inodo: %u, Bloques: ", directorio[i].dir_nfich, inodo->size_fichero, directorio[i].dir_inodo);

            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo->i_nbloque[j] != FFFFH; j++) {

                printf("%u ", inodo->i_nbloque[j]);

            }

            printf("\n");

        }

    }

}



// Renombra un archivo en el directorio

void comando_rename(const char *origen, const char *destino) {

    for (int i = 0; i < 20; i++) {

        if (directorio[i].dir_inodo != FFFFH && strcmp(directorio[i].dir_nfich, origen) == 0) {

            for (int j = 0; j < 20; j++) {

                if (strcmp(directorio[j].dir_nfich, destino) == 0) {

                    printf("Error: El nombre destino ya existe\n");

                    return;

                }

            }

            strncpy(directorio[i].dir_nfich, destino, LEN_NFICH);

            printf("Archivo renombrado correctamente\n");

            return;

        }

    }

    printf("Error: Archivo origen no encontrado\n");

}



// Imprime el contenido de un archivo en la consola

void comando_imprimir(const char *nombre) {

    for (int i = 0; i < 20; i++) {

        // Busca el archivo en el directorio

        if (directorio[i].dir_inodo != FFFFH && strcmp(directorio[i].dir_nfich, nombre) == 0) {

            EXT_SIMPLE_INODE *inodo = &lista_inodos[directorio[i].dir_inodo];

            // Recorre los bloques asociados al archivo

            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo->i_nbloque[j] != FFFFH; j++) {

                unsigned char *bloque = &particion[inodo->i_nbloque[j] * SIZE_BLOQUE];

                fwrite(bloque, 1, SIZE_BLOQUE, stdout);  // Imprime el contenido del bloque

            }

            printf("\n");

            return;  // Finaliza tras imprimir el archivo

        }

    }

    printf("Error: Archivo no encontrado\n");  // Si no se encuentra el archivo

}



// Elimina un archivo del sistema

void comando_remove(const char *nombre) {

    for (int i = 0; i < 20; i++) {

        // Busca el archivo en el directorio

        if (directorio[i].dir_inodo != FFFFH && strcmp(directorio[i].dir_nfich, nombre) == 0) {

            EXT_SIMPLE_INODE *inodo = &lista_inodos[directorio[i].dir_inodo];

            // Libera los bloques asociados al archivo

            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo->i_nbloque[j] != FFFFH; j++) {

                bytemaps->bmap_bloques[inodo->i_nbloque[j]] = 0;  // Marca el bloque como libre

                inodo->i_nbloque[j] = FFFFH;  // Limpia la referencia al bloque

            }

            inodo->size_fichero = 0;  // Marca el inodo como libre

            bytemaps->bmap_inodos[directorio[i].dir_inodo] = 0;

            memset(directorio[i].dir_nfich, 0, LEN_NFICH);  // Limpia el nombre del archivo

            directorio[i].dir_inodo = FFFFH;  // Marca la entrada del directorio como libre

            printf("Archivo eliminado correctamente\n");

            return;

        }

    }

    printf("Error: Archivo no encontrado\n");

}



// Copia un archivo a un nuevo destino

void comando_copy(const char *origen, const char *destino) {

    int origen_inodo = -1;

    // Busca el archivo origen en el directorio

    for (int i = 0; i < 20; i++) {

        if (directorio[i].dir_inodo != FFFFH && strcmp(directorio[i].dir_nfich, origen) == 0) {

            origen_inodo = directorio[i].dir_inodo;

            break;

        }

    }



    if (origen_inodo == -1) {

        printf("Error: Archivo origen no encontrado\n");

        return;

    }



    // Verifica que el archivo destino no exista

    for (int i = 0; i < 20; i++) {

        if (directorio[i].dir_inodo != FFFFH && strcmp(directorio[i].dir_nfich, destino) == 0) {

            printf("Error: El archivo destino ya existe\n");

            return;

        }

    }



    // Busca un inodo libre para el archivo destino

    int inodo_libre = -1;

    for (int i = 0; i < MAX_INODOS; i++) {

        if (bytemaps->bmap_inodos[i] == 0) {

            inodo_libre = i;

            break;

        }

    }



    if (inodo_libre == -1) {

        printf("Error: No hay inodos libres\n");

        return;

    }



    EXT_SIMPLE_INODE *inodo_origen = &lista_inodos[origen_inodo];

    EXT_SIMPLE_INODE *inodo_destino = &lista_inodos[inodo_libre];



    // Copia la información del inodo origen al destino

    inodo_destino->size_fichero = inodo_origen->size_fichero;

    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {

        inodo_destino->i_nbloque[i] = FFFFH;  // Inicializa las referencias a bloques

    }



    // Copia los bloques de datos

    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO && inodo_origen->i_nbloque[i] != FFFFH; i++) {

        int bloque_libre = -1;

        for (int j = 0; j < MAX_BLOQUES_PARTICION; j++) {

            if (bytemaps->bmap_bloques[j] == 0) {

                bloque_libre = j;

                break;

            }

        }



        if (bloque_libre == -1) {

            printf("Error: No hay bloques libres suficientes\n");

            return;

        }



        memcpy(&particion[bloque_libre * SIZE_BLOQUE], &particion[inodo_origen->i_nbloque[i] * SIZE_BLOQUE], SIZE_BLOQUE);  // Copia los datos del bloque

        inodo_destino->i_nbloque[i] = bloque_libre;  // Asigna el bloque al inodo destino

        bytemaps->bmap_bloques[bloque_libre] = 1;  // Marca el bloque como ocupado

    }



    bytemaps->bmap_inodos[inodo_libre] = 1;  // Marca el inodo destino como ocupado



    // Crea una entrada en el directorio para el archivo destino

    for (int i = 0; i < 20; i++) {

        if (directorio[i].dir_inodo == FFFFH) {

            strncpy(directorio[i].dir_nfich, destino, LEN_NFICH);

            directorio[i].dir_inodo = inodo_libre;

            printf("Archivo copiado correctamente\n");

            return;

        }

    }



    printf("Error: No hay espacio en el directorio\n");

}



// Crea un archivo de prueba en el sistema

void crear_archivo_prueba(const char *nombre, unsigned int tamano, unsigned short int bloques[]) {

    for (int i = 0; i < 20; i++) {

        if (directorio[i].dir_inodo == FFFFH) { // Busca una entrada libre en el directorio

            int inodo_libre = -1;

            for (int j = 0; j < MAX_INODOS; j++) {

                if (bytemaps->bmap_inodos[j] == 0) {

                    inodo_libre = j;

                    break;

                }

            }



            if (inodo_libre == -1) {

                printf("Error: No hay inodos libres para crear el archivo\n");

                return;

            }



            EXT_SIMPLE_INODE *nuevo_inodo = &lista_inodos[inodo_libre];

            nuevo_inodo->size_fichero = tamano;

            for (int k = 0; k < MAX_NUMS_BLOQUE_INODO; k++) {

                nuevo_inodo->i_nbloque[k] = (k < 7 && bloques[k] != FFFFH) ? bloques[k] : FFFFH;  // Asigna bloques al inodo

            }



            strncpy(directorio[i].dir_nfich, nombre, LEN_NFICH);

            directorio[i].dir_inodo = inodo_libre;  // Asocia el inodo al archivo



            bytemaps->bmap_inodos[inodo_libre] = 1;  // Marca el inodo como ocupado

            for (int k = 0; k < 7 && bloques[k] != FFFFH; k++) {

                bytemaps->bmap_bloques[bloques[k]] = 1;  // Marca los bloques como ocupados

            }



            printf("Archivo '%s' creado con éxito\n", nombre);

            return;

        }

    }

    printf("Error: No hay espacio en el directorio para registrar el archivo\n");

}
