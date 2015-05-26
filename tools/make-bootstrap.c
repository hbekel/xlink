#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
  
  FILE *f;
  char *filename;
  char *machine;
  struct stat st;
  int address;
  int size;
  int l;
  int checksum = 0;

  if(argc < 1) {
    fprintf(stderr, "Usage: make-bootstrap <machine> <file>");
    return EXIT_FAILURE;
  }

  machine = argv[1];
  filename = argv[2];
  
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

  if(strcmp(machine, "c128") == 0) {
    printf("%d print chr$(14)\n", l+=10);
  }
  printf("%d print\"please wait...\":print\n", l+=10);
  printf("%d d=%d:s=%d:c=0:l=1000:m=0\n", l+=10, address, size);
  printf("%d for i=0 to s-1:for k=0 to 7\n", l+=10);
  printf("%d read v:poke d+i+k,v:c=c+v:next k\n", l+=10);
  printf("%d read v:if c<>v then print\"data checksum error on line\";l:end\n", l+=10);
  printf("%d c=0:l=l+1:i=i+7:nexti\n", l+=10);
  printf("%d print\"on your pc, please run\":print\n", l+=10);

  if(strcmp(machine, "c64") == 0) {
    printf("%d print\"xlink server xlink.prg\"\n", l+=10);
    printf("%d print\"xlink load xlink.prg\":print\n", l+=10);
  }
  else if(strcmp(machine, "c128") == 0) {
    printf("%d print\"xlink server -M c128 xlink.prg\"\n", l+=10);
    printf("%d print\"xlink load -M c128 xlink.prg\":print\n", l+=10);
  }

  printf("%d print\"then save the server:\":print\n", l+=10);
  printf("%d print\"save\"chr$(34)\"xlink\"chr$(34)\",8\"chr$(145)\n", l+=10);
  printf("%d sys %d\n", l+=10, address);
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
