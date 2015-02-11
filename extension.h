#ifndef EXTENSION_H
#define EXTENSION_H

#include <stdbool.h>

typedef struct {
  unsigned short address;
  unsigned short size;
  unsigned char *code;
  unsigned char *cache;
  bool loaded;
} Extension;

Extension *extension_new(unsigned short address, unsigned short size, unsigned char* code);
int extension_load(Extension *self);
int extension_init(Extension *self);
int extension_unload(Extension *self);
void extension_free(Extension *self);

#endif // EXTENSION_H
