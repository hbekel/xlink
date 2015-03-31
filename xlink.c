#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "xlink.h"
#include "error.h"
#include "target.h"
#include "driver/driver.h"
#include "extension.h"
#include "extensions.c"
#include "util.h"

#if windows
  #include <windows.h>
#endif

#define XLINK_COMMAND_LOAD     0x01
#define XLINK_COMMAND_SAVE     0x02
#define XLINK_COMMAND_POKE     0x03
#define XLINK_COMMAND_PEEK     0x04
#define XLINK_COMMAND_JUMP     0x05
#define XLINK_COMMAND_RUN      0x06
#define XLINK_COMMAND_INJECT   0x07
#define XLINK_COMMAND_IDENTIFY 0xfe 
#define XLINK_DEFAULT_TIMEOUT  0x01

Driver* driver;
xlink_error_t* xlink_error;

//------------------------------------------------------------------------------

unsigned char xlink_version(void) {
  return XLINK_VERSION;
}

//------------------------------------------------------------------------------

void xlink_set_debug(bool enabled) {
  logger->level = enabled ? LOGLEVEL_ALL : LOGLEVEL_NONE;
}

//------------------------------------------------------------------------------

void libxlink_initialize() {

  driver = (Driver*) calloc(1, sizeof(Driver));
  driver->path = (char*) calloc(1, sizeof(char));
  driver->timeout = XLINK_DEFAULT_TIMEOUT;

  driver->ready   = &_driver_ready;
  driver->open    = &_driver_open;
  driver->close   = &_driver_close;
  driver->strobe  = &_driver_strobe;
  driver->wait    = &_driver_wait;
  driver->read    = &_driver_read;
  driver->write   = &_driver_write;
  driver->send    = &_driver_send;
  driver->receive = &_driver_receive;
  driver->input   = &_driver_input;
  driver->output  = &_driver_output;
  driver->ping    = &_driver_ping;
  driver->reset   = &_driver_reset;
  driver->boot    = &_driver_boot;
  driver->free    = &_driver_free;

  driver->_open = &_driver_setup_and_open;

  xlink_error = (xlink_error_t *) calloc(1, sizeof(xlink_error_t));
  CLEAR_ERROR;

  xlink_set_debug(false);
}

//------------------------------------------------------------------------------

void libxlink_finalize(void) {
  
  if(driver != NULL) {
    driver->free();
  }

  free(xlink_error);
  logger->free();
}

//------------------------------------------------------------------------------

#if windows
BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved ) {
  switch(nReason) {

   case DLL_PROCESS_ATTACH:
     DisableThreadLibraryCalls(hDllHandle);
     libxlink_initialize();
     break;
 
   case DLL_PROCESS_DETACH:
     libxlink_finalize();
     break;
  }
  return true;
}
#endif

//------------------------------------------------------------------------------

bool xlink_set_device(char* path) {
  return driver_setup(path);
}  

//------------------------------------------------------------------------------

char* xlink_get_device(void) {
  return driver->path;
}

//------------------------------------------------------------------------------

bool xlink_has_device(void) {
  bool result;
  
  logger->suspend();
  result = driver->ready();
  logger->resume();

  return result;
}

//------------------------------------------------------------------------------

bool xlink_identify(xlink_server_info* server) {

  bool result = false;
  unsigned char data[9];
  unsigned char size;
  
  if(driver->open()) {
    
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }

    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_IDENTIFY}, 1)) goto error;
    
    driver->input();
    driver->strobe();

    if(!driver->receive(&size, 1)) goto error;
    size &= 0x0f;

    if(!driver->receive((uchar*) (server->id), size)) goto error;    
    if(!driver->receive(data, 9)) goto error;

    unsigned char checksum = 0xff;
    
    for(int i=0; i<9; i++) {
      checksum &= data[i];
    }
    if(checksum == 0xff) {
      SET_ERROR(XLINK_ERROR_SERVER, "unknown server (does not support identification)");
      goto error;
    }
    
    server->id[size] = '\0';
    
    server->version = data[0];
    server->machine = data[1];
    server->type = data[2];
    
    server->start = 0;
    server->start |= data[3];
    server->start |= data[4] << 8;

    server->end = 0;
    server->end |= data[5];
    server->end |= data[6] << 8;

    server->memtop = 0;
    server->memtop |= data[7];
    server->memtop |= data[8] << 8;
    
    server->length = server->end - server->start;   

    driver->close();
    result = true;
  }
  
 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;

}

//------------------------------------------------------------------------------

