#include "cache.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define ENABLE_LOG 1

#if ENABLE_LOG
#include <stdio.h>
#define LOG(...) (printf(__VA_ARGS__))
#else
#define LOG(...) \
  do {           \
  } while (0)
#endif

#define BLOCK(way, index) (cache.entries[way][index])

/** Marca la entrada del cache como válida. */
#define SET_VALID(way, index) (BLOCK(way, index).is_valid = 1)
/** Devuelve "true" si el cache está marcado como válido. */
#define IS_VALID(way, index) (BLOCK(way, index).is_valid == 1)

/** Marca la entrada del cache como nueva (LRU). */
#define SET_NEW(way, index) (BLOCK(way, index).is_new = 1)
/** Marca la entrada del cache como vieja (LRU). */
#define SET_OLD(way, index) (BLOCK(way, index).is_new = 0)
/** Retorns "true" si la entrada es vieja. */
#define IS_OLD(way, index) (BLOCK(way, index).is_new == 0)

/** Obtiene el offset desde la dirección de memoria. */
#define OFFSET(address) (address & 0x1F)
/** Obtiene el index desde la dirección de memoria. */
#define INDEX(address) ((address >> 5) & 0x0F)
/** Obtiene el tag desde la dirección de memoria. */
#define TAG(address) ((address >> 9) & 0x07)

/** Bloque de datos en el cache. */
typedef uint8_t cache_block_t[CACHE_BS];

/** Entrada en el cache. */
typedef struct {
  uint8_t tag : 3;
  uint8_t is_valid : 1;
  uint8_t is_new : 1;
  cache_block_t data;
} cache_entry_t;

/** Cache. */
typedef struct {
  cache_entry_t entries[CACHE_WAYS_NUM][CACHE_BLOCKS_PER_WAY];
  double miss_count;
  double hit_count;
} cache_t;

/** Instancia global del cache. */
static cache_t cache;

/**
 * @brief Carga un bloque de memoria principal a a la cache.
 *
 * @param index El indice del conjunto de la cache.
 * @param tag El tag del bloque.
 * @param mem Memoria principal.
 * @return La vía en la cual se cargó el bloque.
 */
static void _load_block(int index, int tag, const char *mem) {
  int way = 0;
  if (IS_OLD(0, index)) {
    way = 0;
  } else {
    way = 1;
  }

  cache_entry_t *entry = &cache.entries[way][index];

  /* copia los datos y actualiza los metadatos */
  memcpy(entry->data, mem, CACHE_BS);

  SET_NEW(way, index);
  SET_OLD(!way, index);
  SET_VALID(way, index);
  entry->tag = tag;

  LOG("RM[%d][%d]    -> ", way, index);
}

/**
 * @brief Lee un byte del cache y actualiza los metadatos.
 *
 * @param way Via a leer.
 * @param index Indice del groupo.
 * @param offset Offset del bloque.
 * @return char Dato.
 */
static char _read_cache(int way, int index, int offset) {
  cache.entries[way][index].is_new = 1;
  cache.entries[!way][index].is_new = 0;
  char value = cache.entries[way][index].data[offset];

  LOG("RH[%d][%d][%d] -> ", way, index, offset);
  return value;
}

/**
 * @brief Inicializa el cache.
 *
 */
void cache_init() {
  memset(&cache, 0, sizeof(cache_t));
}

/**
 * @brief Lee un byte del cache.
 *
 * @param address Dirección del byte a leer.
 * @param memoria Memoria.
 * @return unsigned char Byte leído.
 */
char cache_read_byte(int address, const char *memory) {
  int index = INDEX(address);
  int offset = OFFSET(address);

  if (IS_VALID(0, index) && TAG(address) == cache.entries[0][index].tag) {
    cache.hit_count++;
    return _read_cache(0, index, offset);
  } else if (IS_VALID(1, index) && TAG(address) == cache.entries[1][index].tag) {
    cache.hit_count++;
    return _read_cache(1, index, offset);
  } else {
    cache.miss_count++;
    _load_block(index, TAG(address), memory);
    return -1;
  }
}

/**
 * @brief Escribe en la memoria y si en necesario ne el cache.
 *
 * @param address Dirección de memoria en la que se escribe.
 * @param value Valor a escribir.
 * @param memory Memoria principal.
 * @return int 0 si fue un hit, -1 si fue un miss.
 */
int cache_write_byte(int address, unsigned char value, char *memory) {
  int index = INDEX(address);
  int offset = OFFSET(address);

  int rv = 0;

  if (IS_VALID(0, index) && TAG(address) == cache.entries[0][index].tag) {
    cache.hit_count++;
    *(cache.entries[0][index].data + offset) = value;
    cache.entries[0][index].is_new = 1;
    cache.entries[1][index].is_new = 0;
    LOG("WH[%d][%d][%d]=%d\n", 0, index, offset, value);
  } else if (IS_VALID(1, index) && TAG(address) == cache.entries[1][index].tag) {
    cache.hit_count++;
    *(cache.entries[1][index].data + offset) = value;
    cache.entries[1][index].is_new = 1;
    cache.entries[0][index].is_new = 0;
    LOG("WH[%d][%d][%d]=%d\n", 1, index, offset, value);
  } else {
    cache.miss_count++;
    LOG("WM\n");
    rv = -1;
  }

  /* WT: siempre escribe a memoria */
  *(memory + address) = value;
  return rv;
}

/**
 * @brief Devuelve el miss rate redondeado al entero mas cercano.
 *
 * @return uint64_t miss rate.
 */
int cache_get_miss_rate() {
  return round((cache.miss_count * 100) / (cache.miss_count + cache.hit_count));
}
