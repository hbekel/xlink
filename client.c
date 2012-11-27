#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>

#include "c64link.h"

unsigned char debugging = false;

unsigned char memory    = 0x37;
unsigned char cartridge = 0x10;

int start  = -1;
int end    = -1;
 
char* command = "";
char* argument = "";

void fail(const char* message) {
  fprintf(stderr, message);
  fflush(stderr);
  exit(EXIT_FAILURE);    
}

void load(char* filename) {

  FILE *file;
  struct stat st;
  long size;
  char *suffix;
  char *data;
 
  if(filename == NULL)
    fail("c64link: error: no file specified\n");

  file = fopen(filename, "r");

  if(file == NULL) {
    fprintf(stderr, "c64link: error: '%s': %s\n", filename, strerror(errno));
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  stat(filename, &st);
  size = st.st_size;

  // Determine start address
  suffix = (filename + strlen(filename)-4);

  if(start == -1) {
    if(strncmp(suffix, ".prg", 4) == 0) {
      fread(&start, sizeof(char), 2, file);
      start &= 0xffff;
      size -= 2;
    }
    else {
      fprintf(stderr, "c64link: error: not a .prg file and no start address specified\n");
      fflush(stderr);
      fclose(file);
      exit(EXIT_FAILURE);
    }
  }
    
  // Determine end address
  if(end == -1) {
    end = start + size;
  }

  // Read data
  data = (char*) calloc(size, sizeof(char));
  fread(data, sizeof(char), size, file);
  fclose(file);  

  cable_load(memory, cartridge, start, end, data, size);

  free(data);
}

void save(const char* filename) {
  
  FILE *file;
  int size;
  char *data;

  if(filename == NULL)
    fail("c64link: error: no file specified\n");

  if(start < 0 || start > 0xffff)
    fail("c64link: error: no start address specified or out of range");

  if(start < 0 || start > 0xffff)
    fail("c64link: error: no end address specified or out of range");  

  if(start >= end)
    fail("c64link: error: start address greater or equal to end address");  

  size = end-start;

  data = (char*) calloc(size, sizeof(char));

  file = fopen(filename, "w");

  if(file == NULL) {
    fprintf(stderr, "c64link: error: '%s': %s\n", filename, strerror(errno));
    fflush(stderr);
    free(data);
    exit(EXIT_FAILURE);
  }

  cable_save(memory, cartridge, start, end, data, size);

  fwrite(data, sizeof(char), size, file);
  fclose(file);

  free(data);    
}

void jump(const char* argument) {
  
  if(argument == NULL)
    fail("c64link: error: no address specified\n");

  int target = strtol(argument, NULL, 0);
  cable_jump(target);
}

void run(void) {
  cable_run();
}

void reset(void) {
  cable_reset();
}

void version(void) {
  printf("1.0\n");
}

void usage(void) {
  printf("Usage...\n");
}

void debug(void) {
  if(debugging)
    printf("%s (mem 0x%02X crt 0x%02X start 0x%04X end 0x%04X) %s\n",
	   command, memory, cartridge, start, end, argument);
} 

int main(int argc, char **argv) {
  
  struct option options[] = {
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { "debug", no_argument, NULL, 'd' },
    { "address", required_argument, NULL, 'a' },
    { "memory", required_argument, NULL, 'm' },
    { "cartridge", required_argument, NULL, 'c' },
    { 0, 0, 0, 0 },
  };
  int option;
  extern int optind;
  char* part;

  while (1) {

    option = getopt_long(argc, argv, "dhva:m:c:", options, &optind);

    switch (option) {

    case 'h':
      usage();
      exit(EXIT_SUCCESS);
      break;      

    case 'd':
      debugging = true;
      break;

    case 'v':
      version();
      break;

    case 'a':
      start = strtol(optarg, NULL, 0);

      if ((part = strstr(optarg, "-")) != NULL)
	end = strtol(part+1, NULL, 0);
      break;

    case 'm':
      memory = strtol(optarg, NULL, 0);
      break;

    case 'c':
      cartridge = strtol(optarg, NULL, 0);
      break;

    case -1:
      break;
  
    default:
      usage();
      exit(EXIT_FAILURE);
      break;      
    }

    if(option == -1)
      break;
  }

  if (argc - optind+1 >= 1)
    command = argv[optind];

  if (argc - optind+1 >= 2)
    argument = argv[optind+1];

  if(command == NULL)
    fail("c64link: error: no command specified\n");

  if(strncmp(command, "load", 4) == 0) {
    load(argument);
  }

  if(strncmp(command, "save", 4) == 0) {
    save(argument);
  }
  
  if(strncmp(command, "jump", 4) == 0) {
    jump(argument);
  }

  if(strncmp(command, "run", 3) == 0) {
    run();
  }

  if(strncmp(command, "reset", 5) == 0) {
    reset();
  }

  debug();

  exit(EXIT_SUCCESS);
}
