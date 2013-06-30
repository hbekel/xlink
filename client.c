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
#include "disk.h"
#include "pp64.h"

#define COMMAND_NONE    0x00
#define COMMAND_AUTO    0x00
#define COMMAND_LOAD    0x01
#define COMMAND_SAVE    0x02
#define COMMAND_POKE    0x03
#define COMMAND_PEEK    0x04
#define COMMAND_JUMP    0x05
#define COMMAND_RUN     0x06
#define COMMAND_RESET   0x07
#define COMMAND_WAIT    0x08
#define COMMAND_HELP    0x09
#define COMMAND_DOS     0x0a
#define COMMAND_BACKUP  0x0b
#define COMMAND_RESTORE 0x0c
#define COMMAND_VERIFY  0x0d
#define COMMAND_STATUS  0x0e
#define COMMAND_READY   0x0f

Commands* commands;
int debug = false;

char str2id(const char* arg) {
  if (strstr(arg, "c64") != NULL)      return COMMAND_AUTO;
  if (strncmp(arg, "load" ,   4) == 0) return COMMAND_LOAD;
  if (strncmp(arg, "save" ,   4) == 0) return COMMAND_SAVE;
  if (strncmp(arg, "poke" ,   4) == 0) return COMMAND_POKE;
  if (strncmp(arg, "peek" ,   4) == 0) return COMMAND_PEEK;
  if (strncmp(arg, "jump" ,   4) == 0) return COMMAND_JUMP;
  if (strncmp(arg, "run"  ,   3) == 0) return COMMAND_RUN;  
  if (strncmp(arg, "reset",   5) == 0) return COMMAND_RESET;  
  if (strncmp(arg, "wait",    4) == 0) return COMMAND_WAIT;  
  if (strncmp(arg, "help",    4) == 0) return COMMAND_HELP;  
  if (strncmp(arg, "backup",  6) == 0) return COMMAND_BACKUP;  
  if (strncmp(arg, "restore", 7) == 0) return COMMAND_RESTORE;  
  if (strncmp(arg, "verify",  6) == 0) return COMMAND_VERIFY;  
  if (strncmp(arg, "ready",   5) == 0) return COMMAND_READY;  

  if (strncmp(arg, "@", 1) == 0) {
    if(strlen(arg) == 1) {
      return COMMAND_STATUS;
    }
    else {
      return COMMAND_DOS;
    }
  }
  return -1;
}

char* id2str(const char id) {
  if (id == COMMAND_AUTO)    return (char*) "auto";
  if (id == COMMAND_LOAD)    return (char*) "load";
  if (id == COMMAND_SAVE)    return (char*) "save";
  if (id == COMMAND_POKE)    return (char*) "poke";
  if (id == COMMAND_PEEK)    return (char*) "peek";
  if (id == COMMAND_JUMP)    return (char*) "jump";
  if (id == COMMAND_RUN)     return (char*) "run";
  if (id == COMMAND_RESET)   return (char*) "reset";
  if (id == COMMAND_WAIT)    return (char*) "wait";
  if (id == COMMAND_HELP)    return (char*) "help";
  if (id == COMMAND_DOS)     return (char*) "dos";
  if (id == COMMAND_BACKUP)  return (char*) "backup";
  if (id == COMMAND_RESTORE) return (char*) "restore";
  if (id == COMMAND_VERIFY)  return (char*) "verify";
  if (id == COMMAND_STATUS)  return (char*) "status";  
  if (id == COMMAND_READY)   return (char*) "ready";  
  return (char*) "unknown";
}

int valid(int address) {
  return address >= 0x0000 && address <= 0x10000; 
}

void screenOn(void) {
  pp64_poke(0x37, 0x10, 0xd011,0x1b);
}

void screenOff(void) {
  pp64_poke(0x37, 0x10, 0xd011,0x0b);
}

