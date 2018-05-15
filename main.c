#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define TAMANIO_MEMORIA 4096 // 4 KiB
#define TAMANIO_VIA_SIN_METADATA 512
#define TAMANIO_METADATA_VIA 16
#define CANTIDAD_DE_CONJUNTOS 16
#define TAMANIO_BLOQUE_CON_METADATA 33
#define TAMANIO_BLOQUE_SIN_METADATA 32
#define VALIDO 1
#define INVALIDO 0

char memoria[TAMANIO_MEMORIA];

struct t_cache{
    unsigned char w0[TAMANIO_VIA_SIN_METADATA + TAMANIO_METADATA_VIA];
    unsigned char w1[TAMANIO_VIA_SIN_METADATA + TAMANIO_METADATA_VIA];
    unsigned long long cantidad_de_misses;
    unsigned long long cantidad_de_hits;
}t_cache;

struct t_cache cache;


/*
 * Validez debe ser la constante VALIDO o INVALIDO
 * El tag debe contener menos de 3 bits (Ser menor a 8)
 * A esto le falta un bit mas para dejar andando el LRU
 */
unsigned char crear_metadata(unsigned char tag, unsigned char validez){
    return tag+(validez << 3);
}

//Ejemplo teasteado:
//Direccion 511 tiene tag 0
//Direccion 512 tiene tag 1
unsigned char obtener_tag(int address){
    //El tag comienza a partir del bit 9
    return address >> 9;
}

//Devuelve la posicion de la metadata del bloque i dentro de la cache
unsigned int posicion_metadata(unsigned int i){
    return (i*(TAMANIO_BLOQUE_CON_METADATA+1) - 1);
}

unsigned int posicion_byte(unsigned char conjunto, unsigned char offset){
    return (conjunto*TAMANIO_BLOQUE_CON_METADATA + offset);
}

/*
 * Reinicia la tasa miss e invalida la cache
 */
void init(){
    unsigned char TAG = 0;
    cache.cantidad_de_hits = 0;
    cache.cantidad_de_misses = 0;
    //La metadata del segundo bloque, por ejemplo, estaria en el indice 65
    // 33 el pimer bloque con metadata, 66 el segundo, y como c cuenta desde 0
    //queda su metadata en el 65
    
    for(int i = 0; i < CANTIDAD_DE_CONJUNTOS; i++){
        cache.w0[TAMANIO_BLOQUE_CON_METADATA*i - 1] = crear_metadata(TAG, INVALIDO);
        cache.w1[TAMANIO_BLOQUE_CON_METADATA*i - 1] = crear_metadata(TAG, INVALIDO);
    }
}

void marcar_usado_way_0(unsigned char conjunto){
    //El quinto bit esta 1 si se uso ultimo
    //En 0 si no
    cache.w0[posicion_metadata(conjunto)] =
        cache.w0[posicion_metadata(conjunto)] | 0x10;
        
    cache.w1[posicion_metadata(conjunto)] =
        cache.w1[posicion_metadata(conjunto)] & 0x01;
}

void marcar_usado_way_1(unsigned char conjunto){
    //El quinto bit esta 1 si se uso ultimo
    //En 0 si no
    cache.w1[posicion_metadata(conjunto)] =
        cache.w1[posicion_metadata(conjunto)] | 0x10;
        
    cache.w0[posicion_metadata(conjunto)] =
        cache.w0[posicion_metadata(conjunto)] & 0x01;
}


int w0_fue_el_ultimo_usado(unsigned char conjunto){
    //0 es falso, otra cosa es verdadero
    //Solo si hay un 1 en el quinto bit esto no es 0
    //Si hay un 1 en el quinto bit, fue usado, y devuelve que fue el ultimo usado
    return (cache.w0[posicion_metadata(conjunto)] & 0x10);
}

void traer_bloque_al_cache(int address, int conjunto){
    //Para la posicion base borro el offset
    int posicion_base = address & 0xFE0; // FFE = 1111 1110 0000
    if(w0_fue_el_ultimo_usado(conjunto)){
        for(int i = 0; i < TAMANIO_BLOQUE_SIN_METADATA; i++)
            cache.w1[posicion_byte(conjunto, i)] =
                memoria[posicion_base + i];
        marcar_usado_way_1(conjunto);
    }else{
        for(int i = 0; i < TAMANIO_BLOQUE_SIN_METADATA; i++)
            cache.w0[posicion_byte(conjunto, i)] =
                memoria[posicion_base + i];
        marcar_usado_way_0(conjunto);
    }
}

