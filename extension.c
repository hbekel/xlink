#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "util.h"
#include "extension.h"
#include "pp64.h"

extern int pp64_extend(int address);

Extension *extension_new(short address, short size, char* code) {
  
  Extension *extension = (Extension*) calloc(1, sizeof(Extension));
  extension->address = address;
  extension->size = size;
  extension->code = code;
  return extension;
}

int extension_load(Extension *self) {

  logger->enter("extension_load");

  bool result = false;

  if (pp64_load(0x37|0x80, 0x00, self->address, self->address+self->size, self->code, self->size)) {
    result = true;
  }

  logger->leave();
  return result;
}

int extension_init(Extension *self) {

  logger->enter("extension_init");

  bool result = pp64_extend(self->address);
  
  logger->leave();
  return result;
}

void extension_free(Extension *self) {
  free(self);
}
