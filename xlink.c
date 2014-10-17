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

Driver* driver;

//------------------------------------------------------------------------------

unsigned char xlink_version(void) {
  return XLINK_VERSION;
}

//------------------------------------------------------------------------------

void xlink_set_debug(int level) {
  logger->level = level;
}

//------------------------------------------------------------------------------

void libxlink_initialize() {

  driver = (Driver*) calloc(1, sizeof(Driver));
  driver->path = (char*) calloc(1, sizeof(char));

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
  CLEAR_ERROR_IF(true);

  xlink_set_debug(XLINK_LOG_LEVEL_NONE);
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
  driver_setup(path, false);
  return xlink_has_device();
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

bool xlink_identify(xlink_server* server) {

  bool result = true;
  char data[7];
  
  if(driver->open()) {
    
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");

      driver->close();
      result = false;
      goto done;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_IDENTIFY}, 1);
    
    driver->input();
    driver->strobe();
    
    driver->receive(data, 7);

    driver->close();

    unsigned char checksum = 0xff;
    
    for(int i=0; i<7; i++) {
      checksum &= data[i];
    }
    if(checksum == 0xff) {
      SET_ERROR(XLINK_ERROR_SERVER, "unknown server (does not support identification)");

      result = false;
      goto done;
    }
    
    server->version = data[0];
    server->machine = data[1];
    server->type = data[2];
    
    server->start = 0;
    server->start |= data[3];
    server->start |= data[4] << 8;

    server->end = 0;
    server->end |= data[5];
    server->end |= data[6] << 8;
    
    server->length = server->end - server->start;
  }
  
 done:
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_ping() {
  
  bool result = false;

  if(driver->open()) {
    result = driver->ping();    
    driver->close();
  }

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

  if(!xlink_ping()) {
    xlink_reset();
    
    while(timeout) {
      if(xlink_ping()) {
        usleep(250*1000); // wait until basic is ready
        goto done;
      }
      timeout-=XLINK_PING_TIMEOUT;
    }
    result = false;
    goto done;
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
    result = true;
  }

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_load(unsigned char memory, 
		unsigned char bank, 
		int start, 
		int end, 
		char* data, 
		int size) {

  bool result = false;

  if(driver->open()) {
    
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;
    }

    driver->output();    
    driver->send((char []) {XLINK_COMMAND_LOAD, memory, bank, 
          lo(start), hi(start), lo(end), hi(end)}, 7);

    driver->send(data, size);
    
    result = true;
  }

 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_save(unsigned char memory, 
		unsigned char bank, 
		int start, 
		int end, 
		char* data, 
		int size) {

  bool result = false;
  
  if(driver->open()) {

    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_SAVE, memory, bank, 
          lo(start), hi(start), lo(end), hi(end)}, 7);

    driver->input();
    driver->strobe();

    driver->receive(data, size);

    result = true;
  }

 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_peek(unsigned char memory, 
		unsigned char bank, 
		int address, 
		unsigned char* value) {

  bool result = false;
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_PEEK, memory, bank, lo(address), hi(address)}, 5);
    
    driver->input();
    driver->strobe();

    driver->receive((char *)value, 1);

    result = true;
  }

 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_poke(unsigned char memory, 
		unsigned char bank, 
		int address, 
		unsigned char value) {

  bool result = false;
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;
    }
    
    driver->output();
    driver->send((char []) {XLINK_COMMAND_POKE, memory, bank, 
          lo(address), hi(address), value}, 6);    

    result = true;
  }

 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_jump(unsigned char memory, 
		unsigned char bank, 
		int address) {

  bool result = false;

  // jump address is send MSB first (big-endian)    

   if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_JUMP, memory, bank, 
          hi(address), lo(address)}, 5);    

    result = true;
  }
   
 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_run(void) {

  bool result = false;
  
   if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_RUN}, 1);

    result = true;
  }
   
 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_extend(int address) {

  bool result = false;
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      SET_ERROR(XLINK_ERROR_SERVER, "no response from server");
      goto done;      
    }
  
    // send the address-1 high byte first, so the server can 
    // just push it on the stack and rts
    
    address--;

    driver->output();
    driver->send((char []) {XLINK_COMMAND_EXTEND, hi(address), lo(address)}, 3);

    result = true;
  }
  
 done:
  driver->close();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool xlink_drive_status(char* status) {

  unsigned char byte;
  bool result = false;

  Extension *lib = EXTENSION_LIB;
  Extension *drive_status = EXTENSION_DRIVE_STATUS;

  if (extension_load(lib) && 
      extension_load(drive_status) && 
      extension_init(drive_status)) {

    if (driver->open()) {
      
      driver->input();
      driver->strobe();

      int i = 0;

      while(true) {

        driver->receive((char *) &byte, 1);

        if(byte == 0xff) break;

        status[i++] = byte;	
      }
      driver->wait(0);

      driver->close();
      result = true;
    }
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

  if(extension_load(lib) && 
     extension_load(dos_command) && 
     extension_init(dos_command)) {
    
    if(driver->open()) {   

      driver->output();
      driver->send((char []) {strlen(command)}, 1);
      driver->send(command, strlen(command));

      driver->wait(0);

      driver->close();
      result = true;
    }
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

  if (extension_load(lib) && 
      extension_load(sector_read) && 
      extension_init(sector_read)) {

    if (driver->open()) {
      
      sprintf(U1, "U1 2 0 %02d %02d", track, sector);

      driver->output();
      driver->send(U1, strlen(U1));

      driver->input();
      driver->strobe();

      driver->receive((char *) data, 256);
      driver->wait(0);
      
      driver->close();      
      result = true;
    }
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

  if (extension_load(lib) && 
      extension_load(sector_write) && 
      extension_init(sector_write)) {

    if (driver->open()) {
      sprintf(U2, "U2 2 0 %02d %02d", track, sector);

      driver->output();
      driver->send((char *) data, 256);
      driver->send(U2, strlen(U2));

      driver->wait(0);
      
      driver->close();
      result = true;
    }
  }
  
  extension_free(lib);
  extension_free(sector_write);

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------


