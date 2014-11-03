#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#if linux
  #include <readline/readline.h>
  #include <readline/history.h>
#endif 

#include "target.h"
#include "client.h"
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

int commands_each(Commands* self, int (*func) (Command* command)) {
  int result = true;

  for(int i=0; i<self->count; i++) {
    if(!(result = func(self->items[i]))) {
      break;
    }
  }
  return result;
}

//------------------------------------------------------------------------------

int commands_execute(Commands* self) {
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

  self->id = str2id(current());

  if(isCommand(self->name)) {
    next();
  }

  int arity = command_arity(self);
  int consumed = 0;

  for(;hasNext();next()) {

    if(isCommand(current())) {
      break;
    }

    if (consumed == arity) {
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

int command_parse_options(Command *self) {
  
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
      logger->dec();
      break;

    case 'v':
      logger->inc();
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

int command_print(Command* self) {

  char result[1024];
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

int command_find_basic_program(Command* self) {

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

int command_none(Command* self) {

  StringList *arguments = stringlist_new();
  Commands *commands;
  int result = true;

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

int command_load(Command* self) {
  
  FILE *file;
  struct stat st;
  long size;
  int loadAddress;
  unsigned short newServerAddress;
  char *data;

  xlink_server_info server;
  
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

  if (self->memory == 0xff) {
    
    if(self->end > 0xD000 && self->start < 0xE000)
      self->memory = 0x33; // write to ram below io by default
    else 
      self->memory = 0x37;    
  }

  if (self->bank == 0xff) {
    self->bank = 0x00;
  }

  data = (char*) calloc(size, sizeof(char));
  
  fseek(file, self->skip, SEEK_SET);
  fread(data, sizeof(char), size, file);
  fclose(file);  

  command_print(self);

  if(xlink_identify(&server)) {

    if(command_requires_server_relocation(self, &server)) {

      if(!command_server_relocation_possible(self, &server, &newServerAddress)) {
	logger->error("Impossible to relocate ram-based server: out of memory");
	free(data);
	return false;
      }

      if(!command_server_relocate(self, newServerAddress)) {
	logger->error("Server relocation failed");
	free(data);
	return false;
      }
    }
  }

  if (!xlink_load(self->memory, self->bank, self->start, self->end, data, size)) {
    free(data);
    return false;
  }

  
  free(data);
  return true;
}

//------------------------------------------------------------------------------

int command_save(Command* self) {
  
  FILE *file;
  char *suffix;
  int size;
  char *data;

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

  data = (char*) calloc(size, sizeof(char));

  file = fopen(filename, "wb");

  if(file == NULL) {
    logger->error("'%s': %s", filename, strerror(errno));
    free(data);
    return false;
  }

  command_print(self);

  if(!xlink_save(self->memory, self->bank, self->start, self->end, data, size)) {
    free(data);
    fclose(file);
    return false;
  }

  if (strncasecmp(suffix, ".prg", 4) == 0)
    fwrite(&self->start, sizeof(char), 2, file);
  
  fwrite(data, sizeof(char), size, file);
  fclose(file);

  free(data);    
  return true;
}

//------------------------------------------------------------------------------

int command_poke(Command* self) {
  char *argument;
  int address;
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

  address = strtol(addr, NULL, 0);
  value = strtol(val, NULL, 0);

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x00;

  command_print(self);

  return xlink_poke(self->memory, self->bank, address, value);
}

//------------------------------------------------------------------------------

int command_peek(Command* self) {
  
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

int command_jump(Command* self) {

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

int command_run(Command* self) {
  int result;

  if(self->argc == 1) {

    logger->suspend();
    if(!(result = command_load(self))) {
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

int command_requires_server_relocation(Command* self, xlink_server_info* server) {

  bool result = false;

  if(server->type == XLINK_SERVER_TYPE_ROM) {
    goto done;
  }

  if ((self->start >= server->start && self->start <= server->end) ||
      (self->end >= server->start && self->end <= server->end)) {
    
    logger->debug("Relocation required: data ($%04X-$%04X) overlaps server ($%04X-$%04X)",
		  self->start, self->end, server->start, server->end);

    result = true;
  }

 done:
  return result;  
}

//------------------------------------------------------------------------------

int command_server_relocation_possible(Command* self, xlink_server_info* server, unsigned short* address) {

  bool result = false;

  if(0x0801 + server->length < self->start) {
    (*address) = 0x0801;
    result = true;
    goto done;
  }

  if(self->end + server->length < 0xa000) {
    (*address) = self->end;
    result = true;
    goto done;
  }

  if(self->end >= 0xc000 && self->end + server->length < 0xd000) {
    (*address) = self->end;
    result = true;
    goto done;
  }
  
 done:
  return result;  
}

//------------------------------------------------------------------------------

int command_server_relocate(Command* self, unsigned short address) {
  bool result = false;

  int size;
  unsigned char *code = xlink_server(address, &size);

  if(code == NULL) {
    goto done;
  }

  code+=2;
  size-=2;

  logger->debug("Relocating server to $%04X-$%04X", address, address+size);

  if(xlink_load(0x37, 0x00, address, address+size, (char*) code, size)) {

    if(address == 0x0801) {
      result = xlink_run();
    } else {
      result = xlink_jump(0x37, 0x00, address);
    }
  }
  free(code);

 done:
  return result;
}

//------------------------------------------------------------------------------

int command_reset(Command* self) {
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

int command_benchmark(Command* self) {

  Watch* watch = watch_new();
  bool result = false;

  char payload[0x8000];
  char roundtrip[sizeof(payload)];
    
  int start = 0x1000;
  int end = start + sizeof(payload);
    
  if (!xlink_ready()) {
    logger->error("no response from C64");
    goto done;
  }

  logger->info("sending %d bytes...", sizeof(payload));
    
  watch_start(watch);
  
  xlink_load(0x37, 0x00, start, end, payload, sizeof(payload));
  
  float seconds = (watch_elapsed(watch) / 1000.0);
  float kbs = sizeof(payload)/seconds/1024;
    
  logger->info("%.2f seconds at %.2f kb/s", seconds, kbs);       
    
  logger->info("receiving %d bytes...", sizeof(payload));
    
  watch_start(watch);
    
  xlink_save(0x37, 0x00, start, end, roundtrip, sizeof(roundtrip));
  
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

int command_identify(Command *self) {

  xlink_server_info server;
  
  if(xlink_identify(&server)) {

    logger->info("%s %s-based server v%d.%d $%04X-$%04X (%d bytes)",
                 server.machine == XLINK_MACHINE_C64 ? "C64" : "Unknown",
                 server.type == XLINK_SERVER_TYPE_RAM ? "RAM" : "ROM",
                 (server.version & 0xf0) >> 4, server.version & 0x0f,
                 server.start, server.end, server.length);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

int command_server(Command *self) {

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

  data = xlink_server(self->start, &size);

  if ((file = fopen(self->argv[0], "wb")) == NULL) {
    logger->error("couldn't open %s for writing: %s", strerror(errno));
    goto done;
  }

  fwrite(data, sizeof(unsigned char), size, file);
  fclose(file);

 done:
  free(data);
  return result;
}

//------------------------------------------------------------------------------

int command_help(Command *self) {

  if (self->argc > 0) {
    logger->error("unknown command: %s", self->argv[0]);
    return false;
  }

  mode = MODE_HELP;
  return true;
}

//------------------------------------------------------------------------------

int command_status(Command* self) {

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

int command_dos(Command *self) {

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

int command_backup(Command *self) {
    
  int result = true;
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

int command_restore(Command *self) {

  int result = true;
  
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

int command_verify(Command *self) {

  Disk* disk;
  int result = true;

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

int command_ready(Command* self) {

  command_print(self);

  if (!xlink_ready()) {
    logger->error("no response from C64");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------

int command_ping(Command* self) {
  command_print(self);
  
  Watch *watch = watch_new();

  int response = xlink_ping();

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

int command_execute(Command* self) {

  int result = false;

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

#if linux
    if (strcmp(argv[0], "shell") == 0) {
      shell();
      return EXIT_SUCCESS;   
    }
#endif
  }

  commands = commands_new(argc, argv);

  result = commands_execute(commands) ? EXIT_SUCCESS : EXIT_FAILURE;

  commands_free(commands);

  logger->leave();

  return result;
}

//------------------------------------------------------------------------------

#if linux

static char* known_commands[19] = { 
  "help",
  "load", 
  "save",
  "peek",
  "poke",
  "jump",
  "run",
  "backup",
  "restore",
  "verify",
  "reset",
  "ready",
  "exit",
  "quit",
  "ping",
  "bootloader",
  "benchmark",
  "identify",
  "server",
  NULL 
};

static char *dupstr(char *s) {
  char *r = calloc(strlen(s) + 1, sizeof(char));
  strncpy(r, s, strlen(s));
  return r;
}

//------------------------------------------------------------------------------

static void trim(char * const a)
{
  char *p = a, *q = a;
  while (isspace(*q)) ++q;
  while (*q) *p++ = *q++;
  *p = '\0';
  while (p > a && isspace(*--p)) *p = '\0';
}

//------------------------------------------------------------------------------

static char *command_generator(char *text, int state) {
  static int list_index, len;
  char *name;
  
  if (!state) {
    list_index = 0;
    len = strlen(text);
  }
  while ((name = known_commands[list_index])) {
    list_index++;
    
    if (strncmp(name, text, len) == 0)
      return dupstr(name);
  }
  return ((char *)NULL);
}

//------------------------------------------------------------------------------

static char **shell_completion(char *text, int start, int end) {
  extern char **completion_matches();
  return (char **) completion_matches(text, command_generator);
}

//------------------------------------------------------------------------------

static int shell_command(char *line) {
  if(strcmp(line, "help") == 0) {
    usage();
    return true;
  }
  
  if((strcmp(line, "quit") == 0) ||
     (strcmp(line, "exit") == 0)) {
    exit(EXIT_SUCCESS);
  }
  return false;
}

//------------------------------------------------------------------------------

void shell(void) {
  
  char *line;
  char *prompt = "xlink> ";

  Commands* commands;
  StringList *arguments;

  rl_variable_bind("expand-tilde", "on");  
  rl_attempted_completion_function = (rl_completion_func_t *) shell_completion;
  
  while((line = readline(prompt))) {      

    trim(line);

    if(strlen > 0) {
      add_history(line);

      if(!shell_command(line)) {

        arguments = stringlist_new();
        stringlist_append_tokenized(arguments, line, " \t");

        commands = commands_new(arguments->size, arguments->strings);
        commands_execute(commands);
        
        commands_free(commands);
        stringlist_free(arguments);
      }
      mode = MODE_EXEC;
    }    
    free(line);
  }
  printf("\n");
}
#endif

//------------------------------------------------------------------------------

void version(void) {
  printf("xlink 1.0 Copyright (C) 2014 Henning Bekel <h.bekel@googlemail.com>\n");
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
#if linux
  printf("          shell                        : enter interactive command shell\n");
#endif
  printf("          bootloader                   : enter dfu-bootloader (USB devices only)\n");
  printf("          benchmark                    : test/measure transfer speed\n");
  printf("\n");
  printf("          server [-a<addr>] <file>     : create ram based server program\n");
  printf("          ping                         : check if the server is available\n");
  printf("          reset                        : reset C64 (only if using reset circuit)\n");
  printf("          ready                        : try to make sure the server is ready\n");
  printf("          identify                     : identify remote machine and server\n");
  printf("\n");
  printf("          load  [<opts>] <file>        : load file into C64 memory\n");
  printf("          save  [<opts>] <file>        : save C64 memory to file\n");
  printf("          poke  [<opts>] <addr>,<val>  : poke value into C64 memory\n");
  printf("          peek  [<opts>] <addr>        : read value from C64 memory\n");
  printf("          jump  [<opts>] <addr>        : jump to specified address\n");
  printf("          run   [<opts>] [<file>]      : run program, optionally load it before\n");
  printf("          <file>...                    : load file(s) and run last file\n");
  printf("\n");
  printf("          @[<dos-command>]             : read drive status or send dos command\n");    
  printf("          backup <file>                : backup disk to d64 file\n");
  printf("          restore <file>               : restore d64 file to disk\n");
  printf("          verify <file>                : verify disk against d64 file\n");
  printf("\n");
}

//------------------------------------------------------------------------------

int help(int id) {

  switch(id) {
  case COMMAND_NONE:
    usage();
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
    printf("Backup a disk to a d64 file. Reads 35 tracks from disks and saves them to\n");
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
    printf("\n");
    break;

  case COMMAND_SERVER:
    printf("Usage: server [--address <address>] <file>\n");
    printf("\n");
    printf("Write the ram-based C64 server programm to file. Use address to specify the\n");
    printf("start address for the server code. If the address is $0801 the code will be\n");
    printf("prefixed with a single line basic program that installs the server. This is the\n");
    printf("default if no address is specified.\n");
    printf("\n");
    break;
    
  }
  return true;
}


