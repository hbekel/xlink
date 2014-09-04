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
  int saveToDisk, saveToTape;

  if(argc < 3) {
    fprintf(stderr, "Usage: compile-extension.c <file> <saveToDiskAddress> <saveToTapeAddress>\n");
    return EXIT_FAILURE;
  }

  filename = argv[1];

  if((f = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "%s: error opening %s\n", argv[0], filename);
    return EXIT_FAILURE;
  }

  saveToDisk = strtol(argv[2], NULL, 0);
  saveToTape = strtol(argv[3], NULL, 0);

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
  printf("%d print\"please run\":print:print\"  xlink load xlink-server.prg\":print\n", l+=10);
  printf("%d sys %d\n", l+=10, address);
  printf("%d print\"sys %d : rem save server to disk\"\n", l+=10, saveToDisk);
  printf("%d print\"sys %d : rem save server to tape\"\n", l+=10, saveToTape);
  printf("%d print\"sys\";:print (peek(251)+peek(252)*256);:print\": rem start the server\n", l+=10);
  printf("%d end\n", l+=10);
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
