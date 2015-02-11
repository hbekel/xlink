#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  
  FILE *f;

  if(argc < 6) {
    fprintf(stderr, "Usage: compile-extension.c <file> <varname> <offset> <length>\n");
    return EXIT_FAILURE;
  }

  char *filename = argv[1];
  char *varname = argv[2];
  unsigned int address = strtol(argv[3], NULL, 0);
  long int offset = strtol(argv[4], NULL, 0);
  long int size = strtol(argv[5], NULL, 0);

  if((f = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "%s: error opening %s\n", argv[0], filename);
    return EXIT_FAILURE;
  }

  printf("#define EXTENSION_%s extension_new(%d, %ld, (unsigned char[]) { ", varname, address, size);
  fseek(f, offset, SEEK_SET);
  
  int i;
  for(i=0; i<size; i++) {
    printf("%d, ", fgetc(f));
  }
  printf("});\n\n");

  fclose(f);

  return EXIT_SUCCESS;
}

