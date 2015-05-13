#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "target.h"
#include "client.h"
#include "range.h"
#include "disk.h"
#include "util.h"
#include "xlink.h"

#define COMMAND_NONE       0x00
#define COMMAND_LOAD       0x01
#define COMMAND_SAVE       0x02
#define COMMAND_POKE       0x03
#define COMMAND_PEEK       0x04
#define COMMAND_JUMP       0x05
#define COMMAND_RUN        0x06
#define COMMAND_RESET      0x07
#define COMMAND_HELP       0x08
#define COMMAND_DOS        0x09
#define COMMAND_BACKUP     0x0a
#define COMMAND_RESTORE    0x0b
#define COMMAND_VERIFY     0x0c
#define COMMAND_STATUS     0x0d
#define COMMAND_READY      0x0e
#define COMMAND_PING       0x0f
#define COMMAND_BOOTLOADER 0x10
#define COMMAND_BENCHMARK  0x11
#define COMMAND_IDENTIFY   0x12
#define COMMAND_SERVER     0x13
#define COMMAND_RELOCATE   0x14
#define COMMAND_KERNAL     0x15
#define COMMAND_FILL       0x16

#define MODE_EXEC 0x00
#define MODE_HELP 0x01

int mode  = MODE_EXEC;

static struct option options[] = {
  {"help",    no_argument,       0, 'h'},
  {"verbose", no_argument,       0, 'v'},
  {"quiet",   no_argument,       0, 'q'},
  {"device",  required_argument, 0, 'd'},
  {"memory",  required_argument, 0, 'm'},
  {"bank",    required_argument, 0, 'b'},
  {"address", required_argument, 0, 'a'},
  {"skip",    required_argument, 0, 's'},
  {0, 0, 0, 0}
};

//------------------------------------------------------------------------------

char str2id(const char* arg) {
  if (strcmp(arg, "load"      ) == 0) return COMMAND_LOAD;
  if (strcmp(arg, "save"      ) == 0) return COMMAND_SAVE;
  if (strcmp(arg, "poke"      ) == 0) return COMMAND_POKE;
  if (strcmp(arg, "peek"      ) == 0) return COMMAND_PEEK;
  if (strcmp(arg, "jump"      ) == 0) return COMMAND_JUMP;
  if (strcmp(arg, "run"       ) == 0) return COMMAND_RUN;  
  if (strcmp(arg, "reset"     ) == 0) return COMMAND_RESET;  
  if (strcmp(arg, "help"      ) == 0) return COMMAND_HELP;  
  if (strcmp(arg, "backup"    ) == 0) return COMMAND_BACKUP;  
  if (strcmp(arg, "restore"   ) == 0) return COMMAND_RESTORE;  
  if (strcmp(arg, "verify"    ) == 0) return COMMAND_VERIFY;  
  if (strcmp(arg, "ready"     ) == 0) return COMMAND_READY;  
  if (strcmp(arg, "ping"      ) == 0) return COMMAND_PING;  
  if (strcmp(arg, "bootloader") == 0) return COMMAND_BOOTLOADER;  
  if (strcmp(arg, "benchmark" ) == 0) return COMMAND_BENCHMARK;  
  if (strcmp(arg, "identify"  ) == 0) return COMMAND_IDENTIFY;
  if (strcmp(arg, "server"    ) == 0) return COMMAND_SERVER;
  if (strcmp(arg, "relocate"  ) == 0) return COMMAND_RELOCATE;
  if (strcmp(arg, "kernal"    ) == 0) return COMMAND_KERNAL;      
  if (strcmp(arg, "fill"      ) == 0) return COMMAND_FILL;      

  if (strncmp(arg, "@", 1) == 0) {
    if(strlen(arg) == 1) {
      return COMMAND_STATUS;
    }
    else {
      return COMMAND_DOS;
    }
  }
  return COMMAND_NONE;
}

//------------------------------------------------------------------------------

char* id2str(const char id) {
  if (id == COMMAND_NONE)       return (char*) "main";
  if (id == COMMAND_LOAD)       return (char*) "load";
  if (id == COMMAND_SAVE)       return (char*) "save";
  if (id == COMMAND_POKE)       return (char*) "poke";
  if (id == COMMAND_PEEK)       return (char*) "peek";
  if (id == COMMAND_JUMP)       return (char*) "jump";
  if (id == COMMAND_RUN)        return (char*) "run";
  if (id == COMMAND_RESET)      return (char*) "reset";
  if (id == COMMAND_HELP)       return (char*) "help";
  if (id == COMMAND_DOS)        return (char*) "dos";
  if (id == COMMAND_BACKUP)     return (char*) "backup";
  if (id == COMMAND_RESTORE)    return (char*) "restore";
  if (id == COMMAND_VERIFY)     return (char*) "verify";
  if (id == COMMAND_STATUS)     return (char*) "status";  
  if (id == COMMAND_READY)      return (char*) "ready";  
  if (id == COMMAND_PING)       return (char*) "ping";  
  if (id == COMMAND_BOOTLOADER) return (char*) "bootloader";  
  if (id == COMMAND_BENCHMARK)  return (char*) "benchmark";  
  if (id == COMMAND_IDENTIFY)   return (char*) "identify";
  if (id == COMMAND_SERVER)     return (char*) "server";
  if (id == COMMAND_RELOCATE)   return (char*) "relocate";
  if (id == COMMAND_KERNAL)     return (char*) "kernal";      
  if (id == COMMAND_FILL)       return (char*) "fill";      
  return (char*) "unknown";
}

//------------------------------------------------------------------------------

int isCommand(const char *str) {
  return str2id(str) > COMMAND_NONE;
}

//------------------------------------------------------------------------------

int isOption(const char *str) {
  return str[0] == '-';
}

//------------------------------------------------------------------------------

int isOptarg(const char* option, const char* argument) {

  if (!isOption(option)) {
      return false;
  }

  for(int i=0; options[i].name != 0; i++) {
    
    if (!options[i].has_arg) {
      continue;
    }
      
    if(strlen(option) == 2) {
      if(option[1] == options[i].val) {
	return true;
      }
    }
    
    if(strlen(option) > 2) {
      if (option[2] == options[i].val) {
	return true;
      }
    }
  } 
  
  return false;
}

//------------------------------------------------------------------------------

int valid(int address) {
  return address >= 0x0000 && address <= 0x10000; 
}

//------------------------------------------------------------------------------

