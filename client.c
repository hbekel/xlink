#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>

#include "client.h"
#include "pp64.h"

#define COMMAND_AUTO  0x00
#define COMMAND_LOAD  0x01
#define COMMAND_SAVE  0x02
#define COMMAND_POKE  0x03
#define COMMAND_PEEK  0x04
#define COMMAND_JUMP  0x05
#define COMMAND_RUN   0x06
#define COMMAND_RESET 0x07

Commands* commands;
int debug = false;

char str2id(const char* arg) {
  if (strstr(arg, "c64") != NULL) return COMMAND_AUTO;
  if (strncmp(arg, "load" , 4) == 0) return COMMAND_LOAD;
  if (strncmp(arg, "save" , 4) == 0) return COMMAND_SAVE;
  if (strncmp(arg, "poke" , 4) == 0) return COMMAND_POKE;
  if (strncmp(arg, "peek" , 4) == 0) return COMMAND_PEEK;
  if (strncmp(arg, "jump" , 4) == 0) return COMMAND_JUMP;
  if (strncmp(arg, "run"  , 3) == 0) return COMMAND_RUN;  
  if (strncmp(arg, "reset", 3) == 0) return COMMAND_RESET;  
  return -1;
}

char* id2str(const char id) {
  if (id == COMMAND_AUTO)   return "auto";
  if (id == COMMAND_LOAD)   return "load";
  if (id == COMMAND_SAVE)   return "save";
  if (id == COMMAND_POKE)   return "poke";
  if (id == COMMAND_PEEK)   return "peek";
  if (id == COMMAND_JUMP)   return "jump";
  if (id == COMMAND_RUN)    return "run";
  if (id == COMMAND_RESET)  return "reset";
  return "unknown";
}

int valid(int address) {
  return address >= 0x0000 && address <= 0xffff; 
}    

Commands* commands_new() {
  Commands* commands = calloc(1, sizeof(Commands*));
  commands->count = 0;
  commands->items = calloc(1, sizeof(Command**));
  return commands;
}

Command* commands_add(Commands* self, Command* command) {
  self->items = realloc(self->items, (self->count+1) * sizeof(Command*));
  self->items[self->count] = command;
  self->count++;
  return command;
}

Command* command_new(char id) {
  Command* command = (Command *) calloc(1, sizeof(Command));
  command->id     = id;
  command->memory = 0xff;
  command->bank   = 0xff;
  command->start  = -1;
  command->end    = -1;
  command->argc   = 0;
  command->argv   = calloc(1, sizeof(char*));
  command_append_argument(command, "c64");
  return command;
}

void command_append_argument(Command* self, char* arg) {
  self->argv = realloc(self->argv, (self->argc+1) * sizeof(char*));
  self->argv[self->argc] = calloc(strlen(arg)+1, sizeof(char));
  strncpy(self->argv[self->argc], arg, strlen(arg));
  self->argc++;
}

int command_parse_options(Command *self) {

  int option, index;
  static struct option options[] = {
    {"debug",   required_argument, 0, 'd'},
    {"memory",  required_argument, 0, 'm'},
    {"bank",    required_argument, 0, 'b'},
    {"address", required_argument, 0, 'a'},
    {0, 0, 0, 0}
  };
  char *end;
  
  optind = 0;
  
  while(1) {

    option = getopt_long(self->argc, self->argv, "dm:b:a:", options, &index);
    
    if(option == -1)
      break;

    switch(option) {
    
    case 'd':
      debug = true;
      break;

    case 'm':
      self->memory = strtol(optarg, NULL, 0);
      break;

    case 'b':
      self->bank = strtol(optarg, NULL, 0);
      break;

    case 'a':
      self->start = strtol(optarg, NULL, 0);

      if ((end = strstr(optarg, "-")) != NULL) {
	self->end = strtol(end+1, NULL, 0);
      }

      if (!valid(self->start)) {
	fprintf(stderr, "c64: error: %s: start address out of range: 0x%04X\n",
		command_get_name(self), self->start);
	return false;
      }

      if(self->end != -1) {

	if (!valid(self->end)) {
	  fprintf(stderr, "c64: error: %s: end address out of range: 0x%04X\n",
		  command_get_name(self), self->end);
	  return false;
	}
	
	if (self->end < self->start) {
	  fprintf(stderr, "c64: error: %s: end address before start address: 0x%04X > 0x%04X\n",
		  command_get_name(self), self->end, self->start);
	  return false;
	}
	
	if (self->start == self->end) {
	  fprintf(stderr, "c64: error: %s: start address equals end address: 0x%04X == 0x%04X\n",
		  command_get_name(self), self->end, self->start);
	  return false;	
	}
      }
      break;      
    }
  }
  self->argc -= optind;
  
  int i;
  for(i=0; i<self->argc; i++) {
    free(self->argv[i]);
    self->argv[i] = self->argv[i+optind];
  }

  return true;
}

