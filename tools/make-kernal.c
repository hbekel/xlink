#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

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

  argc--;
  argv++;

  if(argc == 0) {
    fprintf(stderr, "Usage: make-kernal <name> <kernal.bin> [<offset, size>...]\n");
    return EXIT_FAILURE;
  }

  char *name = argv[0];

  argc--;
  argv++;
  
  struct stat st;
  char *filename = argv[0];

  if(stat(filename, &st) == -1) {
    fprintf(stderr, "%s: couldn't stat: %s", filename, strerror(errno));
    return EXIT_FAILURE;
  }

  argc--;
  argv++;

  if(argc % 2 != 0) {
    fprintf(stderr, "error: even number of arguments (<offset> <size>) expected\n");
    return EXIT_FAILURE;    
  }

  unsigned char *code = (unsigned char *) calloc(1, sizeof(unsigned char));

  if(!load(filename, &code)) {
    goto done;
  }

  int numPatches = argc/2;
  int offset, size;
  
  printf("void xlink_kernal_%s(unsigned char* image) {\n", name);

  printf("typedef struct {\n");
  printf("  unsigned short offset;\n");
  printf("  unsigned short size;\n");
  printf("  unsigned char* data;\n");
  printf("} Patch;\n");

  printf("Patch *patches[%d];\n", numPatches);
  
  for(int i=0; i<numPatches; i++) {
    offset = strtol(argv[i*2], NULL, 0);
    size = strtol(argv[i*2+1], NULL, 0);

    printf("patches[%d] = &(Patch) {\n", i);
    printf("  .offset = %d,\n", offset);
    printf("  .size = %d,\n", size);

    printf("  .data = (unsigned char[%d]) {", size);
    for(int j=0; j<size; j++) {
      printf("%d,", code[offset+j]);
    }
    printf("}\n");
    printf("};\n");
  }

  printf("for(int i=0; i<%d; i++) {\n", numPatches);
  printf("  for(int k=0; k<patches[i]->size; k++) {\n");
  printf("    image[patches[i]->offset+k] = patches[i]->data[k];\n");
  printf("  }\n");
  printf("}\n");
  
  printf("}\n");
  
 done: 
  free(code);
  return EXIT_SUCCESS;
}