void screenOn(void) {
  xlink_poke(0x37, 0x00, 0xd011, 0x1b);
}

//------------------------------------------------------------------------------

void screenOff(void) {
  xlink_poke(0x37, 0x00, 0xd011, 0x0b);
}

//------------------------------------------------------------------------------

Commands* commands_new(int argc, char **argv) {

  Commands* commands = (Commands*) calloc(1, sizeof(Commands));
  commands->count = 0;
  commands->items = (Command**) calloc(1, sizeof(Command*));

  while(argc > 0) {
    commands_add(commands, command_new(&argc, &argv));
  }  

  return commands;
}

//------------------------------------------------------------------------------

Command* commands_add(Commands* self, Command* command) {
  self->items = (Command**) realloc(self->items, (self->count+1) * sizeof(Command*));
  self->items[self->count] = command;
  self->count++;
  return command;
}

//------------------------------------------------------------------------------

bool commands_each(Commands* self, bool (*func) (Command* command)) {
  bool result = true;

  for(int i=0; i<self->count; i++) {
    if(!(result = func(self->items[i]))) {
      break;
    }
  }
  return result;
}

//------------------------------------------------------------------------------

bool commands_execute(Commands* self) {
  return commands_each(self, &command_execute);
}

//------------------------------------------------------------------------------

void commands_print(Commands* self) {
  commands_each(self, &command_print);
}

//------------------------------------------------------------------------------

void commands_free(Commands* self) {

  for(int i=0; i<self->count; i++) {
    command_free(self->items[i]);
  }  
  free(self->items);
  free(self);
}

//------------------------------------------------------------------------------

Command* command_new(int *argc, char ***argv) {

  Command* command = (Command*) calloc(1, sizeof(Command));

  command->id        = COMMAND_NONE;
  command->name      = NULL;
  command->memory    = 0xff;
  command->bank      = 0xff;
  command->start     = -1;
  command->end       = -1;
  command->skip      = -1;
  command->argc      = 0;
  command->argv      = (char**) calloc(1, sizeof(char*));
  
  command_append_argument(command, (char*)"getopt");
  command_consume_arguments(command, argc, argv);

  return command;
}

void command_free(Command* self) {

  free(self->name);

  self->argc += self->offset;
  self->argv -= self->offset;

  for(int i=0; i<self->argc; i++) {
    free(self->argv[i]);
  }
  free(self->argv);
  free(self);
}

//------------------------------------------------------------------------------
int command_arity(Command* self) {

  if (self->id == COMMAND_NONE)       return -1;
  if (self->id == COMMAND_LOAD)       return 1;
  if (self->id == COMMAND_SAVE)       return 1;
  if (self->id == COMMAND_POKE)       return 1;
  if (self->id == COMMAND_PEEK)       return 1;
  if (self->id == COMMAND_JUMP)       return 1;
  if (self->id == COMMAND_RUN)        return 1;
  if (self->id == COMMAND_RESET)      return 0;
  if (self->id == COMMAND_HELP)       return 1;
  if (self->id == COMMAND_DOS)        return 0;
  if (self->id == COMMAND_BACKUP)     return 1;
  if (self->id == COMMAND_RESTORE)    return 1;
  if (self->id == COMMAND_VERIFY)     return 1;
  if (self->id == COMMAND_STATUS)     return 0;
  if (self->id == COMMAND_READY)      return 0;
  if (self->id == COMMAND_PING)       return 0;
  if (self->id == COMMAND_BOOTLOADER) return 0;
  if (self->id == COMMAND_BENCHMARK)  return 0;
  if (self->id == COMMAND_IDENTIFY)   return 0;
  if (self->id == COMMAND_SERVER)     return 1;
  if (self->id == COMMAND_RELOCATE)   return 1;
  if (self->id == COMMAND_KERNAL)     return 2;    
  if (self->id == COMMAND_FILL)       return 2;    
  return 0;

}
//------------------------------------------------------------------------------
void command_consume_arguments(Command *self, int *argc, char ***argv) {
  
  bool isFirst = true;

  int hasNext(void) {
    return (*argc) > 0;
  }

  void next(void) {
    (*argc)--; 
    (*argv)++;
    isFirst = false;
  }    

  char *current(void) {
    return (*argv)[0];
  }

  int hasPrevious(void) {
    return !isFirst;
  }

  char *previous(void) {
    return (*(*(argv)-1));
  }

  self->name = (char *) calloc(strlen(current())+1, sizeof(char));
  strncpy(self->name, current(), strlen(current()));

  self->id = str2id(self->name);

  if(isCommand(self->name)) {
    next();
  }

  int arity = command_arity(self);
  int consumed = 0;

  for(;hasNext();next()) {

    if(isCommand(current()) && !isOptarg(previous(), current())) {
      break;
    }
    
    if (consumed == arity && arity > 0) {
      if (hasPrevious() && !isOptarg(previous(), current())) {
        break;
      }
      else if (!isOption(current())) {
        break;
      }
    }

    command_append_argument(self, current());

    if (consumed < arity) {
      if (hasPrevious() && isOptarg(previous(), current())) {
        continue;
      }
      else if (isOption(current())) {
        continue;
      }
      consumed+=1;      
    }    
  }
}

//------------------------------------------------------------------------------

void command_append_argument(Command* self, char* arg) {
  self->argv = (char**) realloc(self->argv, (self->argc+1) * sizeof(char*));
  self->argv[self->argc] = (char*) calloc(strlen(arg)+1, sizeof(char));
  strncpy(self->argv[self->argc], arg, strlen(arg));
  self->argc++;
}

//------------------------------------------------------------------------------

bool command_parse_options(Command *self) {
  
  int option, index;
  char *end;
  
  optind = 0;
  
  while(1) {

    option = getopt_long(self->argc, self->argv, "hvqd:m:b:a:s:", options, &index);
    
    if(option == -1)
      break;

    switch(option) {

    case 'h':
      usage();
      break;

    case 'q':
      logger->set("NONE");
      break;

    case 'v':
      logger->set("ALL");
      break;

    case 'd':
      if (!xlink_set_device(optarg)) {
        return false; 
      }
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
        logger->error("start address out of range: 0x%04X", self->start);
        return false;
      }

      if(self->end != -1) {
        
        if (!valid(self->end)) {
          logger->error("end address out of range: 0x%04X", self->end);
          return false;
        }
	
        if (self->end < self->start) {
          logger->error("end address before start address: 0x%04X > 0x%04X", self->end, self->start);
          return false;
        }
	
        if (self->start == self->end) {
          logger->error("start address equals end address: 0x%04X == 0x%04X", self->end, self->start);
          return false;	
        }
      }
      break;

    case 's':
      self->skip = strtol(optarg, NULL, 0);
    }    
  }

  self->argc -= optind;
  self->argv += optind;
  self->offset = optind;
  return true;
}

