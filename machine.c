#include "machine.h"

extern unsigned char* xlink_server_c64(unsigned short address, int *size);
extern unsigned char* xlink_server_basic_c64(int *size);

extern unsigned char* xlink_server_c128(unsigned short address, int *size);
extern unsigned char* xlink_server_basic_c128(int *size);

xlink_machine_t c64 = {
  .type                = XLINK_MACHINE_C64,
  .name                = "c64",
  .memory              = 0x37,
  .bank                = 0x00,
  .safe_memory         = 0x33,
  .safe_bank           = 0x00,
  .default_basic_start = 0x0801,
  .basic_start         = 0x2b,
  .basic_end           = 0x2d,
  .warmstart           = 0xfe66,  
  .mode                = 0x9d,
  .io                  = 0x0c000d00,
  .benchmark           = 0x10008000,
  .free_ram_area       = 0xc000,
  .server              = &xlink_server_c64,
  .basic_server        = &xlink_server_basic_c64,
};

xlink_machine_t c128 = {
  .type                = XLINK_MACHINE_C128,
  .name                = "c128",
  .memory              = 0x00,
  .bank                = 0x0f,
  .safe_memory         = 0x01,
  .safe_bank           = 0x0e,
  .default_basic_start = 0x1c01,
  .basic_start         = 0x2d,
  .basic_end           = 0x1210,
  .warmstart           = 0x4003,
  .mode                = 0x7f,
  .io                  = 0x0c000d00,
  .benchmark           = 0x20004000,
  .free_ram_area       = 0x1300,
  .server              = &xlink_server_c128,
  .basic_server        = &xlink_server_basic_c128,
};

xlink_machine_t *machine = &c64;
