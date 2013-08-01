#ifndef EXTENSION_H
#define EXTENSION_H

typedef struct {
  short address;
  short size;
  char *code;
} Extension;

Extension *extension_new(short address, short size, char* code);
int extension_load(Extension *self);
int extension_init(Extension *self);
void extension_free(Extension *self);

#endif // EXTENSION_H
