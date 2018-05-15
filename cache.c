#include "cache.h"

typedef struct {
  unsigned char w0[TAMANIO_VIA_SIN_METADATA + TAMANIO_METADATA_VIA];
  unsigned char w1[TAMANIO_VIA_SIN_METADATA + TAMANIO_METADATA_VIA];
  unsigned long long cantidad_de_misses;
  unsigned long long cantidad_de_hits;
} cache_t;

static cache_t cache;

/*
 * Validez debe ser la constante cache_estado_valido o cache_estado_invalido
 * El tag debe contener menos de 3 bits (Ser menor a 8)
 * A esto le falta un bit mas para dejar andando el LRU
 */
static unsigned char crear_metadata(unsigned char tag, unsigned char validez) {
  return tag + (validez << 3);
}

// Ejemplo teasteado:
// Direccion 511 tiene tag 0
// Direccion 512 tiene tag 1
static unsigned char obtener_tag(int address) {
  // El tag comienza a partir del bit 9
  return address >> 9;
}

// Devuelve la posicion de la metadata del bloque i dentro de la cache
static unsigned int posicion_metadata(unsigned int i) {
  return (i * (TAMANIO_BLOQUE_CON_METADATA + 1) - 1);
}

static unsigned int posicion_byte(unsigned char conjunto, unsigned char offset) {
  return (conjunto * TAMANIO_BLOQUE_CON_METADATA + offset);
}

static void marcar_usado_way_0(unsigned char conjunto) {
  // El quinto bit esta 1 si se uso ultimo
  // En 0 si no
  cache.w0[posicion_metadata(conjunto)] = cache.w0[posicion_metadata(conjunto)] | 0x10;

  cache.w1[posicion_metadata(conjunto)] = cache.w1[posicion_metadata(conjunto)] & 0x01;
}

static void marcar_usado_way_1(unsigned char conjunto) {
  // El quinto bit esta 1 si se uso ultimo
  // En 0 si no
  cache.w1[posicion_metadata(conjunto)] = cache.w1[posicion_metadata(conjunto)] | 0x10;

  cache.w0[posicion_metadata(conjunto)] = cache.w0[posicion_metadata(conjunto)] & 0x01;
}

static int w0_fue_el_ultimo_usado(unsigned char conjunto) {
  // 0 es falso, otra cosa es verdadero
  // Solo si hay un 1 en el quinto bit esto no es 0
  // Si hay un 1 en el quinto bit, fue usado, y devuelve que fue el ultimo usado
  return (cache.w0[posicion_metadata(conjunto)] & 0x10);
}

static void traer_bloque_al_cache(int address, int conjunto, const char *memoria) {
  // Para la posicion base borro el offset
  int posicion_base = address & 0xFE0;  // FFE = 1111 1110 0000
  if (w0_fue_el_ultimo_usado(conjunto)) {
    for (int i = 0; i < TAMANIO_BLOQUE_SIN_METADATA; i++)
      cache.w1[posicion_byte(conjunto, i)] = memoria[posicion_base + i];
    marcar_usado_way_1(conjunto);
  } else {
    for (int i = 0; i < TAMANIO_BLOQUE_SIN_METADATA; i++)
      cache.w0[posicion_byte(conjunto, i)] = memoria[posicion_base + i];
    marcar_usado_way_0(conjunto);
  }
}

void cache_init() {
  unsigned char TAG = 0;
  cache.cantidad_de_hits = 0;
  cache.cantidad_de_misses = 0;
  // La metadata del segundo bloque, por ejemplo, estaria en el indice 65
  // 33 el pimer bloque con metadata, 66 el segundo, y como c cuenta desde 0
  // queda su metadata en el 65

  for (int i = 0; i < CANTIDAD_DE_CONJUNTOS; i++) {
    cache.w0[TAMANIO_BLOQUE_CON_METADATA * i - 1] = crear_metadata(TAG, cache_estado_invalido);
    cache.w1[TAMANIO_BLOQUE_CON_METADATA * i - 1] = crear_metadata(TAG, cache_estado_invalido);
  }
}

unsigned char cache_read_byte(int address, const char *memoria) {
  unsigned char conjunto, tag_address, offset, nibble_esperado, nibble_w0, nibble_w1;

  // El numero de conjunto es el index
  conjunto = address % CANTIDAD_DE_CONJUNTOS;
  tag_address = obtener_tag(address);
  offset = address & 0x1F;

  // El nibble menos significado de la metadata debe contener el mismo
  // tag_address y ser valido
  nibble_esperado = crear_metadata(tag_address, cache_estado_valido);
  nibble_w0 = cache.w0[posicion_metadata(conjunto)] & 0x0F;
  nibble_w1 = cache.w1[posicion_metadata(conjunto)] & 0x0F;

  if (nibble_w0 == nibble_esperado) {
    marcar_usado_way_0(conjunto);
    cache.cantidad_de_hits++;
    return (cache.w0[posicion_byte(conjunto, offset)]);
  }

  if (nibble_w1 == nibble_esperado) {
    marcar_usado_way_1(conjunto);
    cache.cantidad_de_hits++;
    return (cache.w1[posicion_byte(conjunto, offset)]);
  }
  cache.cantidad_de_misses++;

  // Si no lo encontre traigo el bloque

  traer_bloque_al_cache(address, conjunto, memoria);
  return -1;
}