char* command_get_name(Command* self) {
  return id2str(self->id);
}

void command_print(Command* self) {
  if(debug) {
    printf("%s -m 0x%02X -b 0x%02X -a 0x%04X-0x%04X ", 
	   command_get_name(self), self->memory, self->bank, self->start, self->end);
    
    int i;
    for (i=0; i<self->argc; i++) {
      printf("%s ", self->argv[i]);
    }
    printf("\n");
  }
}  

int command_find_basic_program(Command* self) {

  int bstart = 0x0000;
  int bend   = 0x0000;
  unsigned char value;

  if(pp64_peek(0x37, 0x10, 0x002c, &value)) {
    bstart |= value;
    bstart <<= 8;
  } 
  else return false;

  if(pp64_peek(0x37, 0x10, 0x002b, &value)) {
    bstart |= value;
  } 
  else return false;

  if(pp64_peek(0x37, 0x10, 0x002e, &value)) {
    bend |= value;
    bend <<= 8;
  } 
  else return false;

  if(pp64_peek(0x37, 0x10, 0x002d, &value)) {
    bend |= value;
  } 
  else return false;

  if(bend != bstart + 2) {
    self->start = bstart;
    self->end = bend;
    return true;
  }

  return false;
}

int command_dispatch(Command* self) {

  switch(self->id) {

  case COMMAND_AUTO:
    if(command_auto(self))
      return true;
    break;

  case COMMAND_LOAD:
    if(!command_load(self))
      return false;
    break;

  case COMMAND_SAVE:
    if(!command_save(self))
      return false;
    break;

  case COMMAND_POKE:
    if(!command_poke(self))
      return false;
    break;

  case COMMAND_PEEK:
    if(!command_peek(self))
      return false;
    break;

  case COMMAND_JUMP:
    if(!command_jump(self))
      return false;
    break;

  case COMMAND_RUN:
    if(!command_run(self))
      return false;
    break;

  case COMMAND_RESET:
    if (!command_reset(self))
      return false;
    break;

  }
  return true;
}

int command_auto(Command* self) {

  if (self->argc == 0) {
    return false;
  }

  char *filename = self->argv[0];
  char *suffix = (filename + strlen(filename)-4);

  if (strncmp(suffix, ".prg", 4) != 0) {
    fprintf(stderr, "c64: error: autoload: not a .prg file: %s\n", filename);
    return false;
  }

  if(command_load(self)) {
    if(self->start == 0x0801)
      return command_run(self);
    else {
      return command_jump(self);      
    }
  }
  return false;
}

int command_load(Command* self) {
  
  FILE *file;
  struct stat st;
  long size;
  int loadAddress;
  char *suffix;
  char *data;

  if (self->argc == 0) {
    fprintf(stderr, "c64: error: load: no file specified\n");
    return false;
  }

  char *filename = self->argv[0];
  
  file = fopen(filename, "r");
  
  if (file == NULL) {
    fprintf(stderr, "c64: error: load: '%s': %s\n", filename, strerror(errno));
    return false;
  }
  stat(filename, &st);
  size = st.st_size;
  
  suffix = (filename + strlen(filename)-4);

  // get load address from .prg file
  if (strncmp(suffix, ".prg", 4) == 0) {
    fread(&loadAddress, sizeof(char), 2, file);
    size -= 2;
  }
  else {
    if (self->start == -1) {      
      fprintf(stderr, "c64link: error: load: not a .prg file and no start address specified\n");
      fclose(file);
      return false;
    }
  }
      
  if (self->start == -1)
    self->start = loadAddress & 0xffff;      

  if(self->end == -1) {
    self->end = self->start + size;
  }

  if (self->memory == 0xff) {
    
    if(self->end > 0xD000 && self->start < 0xE000)
      self->memory = 0x33; // write to ram below io by default
    else 
      self->memory = 0x37;    
  }

  if (self->bank == 0xff) {
    self->bank = 0x10;
  }

  data = (char*) calloc(size, sizeof(char));
  fread(data, sizeof(char), size, file);
  fclose(file);  

  command_print(self);

  if (!pp64_load(self->memory, self->bank, self->start, self->end, data, size)) {
    free(data);
    return false;
  }

  free(data);
  return true;
}

