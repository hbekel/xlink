#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "extension.h"
#include "pp64.h"

extern int pp64_extend(int address);

Extension *extension_new(short address, short size, char* code) {
  
  Extension *extension = (Extension*) calloc(1, sizeof(Extension));
  extension->address = address;
  extension->size = size;
  extension-> code = code;
  return extension;
}

int extension_load(Extension *self) {

  if (pp64_load(0x37|0x80, 0x10, self->address, self->address+self->size, self->code, self->size)) {
    return true;
  }
  return false;
}

int extension_init(Extension *self) {

  if(pp64_ping(250)) {

    pp64_extend(self->address);
    return true;
  }
  return false;
}

void extension_free(Extension *self) {
  free(self);
}
