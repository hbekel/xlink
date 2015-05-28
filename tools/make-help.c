#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
  argc--;
  argv++;

  if(argc == 0) {
    fprintf(stderr, "Usage: make-help <filename>\n");
    return EXIT_FAILURE;
  }

  FILE* f;
  char *filename = argv[0];
  char *line = NULL;
  size_t ignored = 0;
  int len = 0;

  if((f = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "%s: %s\n", filename, strerror(errno)); 
    return EXIT_FAILURE;
  }

  printf("bool help(int id) {\n");
  
  printf("switch(id) {\n");
  printf("case COMMAND_NONE:\n");
  printf("usage();\n");
  
  while((len = getline(&line, &ignored, f)) != -1) {

    if(line[0] == '#') continue;
    
    line[len-1] = '\0';
    
    if(strncmp(line, "COMMAND", 7) == 0) {
      printf("break;\n\n");
      printf("case %s:\n", line);
    }
    else {
      if(len == 1) {
	printf("printf(\"\\n\");\n");
      }
      else {
	printf("printf(\"%s\\n\");\n", line);
      }
    }
  }
  
  fclose(f);

  printf("break;\n");
  printf("}\n");
  printf("return true;\n");
  printf("}\n");

  free(line);
  return EXIT_SUCCESS;
}