bool xlink_ping() {
  
  bool result = false;

  if(driver->open()) {
    result = driver->ping();    
    driver->close();
  }

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_reset(void) {
  bool result = false;
  
  if(driver->open()) {
    driver->reset();
    driver->close();
    result = true;
  }
  
  CLEAR_ERROR_IF(result);
  return result;
};

//------------------------------------------------------------------------------

bool xlink_ready(void) {

  bool result = true;
  int timeout = 3000;
  unsigned char mode;

  if(!driver->ready()) {
    result = false;
    goto done;
  }

  if(!xlink_ping()) {
    xlink_reset();
    
    while(timeout) {
      if(xlink_ping()) {
        usleep(250*1000); // wait until basic is ready
        goto done;
      }
      timeout-=250;
    }
    result = false;
  }
  else {
    if(xlink_peek(0x37, 0x00, 0x9d, &mode)) {
      if(mode != 0x80) { 
        // basic program running -> warm start basic
        xlink_jump(0x37, 0x00, 0xfe66);
        usleep(250*1000);
      }
    }
  }
  
 done:
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_bootloader(void) {
  bool result = false;
  
  if(driver->open()) {
    driver->boot();
    driver->close();
    result = true;
  }

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_load(unsigned char memory, 
                unsigned char bank, 
                unsigned short address, 
                unsigned char* data,
                unsigned int size) {

  bool result = false;
  unsigned short start = address;
  unsigned short end = start + size;

  if(driver->open()) {
    
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }

    driver->output();    
    if(!driver->send((unsigned char []) {XLINK_COMMAND_LOAD, memory, bank, 
            lo(start), hi(start), lo(end), hi(end)}, 7)) goto error;

    if(!driver->send(data, size)) goto error;

    driver->close();
    result = true;
  }

 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_save(unsigned char memory, 
                unsigned char bank, 
                unsigned short address, 
                unsigned char* data,
		unsigned int size) {
  
  bool result = false;
  unsigned short start = address;
  unsigned short end = start + size;

  if(driver->open()) {

    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }

    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_SAVE, memory, bank, 
            lo(start), hi(start), lo(end), hi(end)}, 7)) goto error;

    driver->input();
    driver->strobe();

    if(!driver->receive(data, size)) goto error;

    driver->close();
    result = true;
  }

 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_peek(unsigned char memory, 
		unsigned char bank, 
		unsigned short address, 
		unsigned char* value) {

  bool result = false;
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }

    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_PEEK,
            memory, bank, lo(address), hi(address)}, 5)) goto error;
    
    driver->input();
    driver->strobe();

    if(!driver->receive(value, 1)) goto error;

    driver->close();
    result = true;
  }

 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_poke(unsigned char memory, 
		unsigned char bank, 
		unsigned short address, 
		unsigned char value) {

  bool result = false;
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }
    
    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_POKE, memory, bank, 
            lo(address), hi(address), value}, 6)) goto error;    

    driver->close();
    result = true;
  }

 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_fill(unsigned char memory,
		unsigned char bank,
		unsigned short address,
		unsigned char value,
		unsigned int size) {

  bool result = false;

  uchar* data = (uchar*) calloc(size, sizeof(uchar));
  memset(data, value, size);

  result = xlink_load(memory, bank, address, data, size);

  free(data);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_jump(unsigned char memory, 
		unsigned char bank, 
		unsigned short address) {

  bool result = false;

  // jump address is send MSB first (big-endian)    

  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }
    
    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_JUMP, memory, bank, 
          hi(address), lo(address)}, 5)) goto error;

    driver->close();    
    result = true;
  }
  
 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_run(void) {

  bool result = false;
  
   if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;
    }

    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_RUN}, 1)) goto error;

    driver->close();
    result = true;
  }
   
 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_inject(ushort address, uchar* code, uint size) {

  bool result = false;
  uchar memory = 0x80|0x37;
  uchar bank = 0x00;
  
  uchar *cache = (uchar*) calloc(size, sizeof(uchar));
  
  if(!xlink_load(memory, bank, address, code, size)) {
    goto done;
  }
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto error;      
    }
  
    // send the address-1 high byte first, so the server can 
    // just push it on the stack and rts
    
    address--;

    driver->output();
    if(!driver->send((unsigned char []) {XLINK_COMMAND_INJECT,
            hi(address), lo(address)}, 3)) goto error;
    
    driver->close();
    result = true;
  }
  
 done:
  free(cache);
  
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_send(uchar* data, uint size) {

  bool result = false;

  if(driver->open()) {

    driver->output();
    if(!driver->send(data, size)) goto error;
    driver->close();
    result = true;
  }

 done:  
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_send_with_timeout(uchar* data, uint size, uint timeout) {
  bool result = false;
  
  driver->timeout = timeout;
  result = xlink_send(data, size);
  driver->timeout = XLINK_DEFAULT_TIMEOUT;

  return result;
}

//------------------------------------------------------------------------------

bool xlink_receive(uchar *data, uint size) {
  bool result = false;

  if(driver->open()) {

    driver->input();
    driver->strobe();
    if(!driver->receive(data, size)) goto error;
    driver->close();
    result = true;
  }

 done:
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

bool xlink_receive_with_timeout(uchar* data, uint size, uint timeout) {
  bool result = false;
  
  driver->timeout = timeout;
  result = xlink_receive(data, size);
  driver->timeout = XLINK_DEFAULT_TIMEOUT;

  return result;
}

//------------------------------------------------------------------------------

extern unsigned char* xlink_server(unsigned short address, int *size);

bool xlink_relocate(unsigned short address) {

  bool result = false;
  
  Extension* relocate = EXTENSION_SERVER_RELOCATE;

  int size;
  unsigned char* server = xlink_server(address, &size);  

  if(extension_init(relocate)) {
    result = xlink_load(0x37|0x80, 0x00, address, server+2, size-2);
  }
  
  extension_free(relocate);  
  free(server);
  
  CLEAR_ERROR_IF(result);
  return result;  
}

//------------------------------------------------------------------------------

bool xlink_drive_status(char* status) {

  unsigned char byte;
  bool result = false;

  Extension *lib = EXTENSION_LIB;
  Extension *drive_status = EXTENSION_DRIVE_STATUS;

  if (extension_preload(lib) && 
      extension_init(drive_status)) {

    if (driver->open()) {
      
      driver->input();
      driver->strobe();

      int i = 0;

      while(true) {

        driver->receive(&byte, 1);

        if(byte == 0xff) break;

        status[i++] = byte;	
      }
      driver->wait(0);

      driver->close();
      result = true;
    }
    extension_unload(lib);
  }

  extension_free(lib);
  extension_free(drive_status);
  
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_dos(char* cmd) {

  bool result = false;

  Extension *lib = EXTENSION_LIB;
  Extension *dos_command = EXTENSION_DOS_COMMAND;

  char *command = (char *) calloc(strlen(cmd)+1, sizeof(char));
  
  for(int i=0; i<strlen(cmd); i++) {
    command[i] = toupper(cmd[i]);
  }      

  if(extension_preload(lib) && 
     extension_init(dos_command)) {
    
    if(driver->open()) {   

      driver->output();
      driver->send((unsigned char []) {strlen(command)}, 1);
      driver->send((unsigned char*) command, strlen(command));

      driver->wait(0);

      driver->close();
      result = true;
    }
    extension_unload(lib);
  }
  
  free(command);
  extension_free(lib);
  extension_free(dos_command);

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_sector_read(unsigned char track, unsigned char sector, unsigned char* data) {
  
  bool result = false;
  char U1[13];

  Extension *lib = EXTENSION_LIB;
  Extension *sector_read = EXTENSION_SECTOR_READ;

  if (extension_preload(lib) && 
      extension_init(sector_read)) {

    if (driver->open()) {
      
      sprintf(U1, "U1 2 0 %02d %02d", track, sector);

      driver->output();
      driver->send((unsigned char*)U1, strlen(U1));

      driver->input();
      driver->strobe();

      driver->receive(data, 256);
      driver->wait(0);
      
      driver->close();      
      result = true;
    }
    extension_unload(lib);
  }

  extension_free(lib);
  extension_free(sector_read);

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_sector_write(unsigned char track, unsigned char sector, unsigned char *data) {

  int result = false;
  char U2[13];

  Extension *lib = EXTENSION_LIB;
  Extension *sector_write = EXTENSION_SECTOR_WRITE;

  if (extension_preload(lib) && 
      extension_init(sector_write)) {

    if (driver->open()) {
      sprintf(U2, "U2 2 0 %02d %02d", track, sector);

      driver->output();
      driver->send(data, 256);
      driver->send((unsigned char*)U2, strlen(U2));

      driver->wait(0);
      
      driver->close();
      result = true;
    }
    extension_unload(lib);
  }
  
  extension_free(lib);
  extension_free(sector_write);

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------
