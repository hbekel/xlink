#ifndef EXTENSION_H
#define EXTENSION_H

#include <stdbool.h>

typedef struct {
  short address;
  short size;
  char *code;
  char *cache;
  bool loaded;
} Extension;

Extension *extension_new(short address, short size, char* code);
int extension_load(Extension *self);
int extension_init(Extension *self);
int extension_unload(Extension *self);
void extension_free(Extension *self);

#endif // EXTENSION_H
