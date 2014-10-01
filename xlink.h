#ifndef XLINK_H
#define XLINK_H

#include <stdbool.h>

#define XLINK_COMMAND_LOAD         0x01
#define XLINK_COMMAND_SAVE         0x02
#define XLINK_COMMAND_POKE         0x03
#define XLINK_COMMAND_PEEK         0x04
#define XLINK_COMMAND_JUMP         0x05
#define XLINK_COMMAND_RUN          0x06
#define XLINK_COMMAND_EXTEND       0x07
#define XLINK_COMMAND_IDENTIFY     0xfe 

#define XLINK_VERSION 0x10

#define XLINK_SERVER_TYPE_RAM 0x00
#define XLINK_SERVER_TYPE_ROM 0x01

#define XLINK_MACHINE_C64 0x00

#define XLINK_PING_TIMEOUT 250

typedef struct {
  unsigned char version;  // high byte mayor, low byte minor
  unsigned char machine;  // XLINK_MACHINE_XXX
  unsigned char type;     // XLINK_SERVER_TYPE_{RAM|ROM}
  unsigned short start;   // server start address
  unsigned short end;     // server end address
  unsigned short length;  // server code length
} XLinkServerInfo;

#ifdef __cplusplus
extern "C" {
#endif
  unsigned char xlink_version(void);

  bool xlink_set_device(char* path);
  char* xlink_get_device(void);
  bool xlink_has_device(void);

  bool xlink_identify(XLinkServerInfo*);

  bool xlink_ping(void);
  bool xlink_reset(void);
  bool xlink_ready(void);

  bool xlink_save(unsigned char memory, unsigned char bank, 
                  int start, int end, char* data, int size);

  bool xlink_load(unsigned char memory, unsigned char bank, 
                  int start, int end, char* data, int size);

  bool xlink_peek(unsigned char memory, unsigned char bank, 
                  int address, unsigned char* value);

  bool xlink_poke(unsigned char memory, unsigned char bank, 
                  int address, unsigned char value);
  
  bool xlink_jump(unsigned char memory, unsigned char bank, int address);

  bool xlink_run(void);

  bool xlink_drive_status(char* status);
  bool xlink_dos(char* cmd);
  bool xlink_sector_read(unsigned char track, unsigned char sector, unsigned char* data);
  bool xlink_sector_write(unsigned char track, unsigned char sector, unsigned char* data);

#ifdef __cplusplus
}
#endif

#endif // XLINK_H