Commands* commands_new() {
  Commands* commands = (Commands*) calloc(1, sizeof(Commands*));
  commands->count = 0;
  commands->items = (Command**) calloc(1, sizeof(Command**));
  return commands;
}

Command* commands_add(Commands* self, Command* command) {
  self->items = (Command**) realloc(self->items, (self->count+1) * sizeof(Command*));
  self->items[self->count] = command;
  self->count++;
  return command;
}

void commands_free(Commands* self) {
  int i;
  for(i=0; i<self->count; i++) {
    command_free(self->items[i]);
  }  

  free(self->items);
  free(self);
}

Command* command_new(char id) {
  Command* command = (Command*) calloc(1, sizeof(Command));
  command->id     = id;
  command->memory = 0xff;
  command->bank   = 0xff;
  command->start  = -1;
  command->end    = -1;
  command->argc   = 0;
  command->argv   = (char**) calloc(1, sizeof(char*));
  command_append_argument(command, (char*)"c64");
  return command;
}

void command_free(Command* self) {
  int i;

  for(i=0; i<self->argc; i++) {
    free(self->argv[i]);
  }

  free(self->argv);
  free(self);
}

void command_append_argument(Command* self, char* arg) {
  self->argv = (char**) realloc(self->argv, (self->argc+1) * sizeof(char*));
  self->argv[self->argc] = (char*) calloc(strlen(arg)+1, sizeof(char));
  strncpy(self->argv[self->argc], arg, strlen(arg));
  self->argc++;
}

