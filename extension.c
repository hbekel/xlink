#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "util.h"
#include "extension.h"
#include "xlink.h"
#include "machine.h"

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

int extension_preload(Extension *self) {
  
  bool result = false;
  uchar memory = machine->memory | 0x80;
  uchar bank = machine->bank;
  
  if (!xlink_save(memory, bank, self->address, self->cache, self->size)) {
    goto done;
  }
  
  if (xlink_load(memory, bank, self->address, self->code, self->size)) {
    result = true;
  }
  
 done:
  self->loaded = result;
  return result;
}

int extension_unload(Extension *self) {

  bool result = false;
  uchar memory = machine->memory | 0x80;
  uchar bank = machine->bank;

  if(self->loaded) {
    result = xlink_load(memory, bank, self->address, self->cache, self->size);
    self->loaded = !result;
  }  
  return result;  
}

int extension_init(Extension *self) {
  return xlink_inject(self->address, self->code, self->size);  
}

void extension_free(Extension *self) {
  free(self->cache);
  free(self);
}
