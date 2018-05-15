#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"

#define TAMANIO_MEMORIA 4096  // 4 KiB

char memoria[TAMANIO_MEMORIA];

/*
 * Reinicia la tasa miss e invalida la cache
 */
void init() {
  cache_init();
}

/*
 * Si el adress esta en la cache devuelve su valor
 * Si no lo carga de memoria y devuelve -1
 */
unsigned char read_byte(int address) {
  return cache_read_byte(address, memoria);
}

/*
 * lee de un archivo y devuelve su contenido en un string
 */
char *ReadFile(char *filename) {
  char *buffer = NULL;
  int string_size, read_size;
  FILE *handler = fopen(filename, "r");

  if (handler) {
    // Seek the last byte of the file
    fseek(handler, 0, SEEK_END);
    // Offset from the first to the last byte, or in other words, filesize
    string_size = ftell(handler);
    // go back to the start of the file
    rewind(handler);

    // Allocate a string that can hold it all
    buffer = (char *)malloc(sizeof(char) * (string_size + 1));

    // Read it all in one operation
    read_size = fread(buffer, sizeof(char), string_size, handler);

    // fread doesn't set it so put a \0 in the last position
    // and buffer is now officially a string
    buffer[string_size] = '\0';

    if (string_size != read_size) {
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
int IntFromString(char *string, int start, int len) {
  char *number = (char *)malloc(len + 1);
  strncpy(number, string + start, len);
  return atoi(number);
}

/*
 * a partir de un string, devuelve un int en base a un substring
 * determinado por start y len
 */
int InterprateFile(char *string) {
  // Interprets file and excecutes commands according to the read command
  puts(string);
  int i, aux, address;
  for (i = 0; i < strlen(string); ++i) {
    aux = 0;
    if (string[i] == 'W') {
      printf("write data\n");
      // get address and value
      // write_byte(address, value);
    }
    if (string[i] == 'R') {
      if (string[++i] != ' ')
        // file is corrupt
        return -1;
      do
        ;
      while (string[i + ++aux] != '\n');
      address = IntFromString(string, ++i, aux - 1);
      printf("Read \"%c\" from address %d\n", read_byte(address), address);
    }
    if (string[i] == 'M') {
      printf("miss data\n");
      // get_miss_rate();
    }
    // advance untill next line
    do
      i++;
    while (string[i] != '\n');
  }
  return 0;
}

int main() {
  int exit_code = 0;
  char *string = ReadFile("prueba1.mem");
  if (string) {
    exit_code = InterprateFile(string);
    free(string);
  }

  return exit_code;
}
