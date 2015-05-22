#ifndef CLIENT_H
#define CLIENT_H

#include "xlink.h"

typedef struct {
  char id;
  char *name;
  unsigned char memory;
  unsigned char bank;
  int start;
  int end;
  int skip;
  int debug;
  int argc;
  char **argv; 
  int offset;
} Command;

typedef struct {
  int count;
  Command **items;
} Commands;

char str2id(const char* arg);
char* id2str(const char id);
int valid(int address);
void screenOn(void);
void screenOff(void);
void version(void);
void usage(void);
void shell(void);
bool help(int id);

Commands* commands_new(int argc, char **argv);
Command* commands_add(Commands* self, Command* command);
bool commands_each(Commands* self, bool (*func) (Command* command));
bool commands_execute(Commands* self);
void commands_print(Commands* self);
void commands_free(Commands* self);

bool command_server_usable_after_possible_relocation(Command* self);
bool command_requires_server_relocation(Command* self, xlink_server_info_t* server);
bool command_server_relocation_possible(Command* self, xlink_server_info_t* server, unsigned short* address);

Command* command_new(int *argc, char ***argv);
int command_arity(Command* self);
void command_consume_arguments(Command *self, int *argc, char ***argv);
void command_append_argument(Command* self, char* arg);
bool command_parse_options(Command *self);
char *command_get_name(Command* self);
bool command_print(Command* self);
bool command_find_basic_program(Command* self);
void command_apply_memory_and_bank(Command* self);
void command_apply_safe_memory_and_bank(Command* self);
bool command_execute(Command* self);
bool command_none(Command* self);
bool command_load(Command* self);
bool command_save(Command* self);
bool command_poke(Command* self);
bool command_peek(Command* self);
bool command_fill(Command* self);
bool command_jump(Command* self);
bool command_run(Command* self);
bool command_status(Command* self);
bool command_dos(Command* self);
bool command_backup(Command* self);
bool command_restore(Command* self);
bool command_verify(Command* self);
bool command_ready(Command* self);
bool command_ping(Command* self);
bool command_reset(Command* self);
bool command_wait(Command* self);
bool command_help(Command* self);
bool command_bootstrap(Command *self);
bool command_benchmark(Command *self);
bool command_identify(Command *self);
bool command_server(Command *self);
bool command_relocate(Command *self);
bool command_kernal(Command *self);
void command_free(Command* self);

#define NOT_IMPLEMENTED_FOR_C128 if(machine->type == XLINK_MACHINE_C128) { logger->error("not implemented for the C128 (yet)"); return false; }

#if windows
void handle(int signal);
#endif

#endif // CLIENT_H