int command_parse_options(Command *self) {

  int option, index;
  static struct option options[] = {
    {"debug",   required_argument, 0, 'd'},
    {"help",    no_argument,       0, 'h'},
    {"port",    required_argument, 0, 'p'},
    {"memory",  required_argument, 0, 'm'},
    {"bank",    required_argument, 0, 'b'},
    {"address", required_argument, 0, 'a'},
    {0, 0, 0, 0}
  };
  char *end;
  
  optind = 0;
  
  while(1) {

    option = getopt_long(self->argc, self->argv, "dhp:m:b:a:", options, &index);
    
    if(option == -1)
      break;

    switch(option) {
    
    case 'd':
      debug = true;
      break;

    case 'p':
      if (!pp64_configure(optarg))
	return false; 
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

  if(optind > 0) {
	  self->argc -= optind;
	  int i;
	  for(i=0; i<self->argc; i++) {
		  free(self->argv[i]);
		  self->argv[i] = self->argv[i+optind];
	  }
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

int command_auto(Command* self) {

  if (self->argc == 0) {
    return false;
  }

  char *filename = self->argv[0];
  char *suffix = (filename + strlen(filename)-4);

  if (strncasecmp(suffix, ".prg", 4) != 0) {
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
  
  file = fopen(filename, "rb");
  
  if (file == NULL) {
    fprintf(stderr, "c64: error: load: '%s': %s\n", filename, strerror(errno));
    return false;
  }
  stat(filename, &st);
  size = st.st_size;
  
  suffix = (filename + strlen(filename)-4);

  // get load address from .prg file
  if (strncasecmp(suffix, ".prg", 4) == 0) {
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

  file = fopen(filename, "wb");

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

  if (strncasecmp(suffix, ".prg", 4) == 0)
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
  unsigned int comma = strcspn(argument, ",");

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

int command_wait(Command* self) {
  int timeout = 3000;

  if (self->argc > 0) {
    timeout = strtol(self->argv[0], NULL, 0);
  }
  return pp64_ping(timeout);
}

int command_reset(Command* self) {
  command_print(self);
  return pp64_reset();
}

int command_help(Command *self) {
  int id;
  command_print(self);

  if(self->argc == 0) {
    id = COMMAND_NONE;
  }
  else {
    if(self->argv[1][0] == '@') {
      id = COMMAND_DOS;
    }
    else {
      id = str2id(self->argv[1]);
    }
  }
  usage(id);
  return true;
}

int command_dos(Command *self) {

  if (self->argc == 0) {
    return false;
  }
  int result = pp64_dos(self->argv[0]);

  self->id = COMMAND_STATUS;
  return command_execute(self) && result;
}

int command_backup(Command *self) {
  
  bool read_sector(Sector *sector) {
    printf("\rreading track %02d, sector %02d", sector->track, sector->number); fflush(stdout);
    
    return pp64_sector_read(sector->track, sector->number, sector->bytes); 
  }
  
  int result = true;
  Disk* disk;

  if (self->argc == 0) {
    return false;
  }
  
  screenOff();

  disk = disk_new(35);
  if(disk_each_sector(disk, &read_sector)) {
    disk_save(disk, self->argv[0]);
  }

  screenOn();
  printf("\n");

  disk_free(disk);
  return result;
}

int command_restore(Command *self) {

  bool write_sector(Sector *sector) {

    if (debug) {
      sector_print(sector);
    } 
    else {
      printf("\rwriting track %02d, sector %02d", sector->track, sector->number);
      fflush(stdout);
    }

    return pp64_sector_write(sector->track, sector->number, sector->bytes); 
  }

  int result = true;

  if (self->argc == 0) {
    return false;
  }

  char *filename = self->argv[0];
  Disk* disk = disk_load(filename);

  command_print(self);

  if(disk == NULL) {
    return false;
  }

  if(disk->size != 35) {
    fprintf(stderr, "c64: error: no support for 40-track disks\n");
    result = false;
    goto done;
  }

  screenOff();

  disk_each_sector(disk, &write_sector);
  pp64_dos("I");

  screenOn();
  printf("\n");

 done:
  disk_free(disk);
  return result;
}

int command_verify(Command *self) {

  bool verify_sector(Sector* expected) {

    Sector* actual = sector_new(expected->track, expected->number);
    int result = false;

    printf("\rverifying track %02d, sector %02d...", 
	   actual->track, actual->number); fflush(stdout);

    if(!pp64_sector_read(actual->track, actual->number, actual->bytes)) {
      goto done;
    }
    result = sector_equals(expected, actual);

  done:
    sector_free(actual);
    return result;
  }

  bool verify_sector_skip_track_18(Sector* expected) {

    if(expected->track == 18) 
      return true;

    return verify_sector(expected);
  }

  Disk* backup;
  int result = true;

  if (self->argc == 0) {
    return false;
  }
  
  if ((backup = disk_load(self->argv[0])) == NULL) {
    return false;
  }
  screenOff();
  
  // verify track 18 first
  result = track_each_sector(backup->tracks[17], &verify_sector);
  
  if (result) // verify other tracks
    result = disk_each_sector(backup, &verify_sector_skip_track_18);
  
  screenOn();

  printf("%s\n", result ? "OK" : "FAILED");
  disk_free(backup);
  return result;
}

int command_status(Command* self) {

  unsigned char *status = (unsigned char*) calloc(sizeof(unsigned char), 256);
  int result = false;
  
  if((result = pp64_drive_status(status))) {
    printf("%s\n", status);
  }
  free(status);
  return result;
}

int command_ready(Command* self) {

  if(!pp64_ping(250)) {
    pp64_reset();
    return pp64_ping(3000);
  }
  return true;
}

int command_execute(Command* self) {

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

  case COMMAND_WAIT:
    if(!command_wait(self))
      return false;
    break;

  case COMMAND_RESET:
    if (!command_reset(self))
      return false;
    break;

  case COMMAND_HELP:
    if(!command_help(self))
      return false;
    break;

  case COMMAND_DOS:
    if(!command_dos(self))
      return false;
    break;  

  case COMMAND_BACKUP:
    if(!command_backup(self))
      return false;
    break;

  case COMMAND_RESTORE:
    if(!command_restore(self))
      return false;
    break;

  case COMMAND_VERIFY:
    if(!command_verify(self))
      return false;
    break;

  case COMMAND_STATUS:
    if(!command_status(self))
      return false;
    break;

  case COMMAND_READY:
    if(!command_ready(self))
      return false;
    break;
  }
  return true;
}

int main(int argc, char **argv) {

  Command* command = NULL;   
  int id, i, result;

  result = EXIT_SUCCESS;

  commands = commands_new();
  command = commands_add(commands, command_new(COMMAND_AUTO));

  if(argc <= 1) {
    usage(COMMAND_NONE);
    return EXIT_SUCCESS;
  }

  for (i=1; i<argc; i++) {
    id = str2id(argv[i]);

    if (id != -1 && command->id != COMMAND_HELP) {
      command = commands_add(commands, command_new(id));

      if(command->id == COMMAND_DOS && strlen(argv[i]) > 1) {
	command_append_argument(command, argv[i]+1);
      }
    } else 
      command_append_argument(command, argv[i]);    
  }

  for (i=0; i<commands->count; i++) {
    command = commands->items[i];

    if (!command_parse_options(command)) {
      result = EXIT_FAILURE;
      break;
    }
   
    if (!command_execute(command)) {
      result = EXIT_FAILURE;
      break;
    }
  }
 
  commands_free(commands);
  return result;
}

void usage(int id) {

  switch(id) {
  case COMMAND_NONE:
    printf("pp64 client 0.3 Copyright (C) 2013 Henning Bekel <h.bekel@googlemail.com>\n\n");

    printf("Usage: c64 [<opts>] [<file>.prg|@[<dos-command>]]\n");
    printf("       c64 [<opts>] [<command> [<opts>] [<arguments>]]...\n\n");
    printf("Options:\n");
    printf("         -h, --help                    : show this help\n");
    printf("         -d, --debug                   : enable debug messages\n");
    printf("         -p, --port <port>             : ");
#if linux
    printf("port device (default: /dev/parport0)\n");
#elif windows
    printf("port address (default: 0x378)\n");
#endif
    printf("         -a, --address <start>[-<end>] : C64 address/range (default: autodetect)\n");
    printf("         -m, --memory                  : C64 memory config (default: 0x37)\n\n");
    
    printf("Commands:\n");
    printf("          help  <command>              : show detailed help for <command>\n");
    printf("          load  [<opts>] <file>        : load file into C64 memory\n");
    printf("          save  [<opts>] <file>        : save C64 memory to file\n");
    printf("          poke  [<opts>] <addr>,<val>  : poke value into C64 memory\n");
    printf("          peek  [<opts>] <addr>        : read value from C64 memory\n");
    printf("          jump  [<opts>] <addr>        : jump to specified address\n");
    printf("          run   [<opts>]               : run basic program\n");
    printf("          backup <file>                : backup disk to d64 file\n");
    printf("          restore <file>               : restore d64 file to disk\n");
    printf("          verify <file>                : verify disk against d64 file\n");
    printf("          @[command]                   : read drive status or send dos command\n");    
    printf("          ready                        : make sure the server is ready\n\n");
    printf("          wait  [<msec>]               : wait <msec>s for server (default: 3000)\n");
    printf("          reset                        : reset C64 (only if using reset circuit)\n");
    break;

  case COMMAND_LOAD:
    printf("Usage: c64 load [--address <start>[-<end>] [--memory <mem>] <file>\n");
    printf("\n");
    printf("Load the specified file into C64 memory\n");
    printf("\n");
    printf("If the filename ends with .prg, it is assumed that the file is a C64\n");
    printf("PRG file and the first two bytes contain the C64 load address. If no\n");
    printf("load address is specified by the user, the load address from the .prg\n");
    printf("file is used. Otherwise the user supplied load address is used. If an\n");
    printf("additional end address is specified, transfer will end as soon as the\n");
    printf("end address or the end of the file is reached, whichever comes first.\n");
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
    printf("Usage: c64 save [--address <start>-<end>] [--memory <mem>] file\n");
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
    printf("Usage: c64 poke [--memory <mem>] <address>,<byte>\n");
    printf("\n");
    printf("Poke the specified byte to the specified address. \n");
    printf("\n");
    printf("If no memory config is specified, then the default memory config 0x37\n");
    printf("will be used, so that values poked to the io area $d000-$dfff will\n");
    printf("have the expected effect.\n");
    printf("\n");
    break;

  case COMMAND_PEEK:
    printf("Usage: c64 peek [--memory <mem>] <address>\n");
    printf("\n");
    printf("Read the byte at the specified C64 memory address and print it on\n");
    printf("standard output.\n");
    printf("\n");
    printf("If no memory config is specified, the default value 0x37 will be used.\n");
    printf("\n");
    break;

  case COMMAND_JUMP:
   printf("Usage: c64 jump <address>\n");
   printf("\n");
   printf("Jump to the specified address in C64 memory. The stack pointer,\n");
   printf("processor flags and registers will be reset prior to jumping.\n");
   printf("\n");
   break;

  case COMMAND_RUN:
    printf("Usage: c64 run\n");
    printf("\n");
    printf("RUN the currently loaded basic program.\n");
    printf("\n");
    break;

  case COMMAND_WAIT:
    printf("Usage: c64 wait <timeout>\n");
    printf("\n");
    printf("Try to ping the C64 server for at most <timeout> milliseconds. If the\n");
    printf("server responds within the specified timeout, then the next command on\n");
    printf("the command line will be executed, otherwise the client will exit with\n");
    printf("a negative exit status.\n");
    printf("\n");
    break;

  case COMMAND_RESET:
    printf("Usage: c64 reset\n");
    printf("\n");
    printf("If a reset circuit is installed, this command will hold the PC's INIT\n");
    printf("line low for a short period of time, which will ground the C64's RESET\n");
    printf("line, performing a hardware reset.\n");
    printf("\n");
    break;

  case COMMAND_DOS:
    printf("Usage: c64 @[command]\n");
    printf("\n");
    printf("Send the specified DOS command to the drive and report the resulting\n");
    printf("drive status. If no command is specified, report drive status only.\n");
    printf("\n");
    break;

  case COMMAND_BACKUP:
    printf("Usage: c64 backup <file>.d64\n");
    printf("\n");
    printf("Backup a disk to a d64 file. Reads 35 tracks from disks and saves them to\n");
    printf("the specified file. Note that no error checking is performed and no error\n");
    printf("information is appended to the d64 file.\n");
    printf("\n");
    break;

  case COMMAND_RESTORE:
    printf("Usage: c64 restore <file>.d64\n");
    printf("\n");
    printf("Write a 35 track d64 file to disk. The data is written as is, i.e. without\n");
    printf("interpreting any error information that may be included in the d64 file.\n");
    printf("\n");
    break;

  case COMMAND_VERIFY:
    printf("Usage: c64 verify <file>.d64\n");
    printf("\n");
    printf("Verify disk against d64 file. Reads 35 tracks from disk and compares the data\n");
    printf("against the specified d64 file. Track 18 is verified first, then the remaining\n");
    printf("tracks are verified in order.\n");
    printf("\n");
    break;

  case COMMAND_READY:
    printf("Usage: c64 ready [<commands>...]\n");
    printf("\n");
    printf("Makes sure that the server is ready. First the server is pinged. If it doesn't\n");
    printf("respond immediately, the c64 is reset. If the server responds to another ping\n");
    printf("within three seconds, then the remaining commands (if any) are executed.\n");
    printf("\n");
    printf("This command requires the server to be installed permanently so that it is\n");
    printf("available after reset.\n");
    printf("\n");
    break;
  }
}