/*
 * Si el adress esta en la cache devuelve su valor
 * Si no lo carga de memoria y devuelve -1
 */
unsigned char read_byte(int address){
    unsigned char conjunto, tag_address, offset, nibble_esperado,
        nibble_w0, nibble_w1;

    
    //El numero de conjunto es el index
    conjunto = address % CANTIDAD_DE_CONJUNTOS;
    tag_address = obtener_tag(address);
    offset = address & 0x1F;
    
    //El nibble menos significado de la metadata debe contener el mismo 
    //tag_address y ser valido
    nibble_esperado = crear_metadata(tag_address, VALIDO);
    nibble_w0 = cache.w0[posicion_metadata(conjunto)] & 0x0F;
    nibble_w1 = cache.w1[posicion_metadata(conjunto)] & 0x0F;
    
    if(nibble_w0 == nibble_esperado){
        marcar_usado_way_0(conjunto);
        cache.cantidad_de_hits++;
        return(cache.w0[posicion_byte(conjunto, offset)]);
    }
    
    if(nibble_w1 == nibble_esperado){
        marcar_usado_way_1(conjunto);
        cache.cantidad_de_hits++;
        return(cache.w1[posicion_byte(conjunto, offset)]);
    }
    cache.cantidad_de_misses++;
    
    //Si no lo encontre traigo el bloque
    
    traer_bloque_al_cache(address, conjunto);
    return -1;
}


/*
 * lee de un archivo y devuelve su contenido en un string
 */
char* ReadFile(char *filename)
{
   char *buffer = NULL;
   int string_size, read_size;
   FILE *handler = fopen(filename, "r");

   if (handler)
   {
       // Seek the last byte of the file
       fseek(handler, 0, SEEK_END);
       // Offset from the first to the last byte, or in other words, filesize
       string_size = ftell(handler);
       // go back to the start of the file
       rewind(handler);

       // Allocate a string that can hold it all
       buffer = (char*) malloc(sizeof(char) * (string_size + 1) );

       // Read it all in one operation
       read_size = fread(buffer, sizeof(char), string_size, handler);

       // fread doesn't set it so put a \0 in the last position
       // and buffer is now officially a string
       buffer[string_size] = '\0';

       if (string_size != read_size)
       {
           // Something went wrong, throw away the memory and set
           // the buffer to NULL
           free(buffer);
           buffer = NULL;
       }

       // Always remember to close the file.
       fclose(handler);
    }

    return buffer;
}


/*
 * a partir de un string, devuelve un int en base a un substring 
 * determinado por start y len
 */
int IntFromString(char *string, int start, int len)
{
    char *number = (char*) malloc(len+1);
    strncpy(number, string+start, len);
    return atoi(number);
}


/*
 * a partir de un string, devuelve un int en base a un substring 
 * determinado por start y len
 */
int InterprateFile(char *string)
{
    // Interprets file and excecutes commands according to the read command
    puts(string);
    int i, aux, address;
    for (i = 0; i < strlen(string); ++i)
    {
        aux = 0;
        if (string[i] == 'W')
        {
            printf("write data\n");
            // get address and value
            // write_byte(address, value);
        }
        if (string[i] == 'R')
        {
            if (string[++i] != ' ')
                // file is corrupt
                return -1;
            do;
            while (string[i + ++aux] != '\n');
            address = IntFromString(string, ++i, aux -1);
            printf("Read \"%c\" from address %d\n", read_byte(address), address);
        }
        if (string[i] == 'M')
        {
            printf("miss data\n");
            // get_miss_rate();
        }
        // advance untill next line
        do i++;
        while (string[i] != '\n');
    }
    return 0;
}

int main(){
    int exit_code = 0;
    char *string = ReadFile("prueba1.mem");
    if (string)
    {
        exit_code = InterprateFile(string);
        free(string);
    }

    return exit_code;
}
