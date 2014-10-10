#ifndef XLINK_H
#define XLINK_H

#include <stdbool.h>

#define XLINK_VERSION 0x10

#define XLINK_SUCCESS 0x00

#define XLINK_LOG_LEVEL_NONE  0x00
#define XLINK_LOG_LEVEL_ERROR 0x01
#define XLINK_LOG_LEVEL_WARN  0x02
#define XLINK_LOG_LEVEL_INFO  0x03
#define XLINK_LOG_LEVEL_DEBUG 0x04
#define XLINK_LOG_LEVEL_TRACE 0x05
#define XLINK_LOG_LEVEL_ALL   0x06

#define XLINK_COMMAND_LOAD     0x01
#define XLINK_COMMAND_SAVE     0x02
#define XLINK_COMMAND_POKE     0x03
#define XLINK_COMMAND_PEEK     0x04
#define XLINK_COMMAND_JUMP     0x05
#define XLINK_COMMAND_RUN      0x06
#define XLINK_COMMAND_EXTEND   0x07
#define XLINK_COMMAND_IDENTIFY 0xfe 

#define XLINK_SERVER_TYPE_RAM 0x00
#define XLINK_SERVER_TYPE_ROM 0x01

#define XLINK_MACHINE_C64 0x00

#define XLINK_PING_TIMEOUT 250

#if defined(XLINK_LIBRARY_BUILD)
#define IMPORT
#else
#define IMPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct {
    unsigned char version;  // high byte mayor, low byte minor
    unsigned char machine;  // XLINK_MACHINE_C64
    unsigned char type;     // XLINK_SERVER_TYPE_{RAM|ROM}
    unsigned short start;   // server start address
    unsigned short end;     // server end address
    unsigned short length;  // server code length
  } xlink_server;

  typedef struct {
    int code;
    char message[512];
  } xlink_error_t;

  IMPORT xlink_error_t* xlink_error;
  
  unsigned char xlink_version(void);
  void xlink_set_debug(int level);  

  bool xlink_has_device(void);  
  bool xlink_set_device(char* path);
  char* xlink_get_device(void);

  bool xlink_identify(xlink_server* server);
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
