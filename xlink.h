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

  typedef unsigned char uchar;
  typedef unsigned short ushort;
  typedef unsigned int uint;
  
  typedef struct {
    uchar version;  // high byte major, low byte minor
    uchar machine;  // XLINK_MACHINE_C64
    uchar type;     // XLINK_SERVER_TYPE_{RAM|ROM}
    ushort start;   // server start address
    ushort end;     // server end address
    ushort length;  // server code length
    ushort memtop;  // current top of (lower) memory (0xa000 or 0x8000)
  } xlink_server_info;

  typedef struct {
    int code;
    char message[512];
  } xlink_error_t;

  
  IMPORTED extern xlink_error_t* xlink_error;
  
  uchar xlink_version(void);
  void xlink_set_debug(bool enabled);

  bool xlink_has_device(void);  
  bool xlink_set_device(char* path);
  char* xlink_get_device(void);

  bool xlink_identify(xlink_server_info* server);
  bool xlink_relocate(ushort address);

  bool xlink_ping(void);
  bool xlink_reset(void);
  bool xlink_ready(void);

  bool xlink_load(uchar memory, uchar bank, ushort address, uchar* data, uint size);  
  bool xlink_save(uchar memory, uchar bank, ushort address, uchar* data, uint size);
  bool xlink_peek(uchar memory, uchar bank, ushort address, uchar* value);
  bool xlink_poke(uchar memory, uchar bank, ushort address, uchar value);
  bool xlink_jump(uchar memory, uchar bank, ushort address);
  bool xlink_run(void);
    
#ifdef __cplusplus
}
#endif

#endif // XLINK_H
