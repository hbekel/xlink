#ifndef XLINK_H
#define XLINK_H

#include <stdbool.h>

#if defined(WIN32) || defined(__CYGWIN__)
  #if defined(XLINK_LIBRARY_BUILD)
    #define IMPORTED
  #else
    #define IMPORTED __declspec(dllimport)
  #endif
#else
  #define IMPORTED
#endif

#define XLINK_VERSION          0x10
#define XLINK_SERVER_TYPE_RAM  0x00
#define XLINK_SERVER_TYPE_ROM  0x01
#define XLINK_MACHINE_C64      0x00

#define XLINK_SUCCESS          0x00
#define XLINK_ERROR_DEVICE     0x01
#define XLINK_ERROR_LIBUSB     0x02
#define XLINK_ERROR_PARPORT    0x03
#define XLINK_ERROR_SERVER     0x04

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
    unsigned short memtop;  // current top of (lower) memory (0xa000 or 0x8000)
  } xlink_server_info;

  typedef struct {
    int code;
    char message[512];
  } xlink_error_t;

  IMPORTED xlink_error_t* xlink_error;
  
  unsigned char xlink_version(void);
  void xlink_set_debug(bool enabled);  

  bool xlink_has_device(void);  
  bool xlink_set_device(char* path);
  char* xlink_get_device(void);

  void xlink_kernal(unsigned char* image);
  unsigned char* xlink_server_basic(int *size);
  unsigned char* xlink_server(unsigned short address, int *size);
  bool xlink_relocate(unsigned short address);

  bool xlink_identify(xlink_server_info* server);
  bool xlink_ping(void);
  bool xlink_reset(void);
  bool xlink_ready(void);

  bool xlink_load(unsigned char memory, unsigned char bank, 
                  unsigned short start, unsigned short end,
                  unsigned char* data, int size);  

  bool xlink_save(unsigned char memory, unsigned char bank, 
                  unsigned short start, unsigned short end,
                  unsigned char* data, int size);

  bool xlink_peek(unsigned char memory, unsigned char bank, 
                  unsigned short address, unsigned char* value);

  bool xlink_poke(unsigned char memory, unsigned char bank, 
                  unsigned short address, unsigned char value);
  
  bool xlink_jump(unsigned char memory, unsigned char bank,
                  unsigned short address);

  bool xlink_run(void);
  
  bool xlink_drive_status(char* status);
  bool xlink_dos(char* cmd);
  bool xlink_sector_read(unsigned char track, unsigned char sector, unsigned char* data);
  bool xlink_sector_write(unsigned char track, unsigned char sector, unsigned char* data);
  
#ifdef __cplusplus
}
#endif

#endif // XLINK_H
