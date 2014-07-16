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
#include "pp64.h"

#define COMMAND_NONE    0x00
#define COMMAND_LOAD    0x01
#define COMMAND_SAVE    0x02
#define COMMAND_POKE    0x03
#define COMMAND_PEEK    0x04
#define COMMAND_JUMP    0x05
#define COMMAND_RUN     0x06
#define COMMAND_RESET   0x07
#define COMMAND_HELP    0x08
#define COMMAND_DOS     0x09
#define COMMAND_BACKUP  0x0a
#define COMMAND_RESTORE 0x0b
#define COMMAND_VERIFY  0x0c
#define COMMAND_STATUS  0x0d
#define COMMAND_READY   0x0e
#define COMMAND_PING    0x0f
#define COMMAND_TEST    0x10

#define MODE_EXEC 0x00
#define MODE_HELP 0x01

int mode  = MODE_EXEC;

//------------------------------------------------------------------------------

char str2id(const char* arg) {
  if (strncmp(arg, "load" ,   4) == 0) return COMMAND_LOAD;
  if (strncmp(arg, "save" ,   4) == 0) return COMMAND_SAVE;
  if (strncmp(arg, "poke" ,   4) == 0) return COMMAND_POKE;
  if (strncmp(arg, "peek" ,   4) == 0) return COMMAND_PEEK;
  if (strncmp(arg, "jump" ,   4) == 0) return COMMAND_JUMP;
  if (strncmp(arg, "run"  ,   3) == 0) return COMMAND_RUN;  
  if (strncmp(arg, "reset",   5) == 0) return COMMAND_RESET;  
  if (strncmp(arg, "help",    4) == 0) return COMMAND_HELP;  
  if (strncmp(arg, "backup",  6) == 0) return COMMAND_BACKUP;  
  if (strncmp(arg, "restore", 7) == 0) return COMMAND_RESTORE;  
  if (strncmp(arg, "verify",  6) == 0) return COMMAND_VERIFY;  
  if (strncmp(arg, "ready",   5) == 0) return COMMAND_READY;  
  if (strncmp(arg, "ping",    4) == 0) return COMMAND_PING;  
  if (strncmp(arg, "test",    3) == 0) return COMMAND_TEST;  

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
  if (id == COMMAND_NONE)    return (char*) "none";
  if (id == COMMAND_LOAD)    return (char*) "load";
  if (id == COMMAND_SAVE)    return (char*) "save";
  if (id == COMMAND_POKE)    return (char*) "poke";
  if (id == COMMAND_PEEK)    return (char*) "peek";
  if (id == COMMAND_JUMP)    return (char*) "jump";
  if (id == COMMAND_RUN)     return (char*) "run";
  if (id == COMMAND_RESET)   return (char*) "reset";
  if (id == COMMAND_HELP)    return (char*) "help";
  if (id == COMMAND_DOS)     return (char*) "dos";
  if (id == COMMAND_BACKUP)  return (char*) "backup";
  if (id == COMMAND_RESTORE) return (char*) "restore";
  if (id == COMMAND_VERIFY)  return (char*) "verify";
  if (id == COMMAND_STATUS)  return (char*) "status";  
  if (id == COMMAND_READY)   return (char*) "ready";  
  if (id == COMMAND_PING)    return (char*) "ping";  
  if (id == COMMAND_TEST)    return (char*) "test";  
  return (char*) "unknown";
}

//------------------------------------------------------------------------------

int isCommand(const char *str) {
  return str2id(str) > COMMAND_NONE;
}

//------------------------------------------------------------------------------

int valid(int address) {
  return address >= 0x0000 && address <= 0x10000; 
}

void screenOn(void) {
  pp64_poke(0x37, 0x10, 0xd011, 0x1b);
}

//------------------------------------------------------------------------------

