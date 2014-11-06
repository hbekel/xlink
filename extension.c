#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "util.h"
#include "extension.h"
#include "xlink.h"

extern int xlink_extend(int address);

Extension *extension_new(short address, short size, char* code) {
  
  Extension *extension = (Extension*) calloc(1, sizeof(Extension));
  extension->address = address;
  extension->size = size;
  extension->code = code;
  extension->cache = (char*) calloc(size, sizeof(char));
  extension->loaded = false;
  return extension;
}

int extension_load(Extension *self) {
  
  logger->enter("extension_load");

  bool result = false;
  
  if (!xlink_save(0x37|0x80, 0x00, self->address, self->address+self->size, self->cache, self->size)) {
    goto done;
  }
  
  if (xlink_load(0x37|0x80, 0x00, self->address, self->address+self->size, self->code, self->size)) {
    result = true;
  }
  
 done:
  self->loaded = result;
  logger->leave();
  return result;
}

int extension_unload(Extension *self) {

  logger->enter("extension_unload");

  bool result = false;

  if(self->loaded) {
    result = xlink_load(0x37|0x80, 0x00, self->address, self->address+self->size, self->cache, self->size);
    self->loaded = !result;
  }
  
  logger->leave();
  return result;  
}

int extension_init(Extension *self) {

  logger->enter("extension_init");

  bool result = xlink_extend(self->address);
  
  logger->leave();
  return result;
}

void extension_free(Extension *self) {
  free(self->cache);
  free(self);
}
