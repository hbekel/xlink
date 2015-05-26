#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "machine.h"

#define HIGH 0
#define LOW 1

typedef struct {
  unsigned short delta;
  unsigned short lsb;
  unsigned short msb;
} address;

bool load(char* filename, unsigned char** data) {

  FILE *file;
  struct stat st;
  
  if((file = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "error: can't open %s: %s\n", filename, strerror(errno));
    return false;
  }

  stat(filename, &st);
  (*data) = (unsigned char*) realloc((*data), st.st_size);

  fread((*data), sizeof(unsigned char), st.st_size, file);

  fclose(file);
  
  return true;
}

int main(int argc, char **argv) {

  int result = EXIT_FAILURE;
  struct stat st;
  int size;

  argc--;
  argv++;
  
  char *name = argv[0];
  unsigned char *base   = (unsigned char *) calloc(1, sizeof(unsigned char));
  unsigned char *high   = (unsigned char *) calloc(1, sizeof(unsigned char));
  unsigned char *low    = (unsigned char *) calloc(1, sizeof(unsigned char));
  unsigned char *loader = (unsigned char *) calloc(1, sizeof(unsigned char));
  
  if(argc < 4) {
    fprintf(stderr, "usage: make-server <name> <base> <low> <high> <loader>\n");
    goto done;
  }

  if(!load(argv[1], &base))
    goto done;
    
  if(!load(argv[2], &low))
    goto done;
  
  if(!load(argv[3], &high))
    goto done;  

  if(!load(argv[4], &loader))
    goto done;  
  
  stat(argv[1], &st);
  size = st.st_size;

  stat(argv[2], &st);
  if (size != st.st_size) {
    fprintf(stderr, "error: files must be of equal size\n");
    goto done;
  }

  stat(argv[3], &st);
  if (size != st.st_size) {
    fprintf(stderr, "error: files must be of equal size\n");
    goto done;
  }

  stat(argv[4], &st);
  int loader_size = st.st_size;
  
  int count = 0;
  address** table = (address**) calloc(count, sizeof(address*));
  address* addr;
  
  for(int i=0; i<size; i++) {
    if(base[i] != low[i]) {

      table = (address**) realloc(table, (count+1) * sizeof(address*));
      addr = (address*) calloc(1, sizeof(address));

      addr->lsb = i;
      addr->msb = -1;
      addr->delta = base[i]-1;

      table[count] = addr;
      count++;
    }
  }

  int index=0;
  
  for(int i=0; i<size; i++) {
    if(base[i] != high[i]) {
      addr = table[index];
      addr->delta = (((base[i]-1) << 8) | addr->delta);
      addr->msb = i;
      index++;
    }
  }

  printf("#include <stdlib.h>\n");
  printf("#include \"xlink.h\"\n");
  printf("#include \"machine.h\"\n");  
  printf("#include \"error.h\"\n");  
  printf("unsigned char* xlink_server_%s(unsigned short address, int *size);\n", name);
  printf("unsigned char* xlink_server_basic_%s(int *size);\n", name);

  printf("unsigned char* xlink_server_basic_%s(int *size) {\n", name);
  
  printf("unsigned char loader[%d] = {", loader_size);
  for(int i=0; i<loader_size; i++) {
    printf("%d,", loader[i]);
  }  
  printf("};\n");
  printf("loader[0] = lo(machine->default_basic_start);\n");
  printf("loader[1] = hi(machine->default_basic_start);\n");
  printf("sprintf((char*)loader+7, \"%%d\", machine->default_basic_start+0x10);\n");
  
  printf("unsigned char* code = xlink_server_%s(0x4000-%d+2, size);\n", name, size);
  printf("unsigned char* result = (unsigned char*) calloc((*size)+%d-2, sizeof(unsigned char));\n", loader_size);

  printf("for(int i=0; i<%d; i++) { result[i] = loader[i]; }\n", loader_size);
  printf("for(int i=0; i<(*size)-2; i++) { result[i+%d] = code[i+2]; }\n", loader_size);
  printf("free(code);\n");
  printf("(*size) = (*size) + %d -2;\n", loader_size);
  printf("return result;\n");
  
  printf("}\n");
  
  printf("unsigned char* xlink_server_%s(unsigned short address, int *size) {\n", name);

  printf("(*size) = %d;\n", size);

  printf("if(address+%d > 0x10000) {\n", size);
  printf("   SET_ERROR(XLINK_ERROR_SERVER, \"Can't create server: out of memory\");\n");
  printf("   return NULL;\n");
  printf("}\n");
  
  printf("unsigned char code[%d] = { ", size);

  for(int i=0; i<size; i++) {
    printf("%d,", base[i]);
  }
  printf(" };\n");

  printf("unsigned char* result = (unsigned char*) calloc(%d, sizeof(unsigned char));\n", size);
  printf("unsigned short dest;\n");
  
  printf("for(int i=0; i<%d; i++) { result[i] = code[i]; };\n", size);

  for(int i=0; i<count; i++) {
    addr = table[i];
    printf("dest = address+%d;\n", addr->delta);
    printf("result[%d] = (unsigned char) (dest & 0xff);\n", addr->lsb);
    printf("result[%d] = (unsigned char) (dest >> 8);\n", addr->msb);    
  }

  printf("return result;\n");
  printf("}\n");      

  result = EXIT_SUCCESS;
  
 done:
  free(base);
  free(high);
  free(low);
  free(loader);
  return result;
}
