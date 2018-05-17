#ifndef CACHE_H_
#define CACHE_H_

#include <stdint.h>

/**
 * bits: 31            11    9       5        0
 *       --------------------------------------
 *       |             | tag | index | offset |
 *       --------------------------------------
 */

/** Tanaño total de la cache */
#define CACHE_SIZE 1024
/** Tamaño de cada bloque de la cache */
#define CACHE_BS 32
/** Número de vías. */
#define CACHE_WAYS_NUM 2

/** Tamaño de cada vía. */
#define CACHE_WAY_SIZE (CACHE_SIZE / CACHE_WAYS_NUM)
#define CACHE_BLOCKS_PER_WAY ((CACHE_SIZE / CACHE_WAYS_NUM) / CACHE_BS)

void cache_init();
char cache_read_byte(int address, const char *memory);
int cache_write_byte(int address, unsigned char value, char *memory);
int cache_get_miss_rate();

#endif