//------------------------------------------------------------------------------

char* command_get_name(Command* self) {
  return id2str(self->id);
}

//------------------------------------------------------------------------------

bool command_print(Command* self) {

  char result[1024] = "";;
  bool print = false;

  if(strlen(xlink_get_device()) > 0) {
    sprintf(result, "-d %s ",  xlink_get_device());
    print = true;
  }
   
  if((unsigned char) self->memory != 0xff) {
    sprintf(result + strlen(result), "-m 0x%02X ", (unsigned char) self->memory);
    print = true;
  }

  if((unsigned char) self->bank != 0xff) {
    sprintf(result + strlen(result), "-b 0x%02X ", (unsigned char) self->bank);
    print = true;
  }

  if((unsigned short) self->start != 0xffff) {
    sprintf(result + strlen(result), "-a 0x%04X", (unsigned short) self->start);

    if((unsigned short) self->end != 0xffff) {
      sprintf(result + strlen(result), "-0x%04X", (unsigned short) self->end);
    }
    sprintf(result + strlen(result), " ");
    print = true;
  }  

  if((unsigned short) self->skip != 0xffff) {
    sprintf(result + strlen(result), "-s 0x%04X ", (unsigned short) self->skip);
    print = true;
  }

  int i;
  for (i=0; i<self->argc; i++) {
    sprintf(result + strlen(result), "%s ", self->argv[i]);
    print = true;
  }

  if (print) {
    logger->debug(result);
  }

  return true;
} 

//------------------------------------------------------------------------------