void screenOff(void) {
  pp64_poke(0x37, 0x10, 0xd011, 0x0b);
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

int commands_each(Commands* commands, int (*func) (Command* command)) {
  int result = true;

  for(int i=0; i<commands->count; i++) {
    if(!(result = func(commands->items[i]))) {
      break;
    }
  }
  return result;
}

//------------------------------------------------------------------------------

int commands_execute(Commands* commands) {
  return commands_each(commands, &command_execute);
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
  command->command   = NULL;
  command->memory    = 0xff;
  command->bank      = 0xff;
  command->start     = -1;
  command->end       = -1;
  command->argc      = 0;
  command->argv      = (char**) calloc(1, sizeof(char*));
  
  command_append_argument(command, (char*)"getopt");
  command_consume_arguments(command, argc, argv);

  return command;
}

void command_free(Command* self) {

  free(self->command);

  self->argc += self->offset;
  self->argv -= self->offset;

  for(int i=0; i<self->argc; i++) {
    free(self->argv[i]);
  }
  free(self->argv);
  free(self);
}

//------------------------------------------------------------------------------

void command_consume_arguments(Command *self, int *argc, char ***argv) {
  
  int has_next(void) {
    return (*argc) > 0;
  }

  void next(void) {
    (*argc)--; 
    (*argv)++;
  }    

  char *current(void) {
    return (*argv)[0];
  }

  self->command = (char *) calloc(strlen(current())+1, sizeof(char));
  strncpy(self->command, current(), strlen(current()));

  if(isCommand(self->command)) {
    self->id = str2id(current());
    next();
  }

  for(;has_next();next()) {

    if(isCommand(current())) {
      break;
    }
    else {
      command_append_argument(self, current());
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
  static struct option options[] = {
    {"help",    no_argument,       0, 'h'},
    {"device",  required_argument, 0, 'd'},
    {"level",   required_argument, 0, 'l'},
    {"memory",  required_argument, 0, 'm'},
    {"bank",    required_argument, 0, 'b'},
    {"address", required_argument, 0, 'a'},
    {0, 0, 0, 0}
  };
  char *end;
  
  optind = 0;
  
  while(1) {

    option = getopt_long(self->argc, self->argv, "hd:l:m:b:a:", options, &index);
    
    if(option == -1)
      break;

    switch(option) {
    
    case 'l':
      logger->set(optarg);
      break;

    case 'd':
      if (!pp64_set_device(optarg)) {
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
        logger->error("%s: start address out of range: 0x%04X",
                command_get_name(self), self->start);
        return false;
      }

      if(self->end != -1) {
        
        if (!valid(self->end)) {
          logger->error("%s: end address out of range: 0x%04X",
                  command_get_name(self), self->end);
          return false;
        }
	
        if (self->end < self->start) {
          logger->error("%s: end address before start address: 0x%04X > 0x%04X",
                  command_get_name(self), self->end, self->start);
          return false;
        }
	
        if (self->start == self->end) {
          logger->error("%s: start address equals end address: 0x%04X == 0x%04X",
                  command_get_name(self), self->end, self->start);
          return false;	
        }
      }
      break;      
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

void command_print(Command* self) {

  if(self->id == COMMAND_NONE) 
    return;

  char result[1024];

  sprintf(result, "%s ", command_get_name(self));

  if((unsigned char) self->memory != 0xff) {
    sprintf(result + strlen(result), "-m 0x%02X ", (unsigned char) self->memory);
  }

  if((unsigned char) self->bank != 0xff) {
    sprintf(result + strlen(result), "-b 0x%02X ", (unsigned char) self->bank);
  }

  if((unsigned short) self->start != 0xffff) {
    sprintf(result + strlen(result), "-a 0x%04X", (unsigned short) self->start);

    if((unsigned short) self->end != 0xffff) {
      sprintf(result + strlen(result), "-0x%04X", (unsigned short) self->end);
    }
    sprintf(result + strlen(result), " ");
  }  

  int i;
  for (i=0; i<self->argc; i++) {
    sprintf(result + strlen(result), "%s ", self->argv[i]);
  }
  logger->debug(result);
} 

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

int command_none(Command* self) {
  return true;
}

//------------------------------------------------------------------------------

int command_load(Command* self) {
  
  FILE *file;
  struct stat st;
  long size;
  int loadAddress;
  char *suffix;
  char *data;

  if (self->argc == 0) {
    logger->error("load: no file specified");
    return false;
  }

  char *filename = self->argv[0];
  
  file = fopen(filename, "rb");
  
  if (file == NULL) {
    logger->error("load: '%s': %s", filename, strerror(errno));
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
      logger->error("load: not a .prg file and no start address specified");
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

  if (!pp64_load(self->memory, self->bank, self->start, self->end, data, size)) {
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
    logger->error("save: no file specified");
    return false;
  }

  char *filename = self->argv[0];

  if(self->start == -1) {
    if(!command_find_basic_program(self)) {
      logger->error("save: no start address specified and no basic program in memory");
      return false;
    }
  }

  if(self->start == -1) {                   
    logger->error("save: no start address specified");
    return false;
  }
  else {
    if(self->end == -1) {                   
      logger->error("save: no end address specified");
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
    logger->error("save: '%s': %s", filename, strerror(errno));
    free(data);
    return false;
  }

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

//------------------------------------------------------------------------------

int command_poke(Command* self) {
  char *argument;
  int address;
  unsigned char value;
  
  if (self->argc == 0) {
    logger->error("poke: argument required");
    return false;
  }
  argument = self->argv[0];
  unsigned int comma = strcspn(argument, ",");

  if (comma == strlen(argument) || comma == strlen(argument)-1) {
    logger->error("poke: expects <address>,<value>");
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

  return pp64_poke(self->memory, self->bank, address, value);
}

//------------------------------------------------------------------------------

int command_peek(Command* self) {
  
  if (self->argc == 0) {
    logger->error("peek: no address specified");
    return false;
  }

  int address = strtol(self->argv[0], NULL, 0);
  unsigned char value;

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x10;

  if(!pp64_peek(self->memory, self->bank, address, &value)) {
    return false;
  }
  printf("%d\n", value);
  
  return true;
}

//------------------------------------------------------------------------------

int command_jump(Command* self) {

  if (self->argc == 0) {
    logger->error("jump: no address specified");
    return false;
  }

  int address = strtol(self->argv[0], NULL, 0);

  if(address == 0) {
    if(self->start != -1) {
      address = self->start;
    }
    else {
      logger->error("jump: no address specified");
      return false;    
    }
  }

  if (self->memory == 0xff)
    self->memory = 0x37;

  if (self->bank == 0xff)
    self->bank = 0x10;

  return pp64_jump(self->memory, self->bank, address);
}

//------------------------------------------------------------------------------

int command_run(Command* self) {
  int result;

  if(self->argc == 1) {
    if(!(result = command_load(self))) {
      return result;
    }
    if (self->start != 0x0801) {
      
      if (self->memory == 0xff)
        self->memory = 0x37;
      
      if (self->bank == 0xff)
        self->bank = 0x10;
      
      return pp64_jump(self->memory, self->bank, self->start);
    }
  }
  return pp64_run();
}

//------------------------------------------------------------------------------

int command_reset(Command* self) {
  return pp64_reset();
}

//------------------------------------------------------------------------------

int command_test(Command* self) {
  return pp64_test(self->argv[0]);
}

//------------------------------------------------------------------------------

int command_help(Command *self) {

  if (self->argc > 0) {
    logger->error("help: unknown command: %s", self->argv[0]);
    return false;
  }

  mode = MODE_HELP;
  return true;
}

//------------------------------------------------------------------------------

int command_status(Command* self) {

  char *status = (char*) calloc(sizeof(unsigned char), 256);
  int result = false;
  
  if(pp64_drive_status(status)) {
    printf("%s\n", status);
    result = true;
  }

  free(status);
  return result;
}

//------------------------------------------------------------------------------

int command_dos(Command *self) {

  if (pp64_dos(self->command+1)) {
    return command_status(self);
  }
  return false;
}

//------------------------------------------------------------------------------

int command_backup(Command *self) {
  
  bool read_sector(Sector *sector) {
    printf("\rreading track %02d, sector %02d", sector->track, sector->number); fflush(stdout);    
    
    return pp64_sector_read(sector->track, sector->number, sector->bytes); 
  }
  
  int result = true;
  Disk* disk;

  if (self->argc == 0) {
    logger->error("backup: no file specified");
    return false;
  }
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

int command_restore(Command *self) {

  bool write_sector(Sector *sector) {

    printf("\rwriting track %02d, sector %02d", sector->track, sector->number);
    fflush(stdout);

    return pp64_sector_write(sector->track, sector->number, sector->bytes); 
  }

  int result = true;
  
  if (self->argc == 0) {
    logger->error("restore: no file specified");
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
    logger->error("restore: no support for disks > 35 tracks\n");
    result = false;
    goto done;
  }

  printf("formatting disk: \"%s,%s\"...", disk->name, disk->id); fflush(stdout);

  if(!(pp64_dos(format_disk) && pp64_dos("I"))) {
    printf("FAILED\n");
    result = false;
    goto done;
  }
  printf("OK\n");

  screenOff();

  disk_each_sector(disk, &write_sector);
  pp64_dos("I");

  screenOn();
  printf("\n");

 done:
  free(format_disk);
  disk_free(disk);
  return result;
}

//------------------------------------------------------------------------------

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

  bool verify_sector_skipping_track_18(Sector* expected) {

    if(expected->track == 18) 
      return true;

    return verify_sector(expected);
  }

  Disk* disk;
  int result = true;

  if (self->argc == 0) {
    logger->error("verify: no file specified");
    return false;
  }
  
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

  int timeout = 3000;

  if(!pp64_ping()) {
    pp64_reset();
    
    while(timeout) {
      if(pp64_ping()) return true;
      timeout-=PP64_PING_TIMEOUT;
    }
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------

int command_ping(Command* self) {
  return pp64_ping();
}

//------------------------------------------------------------------------------

int command_execute(Command* self) {

  if(mode == MODE_HELP) {
    help(self->id);
    return true;
  }

  if(!command_parse_options(self)) {
    return false;
  }

  command_print(self);

  switch(self->id) {

  case COMMAND_NONE    : return command_none(self);
  case COMMAND_LOAD    : return command_load(self);
  case COMMAND_SAVE    : return command_save(self);
  case COMMAND_POKE    : return command_poke(self);
  case COMMAND_PEEK    : return command_peek(self);
  case COMMAND_JUMP    : return command_jump(self);
  case COMMAND_RUN     : return command_run(self);
  case COMMAND_RESET   : return command_reset(self);
  case COMMAND_HELP    : return command_help(self);
  case COMMAND_DOS     : return command_dos(self);
  case COMMAND_BACKUP  : return command_backup(self);
  case COMMAND_RESTORE : return command_restore(self);
  case COMMAND_VERIFY  : return command_verify(self);
  case COMMAND_STATUS  : return command_status(self);
  case COMMAND_READY   : return command_ready(self);
  case COMMAND_PING    : return command_ping(self);
  case COMMAND_TEST    : return command_test(self);
  }
  
  return false;
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {

  logger->enter(argv[0]);

  argc--; argv++;

  if (argc == 0) {
    usage();
    return EXIT_FAILURE;
  }
  
  if(argc == 1) {
    if (strncmp(argv[0], "help", 4) == 0) {
      usage();
      return EXIT_SUCCESS;
    } 

#if linux
    if (strncmp(argv[0], "shell", 5) == 0) {
      shell();
      return EXIT_SUCCESS;   
    }
#endif

  }

  Commands *commands = commands_new(argc, argv);

  int result = commands_execute(commands) ? EXIT_SUCCESS : EXIT_FAILURE;

  commands_free(commands);

  logger->leave();

  return result;
}

//------------------------------------------------------------------------------

#if linux
void shell(void) {

  extern char **completion_matches();

  static char* known_commands[16] = { 
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
    NULL };

  char *dupstr(char *s) {
    char *r = calloc(strlen(s) + 1, sizeof(char));
    strncpy(r, s, strlen(s));
    return r;
  }
  
  void trim(char * const a)
  {
    char *p = a, *q = a;
    while (isspace(*q)) ++q;
    while (*q) *p++ = *q++;
    *p = '\0';
    while (p > a && isspace(*--p)) *p = '\0';
  }

  char *command_generator(char *text, int state) {
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
  
  char **shell_completion(char *text, int start, int end) {
    return (char **) completion_matches(text, command_generator);
  }

  int shell_command(char *line) {
    if(strcmp(line, "help") == 0) {
      usage();
      return true;
    }
    
    if((strncmp(line, "quit", 4) == 0) ||
       (strncmp(line, "exit", 4) == 0)) {
      exit(EXIT_SUCCESS);
    }
    return false;
  }
  
  char *line;
  char *prompt = "c64> ";

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

void usage(void) {
    printf("pp64 client 0.4 Copyright (C) 2014 Henning Bekel <h.bekel@googlemail.com>\n");
    printf("\n");
    printf("Usage: c64 [<opts>] [<command> [<opts>] [<arguments>]]...\n");
    printf("\n");
    printf("Options:\n");
    printf("         -h, --help                    : show this help\n");
    printf("         -l, --level <level>           : log level (ERROR|WARN|INFO|DEBUG|TRACE)\n");
#if linux
    printf("         -d, --device <path>           : ");
    printf("transfer device (default: /dev/c64)\n");
#elif windows
    printf("         -d, --device <port or USB>    : ");
    printf("port address (default: 0x378)\n");
#endif
    printf("         -a, --address <start>[-<end>] : C64 address/range (default: autodetect)\n");
    printf("         -m, --memory                  : C64 memory config (default: 0x37)\n\n");
    
    printf("Commands:\n");
    printf("          help  [<command>]            : show detailed help for command\n");
#if linux
    printf("          shell                        : enter interactive command shell\n");
#endif
    printf("          ready                        : try to make sure the server is ready\n");
    printf("          reset                        : reset C64 (only if using reset circuit)\n");
    printf("\n");
    printf("          load  [<opts>] <file>        : load file into C64 memory\n");
    printf("          save  [<opts>] <file>        : save C64 memory to file\n");
    printf("          poke  [<opts>] <addr>,<val>  : poke value into C64 memory\n");
    printf("          peek  [<opts>] <addr>        : read value from C64 memory\n");
    printf("          jump  [<opts>] <addr>        : jump to specified address\n");
    printf("          run   [<opts>] [<file>]      : run program, optionally load it before\n");
    printf("\n");
    printf("          @[<dos-command>]             : read drive status or send dos command\n");    
    printf("          backup <file>                : backup disk to d64 file\n");
    printf("          restore <file>               : restore d64 file to disk\n");
    printf("          verify <file>                : verify disk against d64 file\n");
    printf("\n");
}

//------------------------------------------------------------------------------

void help(int id) {

  switch(id) {
  case COMMAND_NONE:
    usage();
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
    printf("Usage: c64 [<file>] run\n");
    printf("\n");
    printf("Without argument, RUN the currently loaded basic program. With a file argument\n");
    printf("specified, load the file beforehand. If the file loads to 0x0801, assume its a\n");
    printf("basic program and RUN it, else assume it's an ml program and jump to the\n");
    printf("address the file loaded to.\n");
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

  case COMMAND_PING:
    printf("Usage: c64 ping\n");
    printf("\n");
    printf("Ping the server, exit successfully if the server responds.\n");
    printf("\n");
    break;
  }
}


