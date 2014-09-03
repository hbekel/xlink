#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
  
  FILE *f;
  char *filename;
  struct stat st;
  int address;
  int size;
  int l;
  int checksum = 0;

  if(argc < 1) {
    fprintf(stderr, "Usage: compile-extension.c <file>\n");
    return EXIT_FAILURE;
  }

  filename = argv[1];

  if((f = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "%s: error opening %s\n", argv[0], filename);
    return EXIT_FAILURE;
  }

  address = fgetc(f);
  address = address | fgetc(f) << 8;

  stat(filename, &st);
  size = st.st_size-2;
  size = size + (8-(size % 8));

  unsigned char *data = (unsigned char*) calloc(size, sizeof(char));

  fread(data, sizeof(char), size, f);
  fclose(f);

  l = 0;

  printf("%d print\"please wait...\":print\n", l+=10);
  printf("%d d=%d:s=%d:c=0:l=1000:m=0\n", l+=10, address, size);
  printf("%d for i=0 to s-1:for k=0 to 7\n", l+=10);
  printf("%d read v:poke d+i+k,v:c=c+v:next k\n", l+=10);
  printf("%d read v:if c<>v then print\"data checksum error on line\";l:end\n", l+=10);
  printf("%d c=0:l=l+1:i=i+7:nexti\n", l+=10);
  printf("%d print\"insert disk, then run\":print:print\"  xlink load xlink-server.prg\"\n", l+=10);
  printf("%d sys4096\n", l+=10);
  printf("%d print:print\"sys49152:rem start the server\"\n", l+=10);

  l = 1000;

  for(int i=0; i<size; i++) {
    if (i%8 == 0) {
      
      if(checksum) {
	printf("%d\n", checksum);
      }
      printf("%d data ", l);
      
      checksum=0;
      l++;
    }
    printf("%d,", data[i]);
    checksum += data[i];

    if(i==size-1 && i%8 != 0) {
      printf("%d\n", checksum);
    }
  }
  free(data);
       
  return EXIT_SUCCESS;
}
