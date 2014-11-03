#ifndef CLIENT_H
#define CLIENT_H

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
int help(int id);

Commands* commands_new(int argc, char **argv);
Command* commands_add(Commands* self, Command* command);
int commands_each(Commands* self, int (*func) (Command* command));
int commands_execute(Commands* self);
void commands_print(Commands* self);
void commands_free(Commands* self);

Command* command_new(int *argc, char ***argv);
int command_arity(Command* self);
void command_consume_arguments(Command *self, int *argc, char ***argv);
void command_append_argument(Command* self, char* arg);
int command_parse_options(Command *self);
char *command_get_name(Command* self);
int command_print(Command* self);
int command_find_basic_program(Command* self);
int command_execute(Command* self);
int command_none(Command* self);
int command_load(Command* self);
int command_save(Command* self);
int command_poke(Command* self);
int command_peek(Command* self);
int command_jump(Command* self);
int command_run(Command* self);
int command_status(Command* self);
int command_dos(Command* self);
int command_backup(Command* self);
int command_restore(Command* self);
int command_verify(Command* self);
int command_ready(Command* self);
int command_ping(Command* self);
int command_reset(Command* self);
int command_wait(Command* self);
int command_help(Command* self);
int command_bootstrap(Command *self);
int command_benchmark(Command *self);
int command_identify(Command *self);
int command_server(Command *self);
void command_free(Command* self);

#if windows
void handle(int signal);
#endif

#endif // CLIENT_H
