#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "xlink.h"
#include "machine.h"
#include "error.h"
#include "target.h"
#include "driver/driver.h"
#include "extension.h"
#include "extensions.c"
#include "util.h"

#if windows
  #include <windows.h>
#endif

#define XLINK_DEFAULT_TIMEOUT  0x01
#define XLINK_GO64 0xff4d

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
  driver->state = XLINK_DRIVER_STATE_IDLE;

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

  if (getenv("XLINK_MACHINE") != NULL) {

    if (strncmp(getenv("XLINK_MACHINE"), "c64", 3) == 0) {
      machine = &c64;
    }
    if (strncmp(getenv("XLINK_MACHINE"), "c128", 4) == 0) {
      machine = &c128;
    }
  }
  
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

bool xlink_identify(xlink_server_info_t* server) {

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
    return true;
  }
 
  CLEAR_ERROR_IF(result);
  return result;
};

//------------------------------------------------------------------------------

static bool server_ready_after(int ms) {

  logger->trace("waiting for server...");
  
  while(ms) {
    if(xlink_ping()) {
      usleep(250*1000);
      return true;
    }
    ms-=250;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_ready(void) {

  bool result = true;
  int timeout = 3000;
  
  xlink_server_info_t remote;
  unsigned char mode;

  if(!driver->ready()) {
    result = false;
    goto done;
  }

  if(!xlink_ping()) {
    logger->trace("server not reachable, resetting...");
    xlink_reset();

    if(!(result = server_ready_after(timeout))) {
      goto done;
    }    
  }

  if(machine->type == XLINK_MACHINE_C64) {
    if((result = xlink_identify(&remote))) {
      if(remote.machine == XLINK_MACHINE_C128) {
	logger->trace("C128 server identified, switching to C64 mode");
	if((result = xlink_jump(c128.memory, c128.bank, XLINK_GO64))) {
	  result = server_ready_after(timeout);
	  goto done;	  
	}
      }
    }
  }

  if(machine->type == XLINK_MACHINE_C128) {
    if((result = xlink_identify(&remote))) {
      if(remote.machine == XLINK_MACHINE_C64) {
	logger->trace("C64 server identified, switching to C128 mode");
	if((result = xlink_reset())) {
	  result = server_ready_after(timeout);
	  goto done;	  
	}
      }
    }
  }
  
  if(result) {
    if((result = xlink_peek(machine->memory, machine->bank, machine->mode, &mode))) {
      if(mode == machine->prgmode) {
	logger->debug("basic program running, performing basic warmstart...");
	if((result = xlink_jump(machine->memory, machine->bank, machine->warmstart))) {
	  usleep(250*1000);
	}
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
  uchar memory = machine->memory;
  uchar bank = machine->bank;
  
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
  CLEAR_ERROR_IF(result);
  return result;

 error:
  driver->close();
  goto done;
}

//------------------------------------------------------------------------------

void xlink_begin() {
  driver->state = XLINK_DRIVER_STATE_IDLE;
}

//------------------------------------------------------------------------------

bool xlink_send(uchar* data, uint size) {

  bool result = false;

  if(driver->open()) {

    if(driver->state == XLINK_DRIVER_STATE_INPUT) {
      driver->wait(0);
    }
    driver->output();
    driver->state = XLINK_DRIVER_STATE_OUTPUT;

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

    if(driver->state == XLINK_DRIVER_STATE_OUTPUT) {
      driver->strobe();
      driver->state = XLINK_DRIVER_STATE_INPUT;
    }

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

void xlink_end() {
  driver->state = XLINK_DRIVER_STATE_IDLE;
}

//------------------------------------------------------------------------------

bool xlink_relocate(unsigned short address) {

  bool result = false;
  
  int size;
  unsigned char* server = machine->server(address, &size);  

  uchar memory = machine->memory | 0x80;
  uchar bank = machine->bank;
  
  if(!(result = xlink_load(memory, bank, address, server+2, size-2))) goto done;

  result = xlink_jump(memory, bank, address);
  
  free(server);

 done:
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

      driver->timeout = 5;
      
      sprintf(U1, "U1 2 0 %02d %02d", track, sector);

      driver->output();
      driver->send((unsigned char*)U1, strlen(U1));

      driver->input();
      driver->strobe();

      driver->receive(data, 256);
      
      driver->wait(0);

      driver->timeout = XLINK_DEFAULT_TIMEOUT;
      
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

  if (extension_preload(lib) && 
      extension_init(sector_write)) {

    if (driver->open()) {

      driver->timeout = 5;
      
      sprintf(U2, "U2 2 0 %02d %02d", track, sector);

      driver->output();
      driver->send(data, 256);
      driver->send((unsigned char*)U2, strlen(U2));

      driver->wait(0);

      driver->timeout = XLINK_DEFAULT_TIMEOUT;
      
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