bool command_find_basic_program(Command* self) {

  int bstart = 0x0000;
  int bend   = 0x0000;
  unsigned char value;

  if(xlink_peek(0x37, 0x00, 0x002c, &value)) {
    bstart |= value;
    bstart <<= 8;
  } 
  else return false;

  if(xlink_peek(0x37, 0x00, 0x002b, &value)) {
    bstart |= value;
  } 
  else return false;

  if(xlink_peek(0x37, 0x00, 0x002e, &value)) {
    bend |= value;
    bend <<= 8;
  } 
  else return false;

  if(xlink_peek(0x37, 0x00, 0x002d, &value)) {
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

//------------------------------------------------------------------------------

bool command_none(Command* self) {

  StringList *arguments = stringlist_new();
  Commands *commands;
  bool result = true;

  command_print(self);
  
  if (self->argc > 0) {

    stringlist_append(arguments, "ready");

    for (int i=0; i<self->argc; i++) {

      if (access(self->argv[i], R_OK) == 0) {               
        stringlist_append(arguments, (i < self->argc-1) ? "load" : "run");      
        stringlist_append(arguments, self->argv[i]);      
      }
      else {
        logger->error("Unknown command: %s", self->argv[i]);
        result = false;
        goto done;
      }
    }
    
    commands = commands_new(arguments->size, arguments->strings);
    
    result = commands_execute(commands);
    
    commands_free(commands);
  }

 done:
  stringlist_free(arguments);
  return result;
}

//------------------------------------------------------------------------------

bool command_load(Command* self) {
  
  FILE *file;
  struct stat st;
  long size;
  int loadAddress;
  unsigned char *data;
  
  if (self->argc == 0) {
    logger->error("no file specified");
    return false;
  }

  char *filename = self->argv[0];
  
  file = fopen(filename, "rb");
  
  if (file == NULL) {
    logger->error("'%s': %s", filename, strerror(errno));
    return false;
  }
  stat(filename, &st);
  size = st.st_size;
  
  if (self->start == -1) {
    // no load address specified, assume PRG file
    fread(&loadAddress, sizeof(char), 2, file);
    self->start = loadAddress & 0xffff;      

    if (self->skip == -1)
      self->skip = 2;
  }
  
  if (self->skip == -1)
    self->skip = 0;

  size -= self->skip;

  if(self->end == -1) {
    self->end = self->start + size;
  }

  if(self->end - self->start < size) {
    size = self->end - self->start;
  }

  if (self->memory == 0xff) {

    Range* io = range_new(0xd000, 0xc000);
    Range* data = range_new(self->start, self->end);
    
    if(range_inside(data, io))
      self->memory = 0x33; // write to ram below io by default
    else 
      self->memory = 0x37;

    free(io);
    free(data);
  }

  if (self->bank == 0xff) {
    self->bank = 0x00;
  }

  data = (unsigned char*) calloc(size, sizeof(unsigned char));
  
  fseek(file, self->skip, SEEK_SET);
  fread(data, sizeof(unsigned char), size, file);
  fclose(file);  

  command_print(self);

  if(!command_server_usable_after_possible_relocation(self)) {
    free(data);
    return false;
  }      

  if (!xlink_load(self->memory, self->bank, self->start, data, size)) {
    free(data);
    return false;
  }

  free(data);
  return true;
}

//------------------------------------------------------------------------------

bool command_save(Command* self) {
  
  FILE *file;
  char *suffix;
  int size;
  unsigned char *data;

  if (self->argc == 0) {
    logger->error("no file specified");
    return false;
  }

  char *filename = self->argv[0];

  if(self->start == -1) {
    if(!command_find_basic_program(self)) {
      logger->error("no start address specified and no basic program in memory");
      return false;
    }
  }

  if(self->start == -1) {                   
    logger->error("no start address specified");
    return false;
  }
  else {
    if(self->end == -1) {                   
      logger->error("no end address specified");
      return false;
    }
  }

  size = self->end - self->start;

  suffix = (filename + strlen(filename)-4);

  if (self->memory == 0xff)
    self->memory = 0x37;    

  if (self->bank == 0xff)
    self->bank = 0x00;

  data = (unsigned char*) calloc(size, sizeof(unsigned char));

  file = fopen(filename, "wb");

  if(file == NULL) {
    logger->error("'%s': %s", filename, strerror(errno));
    free(data);
    return false;
  }

  command_print(self);

  if(!xlink_save(self->memory, self->bank, self->start, data, size)) {
    free(data);
    fclose(file);
    return false;
  }

  if (strncasecmp(suffix, ".prg", 4) == 0)
    fwrite(&self->start, sizeof(unsigned char), 2, file);
  
  fwrite(data, sizeof(unsigned char), size, file);
  fclose(file);

  free(data);    
  return true;
}

//------------------------------------------------------------------------------

bool command_poke(Command* self) {
  char *argument;
  unsigned char value;
  
  if (self->argc == 0) {
    logger->error("argument required");
    return false;
  }
  argument = self->argv[0];
  unsigned int comma = strcspn(argument, ",");

  if (comma == strlen(argument) || comma == strlen(argument)-1) {
    logger->error("expects <address>,<value>");
    return false;
  }
  
  char* addr = argument;
  char* val = argument + comma + 1;
  addr[comma] = '\0';

  self->start = strtol(addr, NULL, 0);
  value = strtol(val, NULL, 0);

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x00;

  self->end = self->start;
  
  command_print(self);

  if(!command_server_usable_after_possible_relocation(self)) {
    return false;
  }      

  return xlink_poke(self->memory, self->bank, self->start, value);
}

//------------------------------------------------------------------------------

bool command_peek(Command* self) {
  
  if (self->argc == 0) {
    logger->error("no address specified");
    return false;
  }

  int address = strtol(self->argv[0], NULL, 0);
  unsigned char value;

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x00;

  command_print(self);

  if(!xlink_peek(self->memory, self->bank, address, &value)) {
    return false;
  }
  printf("%d\n", value);
  
  return true;
}

//------------------------------------------------------------------------------

bool command_fill(Command* self) {

  bool result = false;

  if (self->argc == 0) {
    logger->error("no arguments given");
    goto done;
  }

  if(self->argc == 1) {
    logger->error("no value specified");
    goto done;
  }

  Range *range = range_parse(self->argv[0]);

  if(!range_ends(range)) {
    range->end = 0x10000;
  }

  if(!range_valid(range)) {
    logger->error("invalid memory range: $%04X-$%04X", range->start, range->end);
    free(range);
    goto done;
  }
  
  unsigned char value = (unsigned char) strtol(self->argv[1], NULL, 0);

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x00;

  int size = range_size(range);
  
  self->start = range->start;
  self->end = range->end;
  
  free(range);
  
  command_print(self);

  if(!command_server_usable_after_possible_relocation(self)) {
    goto done;
  }      

  result = xlink_fill(self->memory, self->bank, self->start, value, size);  
  
 done:
  return result;
}

//------------------------------------------------------------------------------

bool command_jump(Command* self) {

  if (self->argc == 0) {
    logger->error("no address specified");
    return false;
  }

  int address = strtol(self->argv[0], NULL, 0);

  if(address == 0) {
    if(self->start != -1) {
      address = self->start;
    }
    else {
      logger->error("no address specified");
      return false;    
    }
  }

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x00;

  command_print(self);

  return xlink_jump(self->memory, self->bank, address);
}

//------------------------------------------------------------------------------

bool command_run(Command* self) {
  bool result = false;

  if(self->argc == 1) {

    logger->suspend();
    if(!(result = command_load(self))) {
      logger->resume();
      return result;
    }
    logger->resume();

    if (self->start != 0x0801) {
      
      if (self->memory == 0xff)
        self->memory = 0x37;
      
      if (self->bank == 0xff)
        self->bank = 0x00;
      
      command_print(self);

      return xlink_jump(self->memory, self->bank, self->start);
    }
  }
  command_print(self);
  return xlink_run();
}

//------------------------------------------------------------------------------

extern bool xlink_relocate(unsigned short address);

bool command_server_usable_after_possible_relocation(Command* self) {

  unsigned short newServerAddress;  
  xlink_server_info_t server;
  
  if(xlink_identify(&server)) {

    if(command_requires_server_relocation(self, &server)) {

      if(!command_server_relocation_possible(self, &server, &newServerAddress)) {
        logger->error("impossible to relocate ram-based server: out of memory");
        return false;
      }

      logger->debug("relocating server to $%04X", newServerAddress);

      if(!xlink_relocate(newServerAddress)) {
        logger->error("failed to relocate ram-based server: %s", xlink_error->message);
        return false;
      }
    }
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool command_requires_server_relocation(Command* self, xlink_server_info_t* server) {

  bool result = false;

  if(server->type == XLINK_SERVER_TYPE_ROM) {
    return false;
  }

  Range* data = range_new(self->start, self->end);
  Range* code = range_new(server->start, server->end);

  if(range_overlaps(data, code)) {
    
    logger->debug("relocation required: data ($%04X-$%04X) overlaps server ($%04X-$%04X)",
		  self->start, self->end, server->start, server->end);
    
    result = true;
  }
  free(data);
  free(code);
  return result;  
}

//------------------------------------------------------------------------------

bool command_server_relocation_possible(Command* self, xlink_server_info_t* server, unsigned short* address) {

  bool result = true;
  
  Range* screen = range_new(0x0400, 0x07e7);  
  Range* lower  = range_new(0x0801, server->memtop);
  Range* upper  = range_new(0xc000, 0xd000);
  Range* all    = range_new(screen->start, upper->end);

  Range* data = range_new(self->start, self->end);
  Range* code = range_new(server->start, server->end);
  
  // first check if the data already covers the complete range of possible memory areas

  if(range_inside(all, data)) {
    result = false;
    goto done;
  }
  
  // else try to relocate server as close as possible to...

  // ...the end of the upper memory area ($c000-$d000)
  
  code->start = upper->end - server->length;
  code->end = upper->end;
  
  while(range_inside(code, upper)) {

    if(range_overlaps(code, data)) {
      range_move(code, -1);
    }
    else {
      (*address) = code->start;
      goto done;
    }
  }

  // ...the end of the lower memory area ($0801-$8000 or $0801-$a000)
  
  code->start = lower->end - server->length;
  code->end = lower->end;

  while(range_inside(code, lower)) {
    
    if(range_overlaps(code, data)) {
      range_move(code, -1);
    }
    else {
      (*address) = code->start;
      goto done;
    }
  }

  // ...the end of the default screen memory area ($0400-$07e7)

  code->start = screen->end - server->length;
  code->end = screen->end;

  while(range_inside(code, screen)) {
    
    if(range_overlaps(code, data)) {
      range_move(code, -1);
    }
    else {
      (*address) = code->start;
      goto done;
    }
  }
  
  result = false;
  
 done:  
  free(code);
  free(data);
  free(upper);
  free(lower);
  free(screen);
  free(all);
  return result;  
}

//------------------------------------------------------------------------------

bool command_relocate(Command *self) {

  bool result = false;
  
  xlink_server_info_t server;

  if(self->argc != 1) {
    logger->error("no relocation address specified");
    return false;
  }
  
  if(!xlink_identify(&server)) {
    logger->error("failed to identify server");
    return false;
  }

  if(server.type == XLINK_SERVER_TYPE_ROM) {
    logger->info("identified ROM-based server (no relocation required)");
    return true;
  }

  unsigned short address = strtol(self->argv[0], NULL, 0);
  
  Range* lorom = range_new(server.memtop, 0xc000); 
  Range* hirom = range_new(0xe000, 0x10000);
  Range* io    = range_new(0xd000, 0xe000);    
  Range* code  = range_new(address, address + server.length);

  if(!range_valid(code)) {
    logger->error("cannot relocate server to $%04X-$%04X: invalid memory range",
		  code->start, code->end);
    goto done;
  }
  
  if(server.type == XLINK_SERVER_TYPE_RAM) {
   
    if(range_inside(code, lorom)) {
      logger->error("cannot relocate server to $%04X-$%04X: range occupies lower rom area $%04X-$%04X",
		    code->start, code->end, lorom->start, lorom->end);
      goto done;
    }

    if(range_inside(code, hirom)) {
      logger->error("cannot relocate server to $%04X-$%04X: range occupies upper rom area $%04X-$%04X",
		    code->start, code->end, hirom->start, hirom->end);
      goto done;
    }

    if(range_inside(code, io)) {
      logger->error("cannot relocate server to $%04X-$%04X: range occupies io area $%04X-$%04X",
		    code->start, code->end, io->start, io->end);
      goto done;
    }

    result = xlink_relocate(address);
    goto done;
  }

  logger->error("unknown server type: %d", server.type);
  
 done:
    free(lorom);
    free(hirom);
    free(io);
    free(code);

  return result;
}

//------------------------------------------------------------------------------

bool command_reset(Command* self) {
  command_print(self);
  return xlink_reset();
}

//------------------------------------------------------------------------------

extern bool xlink_bootloader(void);

int command_bootloader(Command *self) {
  command_print(self);
  return xlink_bootloader();
}

//------------------------------------------------------------------------------

bool command_benchmark(Command* self) {

  command_print(self);

  Watch* watch = watch_new();
  bool result = false;

  xlink_server_info_t server;
  
  unsigned char payload[0x6000];
  unsigned char roundtrip[sizeof(payload)];
    
  int start = 0x1000;
    
  if (!xlink_ping()) {
    logger->error("no response from server");
    goto done;
  }

  if(xlink_identify(&server)) {
    if(server.type == XLINK_SERVER_TYPE_RAM) {
      xlink_relocate(0xc000);
    }
  }

  logger->info("sending %d bytes...", sizeof(payload));
    
  watch_start(watch);
  
  if(!xlink_load(0x37, 0x00, start, payload, sizeof(payload))) goto done;
  
  float seconds = (watch_elapsed(watch) / 1000.0);
  float kbs = sizeof(payload)/seconds/1024;
    
  logger->info("%.2f seconds at %.2f kb/s", seconds, kbs);       
    
  logger->info("receiving %d bytes...", sizeof(payload));
    
  watch_start(watch);
    
  if(!xlink_save(0x37, 0x00, start, roundtrip, sizeof(roundtrip))) goto done;
  
  seconds = (watch_elapsed(watch) / 1000.0);
  kbs = sizeof(payload)/seconds/1024;
    
  logger->info("%.2f seconds at %.2f kb/s", seconds, kbs);
    
  logger->info("verifying...");
  
  for(int i=0; i<sizeof(payload); i++) {
    if(payload[i] != roundtrip[i]) {
      logger->error("roundtrip error at byte %d: %d != %d", i, payload[i], roundtrip[i]);
      result = false;
      goto done;
    }
  }
  logger->info("completed successfully");
  
  result = true;
  
 done:
  watch_free(watch);
  return result;
}

//------------------------------------------------------------------------------

bool command_identify(Command *self) {

  xlink_server_info_t server;
  
  if(xlink_identify(&server)) {

    printf("%s %d.%d %s %s $%04X-$%04X\n",
           server.id,
           (server.version & 0xf0) >> 4, server.version & 0x0f,
           server.machine == XLINK_MACHINE_C64 ? "C64" : (XLINK_MACHINE_C128 ? "C128" : "Unknown"),
           server.type == XLINK_SERVER_TYPE_RAM ? "RAM" : "ROM",
           server.start, server.end);

    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

extern unsigned char* xlink_server(unsigned short address, int *size);
extern unsigned char* xlink_server_basic(int *size);

bool command_server(Command *self) {

  bool result = false;

  FILE *file;
  int size;
  unsigned char *data;

  if (self->argc == 0) {
    logger->error("no file specified");
    return false;
  }

  if (self->start == -1) {
    self->start = 0x0801;
  }

  command_print(self);

  if(self->start == 0x0801) {
    data = xlink_server_basic(&size);
  } else {
    data = xlink_server(self->start, &size);

    if(data == NULL) {
      return false;
    }    
  }

  if ((file = fopen(self->argv[0], "wb")) == NULL) {
    logger->error("couldn't open %s for writing: %s", strerror(errno));
    goto done;
  }

  fwrite(data, sizeof(unsigned char), size, file);
  fclose(file);

  logger->info("wrote %s (%d bytes)", self->argv[0], size);

  result = true;

 done:
  free(data);
  return result;
}

//------------------------------------------------------------------------------

extern void xlink_kernal(unsigned char* image);

bool command_kernal(Command *self) {

  bool result = false;
  struct stat st;
  FILE *file;
  
  if(self->argc < 1) {
    logger->error("no input file specified");
    goto done;
  }

  if(self->argc < 2) {
    logger->error("no output file specified");
    goto done;
  }

  char *inputfile = self->argv[0];
  char *outputfile = self->argv[1];

  if(stat(inputfile, &st) == -1) {
    logger->error("%s: %s", inputfile, strerror(errno));
    goto done;
  }

  if(st.st_size != 0x2000) {
    logger->error("input file: size must be exactly %d bytes (%s: %d bytes)",
		  0x2000, inputfile, st.st_size);
    goto done;
  }

  if((file = fopen(inputfile, "rb")) == NULL) {
    logger->error("%s: %s\n", inputfile, strerror(errno));
    goto done;
  }

  unsigned char image[0x2000];  
  fread(image, sizeof(unsigned char), 0x2000, file);
  fclose(file);

  xlink_kernal(image);

  if((file = fopen(outputfile, "wb")) == NULL) {
    logger->error("%s: %s\n", outputfile, strerror(errno));
    goto done;
  }
  fwrite(image, sizeof(unsigned char), 0x2000, file);
  fclose(file);

  logger->info("patched %s", outputfile);
  
  result = true;
    
 done:
  return result;
}

//------------------------------------------------------------------------------

bool command_help(Command *self) {

  if (self->argc > 0) {
    logger->error("unknown command: %s", self->argv[0]);
    return false;
  }

  mode = MODE_HELP;
  return true;
}

//------------------------------------------------------------------------------

extern bool xlink_drive_status(char* status);
extern bool xlink_dos(char* cmd);
extern bool xlink_sector_read(unsigned char track, unsigned char sector, unsigned char* data);
extern bool xlink_sector_write(unsigned char track, unsigned char sector, unsigned char* data);

bool command_status(Command* self) {

  char *status = (char*) calloc(sizeof(unsigned char), 256);
  int result = false;
  
  command_print(self);

  if(xlink_drive_status(status)) {
    printf("%s\n", status);
    result = true;
  }

  free(status);
  return result;
}

//------------------------------------------------------------------------------

bool command_dos(Command *self) {

  command_print(self);

  if (xlink_dos(self->name+1)) {
    return command_status(self);
  }
  return false;
}

//------------------------------------------------------------------------------

static bool read_sector(Sector *sector) {
  printf("\rreading track %02d, sector %02d", sector->track, sector->number); fflush(stdout);    
  
  return xlink_sector_read(sector->track, sector->number, sector->bytes); 
}

//------------------------------------------------------------------------------

bool command_backup(Command *self) {
    
  bool result = true;
  Disk* disk;

  if (self->argc == 0) {
    logger->error("no file specified");
    return false;
  }

  command_print(self);

  char *filename = self->argv[0];

  screenOff();

  disk = disk_new(35);
  if(disk_each_sector(disk, &read_sector)) {
    disk_save(disk, filename);
  }

  screenOn();
  printf("\n");

  disk_free(disk);
  return result;
}

//------------------------------------------------------------------------------

static bool write_sector(Sector *sector) {

  printf("\rwriting track %02d, sector %02d", sector->track, sector->number);
  fflush(stdout);
  
  return xlink_sector_write(sector->track, sector->number, sector->bytes); 
}

//------------------------------------------------------------------------------

bool command_restore(Command *self) {

  bool result = true;
  
  if (self->argc == 0) {
    logger->error("no file specified");
    return false;
  }

  char *filename = self->argv[0];
  Disk* disk = disk_load(filename);

  if(disk == NULL) {
    return false;
  }

  int size = 2+16+1+2+1; 

  char *format_disk = (char *) calloc(size, sizeof(char));
  snprintf(format_disk, size, "N:%s,%s", disk->name, disk->id);

  if(disk->size > 35) {
    logger->error("no support for disks > 35 tracks\n");
    result = false;
    goto done;
  }

  command_print(self);

  printf("formatting disk: \"%s,%s\"...", disk->name, disk->id); fflush(stdout);

  if(!(xlink_dos(format_disk) && xlink_dos("I"))) {
    printf("FAILED\n");
    result = false;
    goto done;
  }
  printf("OK\n");

  screenOff();

  disk_each_sector(disk, &write_sector);
  xlink_dos("I");

  screenOn();
  printf("\n");

 done:
  free(format_disk);
  disk_free(disk);
  return result;
}

//------------------------------------------------------------------------------

static bool verify_sector(Sector* expected) {
  
  Sector* actual = sector_new(expected->track, expected->number);
  int result = false;
  
  printf("\rverifying track %02d, sector %02d...", 
	 actual->track, actual->number); fflush(stdout);
  
  if(!xlink_sector_read(actual->track, actual->number, actual->bytes)) {
    goto done;
  }
  result = sector_equals(expected, actual);
  
 done:
  sector_free(actual);
  return result;
}

//------------------------------------------------------------------------------

static bool verify_sector_skipping_track_18(Sector* expected) {
  
  if(expected->track == 18) 
    return true;
  
  return verify_sector(expected);
}

//------------------------------------------------------------------------------

bool command_verify(Command *self) {

  Disk* disk;
  bool result = true;

  if (self->argc == 0) {
    logger->error("no file specified");
    return false;
  }
  
  command_print(self);

  if ((disk = disk_load(self->argv[0])) == NULL) {
    return false;
  }
  screenOff();
  
  // verify track 18 first
  result = track_each_sector(disk->tracks[17], &verify_sector);
  
  if (result) // verify other tracks
    result = disk_each_sector(disk, &verify_sector_skipping_track_18);
  
  screenOn();

  printf("%s\n", result ? "OK" : "FAILED");

  disk_free(disk);
  return result;
}

//------------------------------------------------------------------------------

bool command_ready(Command* self) {

  command_print(self);

  if (!xlink_ready()) {
    logger->error("no response from C64");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------

bool command_ping(Command* self) {
  command_print(self);
  
  Watch *watch = watch_new();

  bool response = xlink_ping();

  if (response) {
    logger->info("received reply after %.0fms", watch_elapsed(watch));
  } 
  else {
    logger->info("no reply after %.0fms", watch_elapsed(watch));
  }
  watch_free(watch);
  return response;
}

//------------------------------------------------------------------------------

bool command_execute(Command* self) {

  bool result = false;

  if(mode == MODE_HELP) {
    return help(self->id);
  }

  logger->enter(command_get_name(self));

  if(!(result = command_parse_options(self))) {
    logger->leave();
    return result;
  }

  switch(self->id) {

  case COMMAND_NONE       : result = command_none(self);       break;
  case COMMAND_LOAD       : result = command_load(self);       break;
  case COMMAND_SAVE       : result = command_save(self);       break;
  case COMMAND_POKE       : result = command_poke(self);       break;
  case COMMAND_PEEK       : result = command_peek(self);       break;
  case COMMAND_JUMP       : result = command_jump(self);       break;
  case COMMAND_RUN        : result = command_run(self);        break;
  case COMMAND_RESET      : result = command_reset(self);      break;
  case COMMAND_HELP       : result = command_help(self);       break;
  case COMMAND_DOS        : result = command_dos(self);        break;
  case COMMAND_BACKUP     : result = command_backup(self);     break;
  case COMMAND_RESTORE    : result = command_restore(self);    break;
  case COMMAND_VERIFY     : result = command_verify(self);     break;
  case COMMAND_STATUS     : result = command_status(self);     break;
  case COMMAND_READY      : result = command_ready(self);      break;
  case COMMAND_PING       : result = command_ping(self);       break;
  case COMMAND_BOOTLOADER : result = command_bootloader(self); break;
  case COMMAND_BENCHMARK  : result = command_benchmark(self);  break;
  case COMMAND_IDENTIFY   : result = command_identify(self);   break;
  case COMMAND_SERVER     : result = command_server(self);     break;
  case COMMAND_RELOCATE   : result = command_relocate(self);   break;
  case COMMAND_KERNAL     : result = command_kernal(self);     break;            
  case COMMAND_FILL       : result = command_fill(self);       break;            
  }
  
  logger->leave();

  return result;
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {

  Commands *commands;
  int result;

  logger->set("INFO");
  logger->enter(argv[0]);

  argc--; argv++;

  if (argc == 0) {
    usage();
    return EXIT_FAILURE;
  }
  
  if(argc == 1) {
    if (strcmp(argv[0], "help") == 0) {
      usage();
      return EXIT_SUCCESS;
    } 
  }

  commands = commands_new(argc, argv);

  result = commands_execute(commands) ? EXIT_SUCCESS : EXIT_FAILURE;

  commands_free(commands);

  logger->leave();

  return result;
}

//------------------------------------------------------------------------------

void version(void) {
  printf("xlink %.1f Copyright (C) 2015 Henning Bekel <h.bekel@googlemail.com>\n", CLIENT_VERSION);
}

//------------------------------------------------------------------------------

void usage(void) {
  version();
  printf("\n");
  printf("Usage: xlink [<opts>] [<command> [<opts>] [<arguments>]]...\n");
  printf("\n");
  printf("Options:\n");
  printf("         -h, --help                    : show this help\n");
  printf("         -q, --quiet                   : show errors only\n");
  printf("         -v, --verbose                 : show verbose debug output\n");
#if linux
  printf("         -d, --device <path>           : ");
  printf("transfer device (default: /dev/xlink)\n");
#elif windows
  printf("         -d, --device <port or \"usb\">  : ");
  printf("transfer device (default: \"usb\")\n");
#endif
  printf("         -a, --address <start>[-<end>] : C64 address/range (default: autodetect)\n");
  printf("         -s, --skip <n>                : Skip n bytes of file\n");
  printf("         -m, --memory                  : C64 memory config (default: 0x37)\n");
  printf("         -b, --bank                    : C64 memory bank (unused)\n");
  printf("\n");
  printf("Commands:\n");
  printf("          help  [<command>]            : show detailed help for command\n");
  printf("\n");
  printf("          kernal <infile> <outfile>    : patch kernal image to include server\n");
  printf("          server [-a<addr>] <file>     : create server and save to file\n");
  printf("          relocate <addr>              : relocate running server\n");
  printf("\n");  
  printf("          reset                        : reset C64 (if supported by hardware)\n");
  printf("          ready                        : try to make sure the server is ready\n");
  printf("          ping                         : check if the server is available\n");
  printf("          identify                     : identify remote server and machine\n");
  printf("\n");
  printf("          load  [<opts>] <file>        : load file into C64 memory\n");
  printf("          save  [<opts>] <file>        : save C64 memory to file\n");
  printf("          poke  [<opts>] <addr>,<val>  : poke value into C64 memory\n");
  printf("          peek  [<opts>] <addr>        : read value from C64 memory\n");
  printf("          fill  <range>  <val>         : fill memory range with value\n");
  printf("          jump  [<opts>] <addr>        : jump to specified address\n");
  printf("          run   [<opts>] [<file>]      : run program, optionally load it before\n");
  printf("          <file>...                    : load file(s) and run last file\n");
  printf("\n");
  printf("          @[<dos-command>]             : read drive status or send dos command\n");    
  printf("          backup <file>                : backup disk to d64 file\n");
  printf("          restore <file>               : restore d64 file to disk\n");
  printf("          verify <file>                : verify disk against d64 file\n");
  printf("\n");
  printf("          benchmark                    : test/measure transfer speed\n");
  printf("          bootloader                   : enter dfu-bootloader (at90usb162)\n");  
  printf("\n");
}

//------------------------------------------------------------------------------

bool help(int id) {

  switch(id) {
  case COMMAND_NONE:
    usage();
    break;

  case COMMAND_HELP:
    printf("Usage: help <command>\n");
    printf("\n");
    printf("Show detailed help for <command>\n");
    printf("\n");
    break;
    
  case COMMAND_LOAD:
    printf("Usage: load [--address <start>[-<end>] [--memory <mem>] [--skip <n>] <file>\n");
    printf("\n");
    printf("Load the specified file into C64 memory\n");
    printf("\n");
    printf("If no start address is given it is assumed that the file is a PRG\n");
    printf("file and that its first two bytes contain the start address in\n");
    printf("little-endian order.\n");
    printf("\n");    
    printf("Otherwise, if a start address is given it is assumed that the\n");
    printf("file is a plain binary file that does not contain a start\n");
    printf("address. In this case the entire file is loaded to the specified\n");
    printf("address. The --skip option may be used to skip an arbitrary\n");
    printf("amount of bytes at the beginning of the file.\n");
    printf("\n");    
    printf("If an additional end address is specified, transfer will end as\n");
    printf("soon as the end address or the end of the file is reached,\n");
    printf("whichever comes first.\n");    
    printf("\n");
    printf("If a memory config is specified, it is poked to $01 prior to writing\n");
    printf("the transfered value to C64 memory. The memory setting defaults to\n");
    printf("0x37, which is the default setting in direct mode. If the file\n");
    printf("overlaps the io area ($d000-$dfff) then the default memory config will\n");
    printf("be changed to 0x33, so that data will always be loaded into the RAM\n");
    printf("residing below the io area. This is a safety measure preventing\n");
    printf("possible damage to either the PC's parallel port or the C64's CIA2.\n");
    printf("In order to load data directly into the io area the memory config\n");
    printf("needs to be set to 0x37 explicitly.\n");
    printf("\n");
    printf("If the server on the C64 is running from RAM and is located\n");
    printf("in the same memory area as the data to be loaded then an attempt\n");
    printf("is made to relocate the server to a different location beforehand.\n");
    printf("\n"); 
    break;

  case COMMAND_SAVE:
    printf("Usage: save [--address <start>-<end>] [--memory <mem>] file\n");
    printf("\n");
    printf("Save the specified C64 memory area to a file.\n");
    printf("\n");
    printf("If the destination filename ends with .prg then the destination file\n");
    printf("will be prefixed with the supplied start address. If no address range\n");
    printf("is specified, then the basic program currently residing in C64 memory\n");
    printf("will be saved.\n");
    printf("\n");
    printf("If a memory config is specified, it will be poked to $01 prior to\n");
    printf("reading the value to be transfered from C64 memory. The default value\n");
    printf("is 0x37.\n");
    printf("\n");
    break;

  case COMMAND_POKE:
    printf("Usage: poke [--memory <mem>] <address>,<byte>\n");
    printf("\n");
    printf("Poke the specified byte to the specified address. \n");
    printf("\n");
    printf("If no memory config is specified, then the default memory config 0x37\n");
    printf("will be used, so that values poked to the io area $d000-$dfff will\n");
    printf("have the expected effect.\n");
    printf("\n");
    break;

  case COMMAND_PEEK:
    printf("Usage: peek [--memory <mem>] <address>\n");
    printf("\n");
    printf("Read the byte at the specified C64 memory address and print it on\n");
    printf("standard output.\n");
    printf("\n");
    printf("If no memory config is specified, the default value 0x37 will be used.\n");
    printf("\n");
    break;

  case COMMAND_JUMP:
   printf("Usage: jump <address>\n");
   printf("\n");
   printf("Jump to the specified address in C64 memory. The stack pointer,\n");
   printf("processor flags and registers will be reset prior to jumping.\n");
   printf("\n");
   break;

  case COMMAND_RUN:
    printf("Usage: run [<file>]\n");
    printf("\n");
    printf("Without argument, RUN the currently loaded basic program. With a file argument\n");
    printf("specified, load the file beforehand. If the file loads to 0x0801, assume its a\n");
    printf("basic program and RUN it, else assume it's an ml program and jump to the\n");
    printf("address the file loaded to.\n");
    printf("\n");
    break;

  case COMMAND_RESET:
    printf("Usage: reset\n");
    printf("\n");
    printf("Reset the C64. Works without the server actually running on the C64 side.\n");
    printf("\n");
    break;

  case COMMAND_DOS:
  case COMMAND_STATUS:
    printf("Usage: @[command]\n");
    printf("\n");
    printf("Send the specified DOS command to the drive and report the resulting\n");
    printf("drive status. If no command is specified, report drive status only.\n");
    printf("\n");
    break;

  case COMMAND_BACKUP:
    printf("Usage: backup <file>.d64\n");
    printf("\n");
    printf("Backup a disk to a d64 file. Reads 35 tracks from disk and saves them to\n");
    printf("the specified file. Note that no error checking is performed and no error\n");
    printf("information is appended to the d64 file.\n");
    printf("\n");
    break;

  case COMMAND_RESTORE:
    printf("Usage: restore <file>.d64\n");
    printf("\n");
    printf("Write a 35 track d64 file to disk. The data is written as is, i.e. without\n");
    printf("interpreting any error information that may be included in the d64 file.\n");
    printf("\n");
    break;

  case COMMAND_VERIFY:
    printf("Usage: verify <file>.d64\n");
    printf("\n");
    printf("Verify disk against d64 file. Reads 35 tracks from disk and compares the data\n");
    printf("against the specified d64 file. Track 18 is verified first, then the remaining\n");
    printf("tracks are verified in order.\n");
    printf("\n");
    break;

  case COMMAND_READY:
    printf("Usage: ready [<commands>...]\n");
    printf("\n");
    printf("Makes sure that the server is ready. First the server is pinged. If it doesn't\n");
    printf("respond immediately, the C64 is reset. If the server responds to another ping\n");
    printf("within three seconds, then the remaining commands (if any) are executed.\n");
    printf("\n");
    printf("This command requires the server to be installed permanently so that it is\n");
    printf("available after reset.\n");
    printf("\n");
    break;

  case COMMAND_PING:
    printf("Usage: ping\n");
    printf("\n");
    printf("Ping the server, exit successfully if the server responds.\n");
    printf("\n");
    break;

  case COMMAND_BOOTLOADER:
    printf("Usage: bootloader\n");
    printf("\n");
    printf("Prepare USB devices for firmware updates. Enters the atmel dfu-bootloader.\n");
    printf("\n");
    break;

  case COMMAND_BENCHMARK:
    printf("Usage: benchmark\n");
    printf("\n");
    printf("Write 32k of random data into C64 memory, then read it back and compare it\n");
    printf("to the original data while measuring the achieved transfer rates.\n");
    printf("\n");
    break;

  case COMMAND_IDENTIFY:
    printf("Usage: identify\n");
    printf("\n");
    printf("Query information about the remote server. Reports machine type, server\n");
    printf("version, server type (RAM- or ROM-based), memory location and size.\n");
    printf("Additionally reports the top of the lower memory area ($A000 or $8000).\n");    
    printf("\n");
    break;

  case COMMAND_SERVER:
    printf("Usage: server [--address <address>] <file>\n");
    printf("\n");
    printf("Write a ram-based C64 server programm to file. Use address to specify the\n");
    printf("start address for the server code. If the address is $0801 the server can\n");
    printf("be started with RUN. This is the default if no address is specified.\n");
    printf("\n");
    break;

  case COMMAND_RELOCATE:
    printf("Usage: relocate <address>\n");
    printf("\n");
    printf("Relocate the currently running ram-based server to the specified address.\n");
    printf("Note that the server cannot be relocated to areas occupied by ROM or IO.\n");
    printf("\n");
    break;

  case COMMAND_KERNAL:
    printf("Usage: kernal <infile> <outfile>\n");
    printf("\n");
    printf("Patch the kernal image supplied via <infile> to include an xlink server and\n");
    printf("write the results to <outfile>. Note that the resulting kernal will no longer\n");
    printf("support tape IO.\n");
    printf("\n");
    break;

  case COMMAND_FILL:
    printf("Usage: fill --address <start>-<end> <value>\n");
    printf("\n");
    printf("Fill the specified memory area with <value>. The memory config defaults to 0x37.\n");
    printf("\n");
    break;   
  }
  return true;
}


