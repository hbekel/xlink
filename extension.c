#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "util.h"
#include "extension.h"
#include "xlink.h"

extern int xlink_extend(int address);

Extension *extension_new(unsigned short address, unsigned short size, unsigned char* code) {
  
  Extension *extension = (Extension*) calloc(1, sizeof(Extension));
  extension->address = address;
  extension->size = size;
  extension->code = code;
  extension->cache = (unsigned char*) calloc(size, sizeof(unsigned char));
  extension->loaded = false;
  return extension;
}

int extension_load(Extension *self) {
  
  bool result = false;
  
  if (!xlink_save(0x37|0x80, 0x00, self->address, self->address+self->size, self->cache, self->size)) {
    goto done;
  }
  
  if (xlink_load(0x37|0x80, 0x00, self->address, self->address+self->size, self->code, self->size)) {
    result = true;
  }
  
 done:
  self->loaded = result;
  return result;
}

int extension_unload(Extension *self) {

  bool result = false;

  if(self->loaded) {
    result = xlink_load(0x37|0x80, 0x00, self->address, self->address+self->size, self->cache, self->size);
    self->loaded = !result;
  }  
  return result;  
}

int extension_init(Extension *self) {
  return xlink_extend(self->address);  
}

void extension_free(Extension *self) {
  free(self->cache);
  free(self);
}
