#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

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
  
  unsigned char *base = (unsigned char *) calloc(1, sizeof(unsigned char));
  unsigned char *high = (unsigned char *) calloc(1, sizeof(unsigned char));
  unsigned char *low  = (unsigned char *) calloc(1, sizeof(unsigned char));
  
  argc--;
  argv++;

  if(argc < 3) {
    fprintf(stderr, "usage: make-server <base> <low> <high>\n");
    goto done;
  }

  if(!load(argv[0], &base))
    goto done;
    
  if(!load(argv[1], &low))
    goto done;
  
  if(!load(argv[2], &high))
    goto done;  
  
  stat(argv[0], &st);
  size = st.st_size;

  stat(argv[1], &st);
  if (size != st.st_size) {
    fprintf(stderr, "error: files must be of equal size\n");
    goto done;
  }

  stat(argv[2], &st);
  if (size != st.st_size) {
    fprintf(stderr, "error: files must be of equal size\n");
    goto done;
  }
  
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
  printf("#include \"error.h\"\n");  

  printf("unsigned char* xlink_server_basic(int *size) {\n");
  printf("unsigned char basic[15] = { 0x01, 0x08, 0x0b, 0x08, 0x0a, 0x00, 0x9e, 0x32, 0x30, 0x36, 0x32, 0x00, 0x00, 0x00, 0x00 };\n");

  printf("unsigned char* code = xlink_server(0x080e, size);\n");
  printf("unsigned char* result = (unsigned char*) calloc((*size)+11, sizeof(unsigned char));\n");

  printf("for(int i=0; i<15; i++) { result[i] = basic[i]; }\n");
  printf("for(int i=0; i<(*size)-2; i++) { result[i+15] = code[i+2]; }\n");
  printf("free(code);\n");
  printf("(*size) = (*size) + 13;\n");
  printf("return result;\n");
  
  printf("}\n");
  
  printf("unsigned char* xlink_server(unsigned short address, int *size) {\n");

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
  return result;
}
