#ifndef PP64_H
#define PP64_H

#include <stdbool.h>

#define PP64_PING_TIMEOUT 250

#define PP64_COMMAND_LOAD         0x01
#define PP64_COMMAND_SAVE         0x02
#define PP64_COMMAND_POKE         0x03
#define PP64_COMMAND_PEEK         0x04
#define PP64_COMMAND_JUMP         0x05
#define PP64_COMMAND_RUN          0x06
#define PP64_COMMAND_EXTEND       0x07

#ifdef __cplusplus
extern "C" {
#endif
  bool pp64_set_device(char* path);
  bool pp64_has_device();

  bool pp64_ping(void);
  bool pp64_reset(void);
  bool pp64_load(unsigned char memory, unsigned char bank, int start, int end, char* data, int size);
  bool pp64_save(unsigned char memory, unsigned char bank, int start, int end, char* data, int size);
  bool pp64_peek(unsigned char memory, unsigned char bank, int address, unsigned char* value);
  bool pp64_poke(unsigned char memory, unsigned char bank, int address, unsigned char value);
  bool pp64_jump(unsigned char memory, unsigned char bank, int address);
  bool pp64_run(void);

  bool pp64_drive_status(char* status);
  bool pp64_dos(char* cmd);
  bool pp64_sector_read(unsigned char track, unsigned char sector, unsigned char* data);
  bool pp64_sector_write(unsigned char track, unsigned char sector, unsigned char* data);

  bool pp64_test(char *test);
#ifdef __cplusplus
}
#endif

#endif // PP64_H
