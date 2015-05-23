#ifndef PRIVATE_H
#define PRIVATE_H

#if defined(WIN32) || defined(__CYGWIN__)
  #if defined(XLINK_LIBRARY_BUILD)
    #define IMPORTED
  #else
    #define IMPORTED __declspec(dllimport)
  #endif
#else
  #define IMPORTED
#endif

#include "xlink.h"

typedef struct {
  int type;
  char name[256];
  uchar memory;
  uchar bank;
  uchar safe_memory;
  uchar safe_bank;
  ushort default_basic_start;
  ushort basic_start;
  ushort basic_end;
  ushort warmstart;
  ushort mode;
  uint io;
  uint screenram;
  uint loram;
  uint hiram;
  uint lorom;
  uint hirom;
  uint benchmark;
  ushort free_ram_area;
  uchar* (*server) (ushort address, int *size);
  uchar* (*basic_server) (int *size);
  void (*kernal) (uchar *image);
} xlink_machine_t;

IMPORTED extern xlink_machine_t c64;
IMPORTED extern xlink_machine_t c128;
IMPORTED extern xlink_machine_t *machine;

#endif // PRIVATE_H