int command_save(Command* self) {
  
  FILE *file;
  char *suffix;
  int size;
  char *data;

  if (self->argc == 0) {
    fprintf(stderr, "c64: error: save: no file specified\n");
    return false;
  }

  char *filename = self->argv[0];

  if(self->start == -1) {
    if(!command_find_basic_program(self)) {
      fprintf(stderr, "c64link: error: no start address specified and no basic program in memory\n");
      return false;
    }
  }

  if(self->start == -1) {                   
    fprintf(stderr, "c64link: error: no start address specified\n");
    return false;
  }
  else {
    if(self->end == -1) {                   
      fprintf(stderr, "c64link: error: no end address specified\n");
      return false;
    }
  }

  size = self->end - self->start;

  suffix = (filename + strlen(filename)-4);

  if (self->memory == 0xff)
    self->memory = 0x37;    

  if (self->bank == 0xff)
    self->bank = 0x10;

  data = (char*) calloc(size, sizeof(char));

  file = fopen(filename, "w");

  if(file == NULL) {
    fprintf(stderr, "c64link: error: '%s': %s\n", filename, strerror(errno));
    free(data);
    return false;
  }

  command_print(self);

  if(!pp64_save(self->memory, self->bank, self->start, self->end, data, size)) {
    free(data);
    fclose(file);
    return false;
  }

  if (strncmp(suffix, ".prg", 4) == 0)
    fwrite(&self->start, sizeof(char), 2, file);
  
  fwrite(data, sizeof(char), size, file);
  fclose(file);

  free(data);    
  return true;
}

int command_poke(Command* self) {
  char *argument;
  int address;
  unsigned char value;
  
  if (self->argc == 0) {
    fprintf(stderr, "c64: error: poke: argument required\n");
    return false;
  }
  argument = self->argv[0];
  int comma = strcspn(argument, ",");

  if (comma == strlen(argument) || comma == strlen(argument)-1) {
    fprintf(stderr, "c64: syntax error: poke: expects <address>,<value>\n");
    return false;
  }
  
  char* addr = argument;
  char* val = argument + comma + 1;
  addr[comma] = '\0';

  address = strtol(addr, NULL, 0);
  value = strtol(val, NULL, 0);

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x10;

  command_print(self);

  return pp64_poke(self->memory, self->bank, address, value);
}

int command_peek(Command* self) {
  
  if (self->argc == 0) {
    fprintf(stderr, "c64: error: peek: no address specified\n");
    return false;
  }

  int address = strtol(self->argv[0], NULL, 0);
  unsigned char value;

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x10;

  command_print(self);

  if(!pp64_peek(self->memory, self->bank, address, &value)) {
    return false;
  }
  printf("%d\n", value);
  
  return true;
}

int command_jump(Command* self) {

  if (self->argc == 0) {
    fprintf(stderr, "c64: error: jump: no address specified\n");
    return false;
  }

  int address = strtol(self->argv[0], NULL, 0);

  if(address == 0) {
    if(self->start != -1) {
      address = self->start;
    }
    else {
      fprintf(stderr, "c64: error: jump: no address specified\n");
      return false;    
    }
  }

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x10;

  command_print(self);

  return pp64_jump(self->memory, self->bank, address);
}

int command_run(Command* self) {
  command_print(self);
  return pp64_run();
}

int command_reset(Command* self) {
  command_print(self);
  return pp64_reset();
}

int main(int argc, char **argv) {
  
  Command* command = NULL;   
  int id, i;
  commands = commands_new();

  for (i=0; i<argc; i++) {
    if ((id = str2id(argv[i])) != -1) {
      command = commands_add(commands, command_new(id)); 
    } else 
      command_append_argument(command, argv[i]);    
  }

  for (i=0; i<commands->count; i++) {
    command = commands->items[i];

    if (!command_parse_options(command))
      return EXIT_FAILURE;
   
    if (!command_dispatch(command))
      return EXIT_FAILURE;
  }   
  return EXIT_SUCCESS;
}

