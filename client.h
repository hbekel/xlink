typedef struct {
  char id;
  unsigned char memory;
  unsigned char bank;
  int start;
  int end;
  int debug;
  int argc;
  char **argv;
} Command;

typedef struct {
  int count;
  Command **items;
} Commands;

char str2id(const char* arg);
char* id2str(const char id);
int valid(int address);
void version(void);
void usage(int id);

Commands* commands_new(void);
Command* commands_add(Commands* self, Command* command);

Command* command_new(char id);
void command_append_argument(Command* self, char* arg);
int command_parse_options(Command *self);
char *command_get_name(Command* self);
int command_find_basic_program(Command* self);
int command_dispatch(Command* self);
int command_auto(Command* self);
int command_load(Command* self);
int command_save(Command* self);
int command_poke(Command* self);
int command_peek(Command* self);
int command_jump(Command* self);
int command_run(Command* self);
int command_reset(Command* self);
int command_wait(Command* self);
int command_help(Command* self);
void command_print(Command* self);

#if windows
void handle(int signal);
#endif
