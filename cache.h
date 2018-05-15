#ifndef CACHE_H_
#define CACHE_H_

#define TAMANIO_VIA_SIN_METADATA 512
#define TAMANIO_METADATA_VIA 16
#define CANTIDAD_DE_CONJUNTOS 16
#define TAMANIO_BLOQUE_CON_METADATA 33
#define TAMANIO_BLOQUE_SIN_METADATA 32

typedef enum {
  cache_estado_invalido,
  cache_estado_valido,
} cache_estado_t;

void cache_init();
unsigned char cache_read_byte(int address, const char *memoria);
int cache_write_byte(int address, unsigned char value, char *memoria);
unsigned int cache_get_miss_rate();

#endif
