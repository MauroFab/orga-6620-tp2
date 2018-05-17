#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"

#define MEM_SIZE 4096  // 4 KiB

char memory[MEM_SIZE];

/*
 * Reinicia la tasa miss e invalida la cache
 */
void init() {
  cache_init();
}

/*
 * Si el adress esta en la cache devuelve su valor
 * Si no lo carga de memory y devuelve -1
 */
static char read_byte(int address) {
  if (address >= MEM_SIZE) {
    return -1;
  }
  char rv = cache_read_byte(address, memory);
  if (rv == -1) {
    return memory[address];
  }
  return rv;
}

static int write_byte(int address, unsigned char value) {
  if (address >= MEM_SIZE) {
    return -1;
  }
  return cache_write_byte(address, value, memory);
}

static bool _process_file(FILE *file) {
  while (!feof(file)) {
    unsigned int address;
    unsigned int value;
    char command = '\0';

    if (fscanf(file, "%c", &command) != 1) {
      /* verifica que se trate el fin de archivo */
      return (feof(file) > 0);
    }

    switch (command) {
      case 'R':
        if (fscanf(file, "%u", &address) != 1) {
          return false;
        }
        printf("%d\n", read_byte(address));
        break;

      case 'W':
        if (fscanf(file, "%u, %u", &address, &value) != 2) {
          return false;
        }
        write_byte(address, value);
        break;

      case 'M':
        if (fscanf(file, "%c", &command) != 1) {
          return false;
        }
        if (command != 'R') {
          return false;
        }

        printf("MR=%d\n", cache_get_miss_rate());
        break;

      default:
        return false;
    }

    /* lee el fin de linea */
    if (fread(&value, 1, 1, file) == 0) {
      break;
    }
  }

  return true;
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Uso: %s [archivo]\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    printf("No se pudo abrir el archivo.\n");
    return EXIT_FAILURE;
  }

  init();

  if (!_process_file(file)) {
    goto error;
  }

  fclose(file);
  return EXIT_SUCCESS;

error:
  fclose(file);
  printf("Error de formato.\n");
  return EXIT_FAILURE;
}
