#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "xlink.h"
#include "target.h"
#include "driver/driver.h"
#include "extension.h"
#include "extensions.c"
#include "util.h"

#if windows
  #include <windows.h>
#endif

extern Driver* driver;

//------------------------------------------------------------------------------

unsigned char xlink_version(void) {
  return XLINK_VERSION;
}

//------------------------------------------------------------------------------

void libxlink_initialize() {

#if linux
  char default_usb_device[] = "/dev/xlink";
  char default_parport_device[] = "/dev/parport0";

#elif windows
  char default_usb_device[] = "";
  char default_parport_device[] = "0x378";
#endif

  logger->suspend();

  if (getenv("XLINK_DEVICE") != NULL) {
    driver_setup(getenv("XLINK_DEVICE"));
  
  } else {

    driver_setup(default_usb_device);

    if (!driver->ready()) {
      driver_setup(default_parport_device);
    }
  }
  
  logger->resume();
}

//------------------------------------------------------------------------------

void libxlink_finalize(void) {
  if(driver != NULL) {
    driver->free();
  }
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
  driver_setup(path);
  return xlink_has_device();
}  

//------------------------------------------------------------------------------

char* xlink_get_device(void) {
  return driver->path;
}

//------------------------------------------------------------------------------

bool xlink_has_device(void) {
  return driver->ready();
}

//------------------------------------------------------------------------------

bool xlink_identify(XLinkServerInfo* server) {

  char data[7];

  if(driver->open()) {
    
    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_IDENTIFY}, 1);
    
    driver->input();
    driver->strobe();
    
    driver->receive(data, 7);

    driver->close();

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
    return true;
  }
  return false;
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

  if(driver->open()) {
    driver->reset();
    driver->close();
    return true;
  }
  return false;
};

//------------------------------------------------------------------------------

bool xlink_ready(void) {

  int timeout = 3000;

  if(!xlink_ping()) {
    xlink_reset();
    
    while(timeout) {
      if(xlink_ping()) {
        usleep(250*1000); // wait until basic is ready
        return true;
      }
      timeout-=XLINK_PING_TIMEOUT;
    }
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------

bool xlink_bootloader(void) {
  
  if(driver->open()) {
    driver->boot();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_load(unsigned char memory, 
		unsigned char bank, 
		int start, 
		int end, 
		char* data, 
		int size) {
  
  logger->enter("xlink_load");

  bool result = true;

  if(driver->open()) {
    
    if(!driver->ping()) {
      logger->error("no response from C64");
      result = false;
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
  logger->leave();
  return result;
}

//------------------------------------------------------------------------------

bool xlink_save(unsigned char memory, 
		unsigned char bank, 
		int start, 
		int end, 
		char* data, 
		int size) {

  if(driver->open()) {

    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_SAVE, memory, bank, 
          lo(start), hi(start), lo(end), hi(end)}, 7);

    driver->input();
    driver->strobe();

    driver->receive(data, size);

    driver->close();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_peek(unsigned char memory, 
		unsigned char bank, 
		int address, 
		unsigned char* value) {
  
  if(driver->open()) {
  
    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_PEEK, memory, bank, lo(address), hi(address)}, 5);
    
    driver->input();
    driver->strobe();

    driver->receive((char *)value, 1);

    driver->close();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_poke(unsigned char memory, 
		unsigned char bank, 
		int address, 
		unsigned char value) {

  if(driver->open()) {
  
    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }
    
    driver->output();
    driver->send((char []) {XLINK_COMMAND_POKE, memory, bank, 
          lo(address), hi(address), value}, 6);    

    driver->close();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_jump(unsigned char memory, 
		unsigned char bank, 
		int address) {

    // jump address is send MSB first (big-endian)    

   if(driver->open()) {
  
    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_JUMP, memory, bank, 
          hi(address), lo(address)}, 5);    

    driver->close();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_run(void) {

   if(driver->open()) {
  
    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }

    driver->output();
    driver->send((char []) {XLINK_COMMAND_RUN}, 1);

    driver->close();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_extend(int address) {

  if(driver->open()) {
  
    if(!driver->ping()) {
      logger->error("no response from C64");
      driver->close();
      return false;
    }
  
    // send the address-1 high byte first, so the server can 
    // just push it on the stack and rts
    
    address--;

    driver->output();
    driver->send((char []) {XLINK_COMMAND_EXTEND, hi(address), lo(address)}, 3);

    driver->close();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

bool xlink_drive_status(char* status) {

  logger->enter("xlink_drive_status");

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
  logger->leave();

  return result;
}

//------------------------------------------------------------------------------

bool xlink_dos(char* cmd) {

  logger->enter("xlink_dos");

  bool result = false;

  Extension *lib = EXTENSION_LIB;
  Extension *dos_command = EXTENSION_DOS_COMMAND;

  char *command = (char *) calloc(strlen(cmd)+1, sizeof(char));
  
  for(int i=0; i<strlen(cmd); i++) {
    command[i] = toupper(cmd[i]);
  }      

  logger->debug("loading & initialising extensions");

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
  logger->leave();

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
  return result;
}

//------------------------------------------------------------------------------

bool xlink_stream_open() {

  Extension *stream = EXTENSION_STREAM; 
  bool result = extension_load(stream) && extension_init(stream) && driver->open();
  
  extension_free(stream);
  return result;  
}

//------------------------------------------------------------------------------

bool xlink_stream_poke(int address, unsigned char value) {
  
  driver->output();
  driver->send((char []) {XLINK_COMMAND_STREAM_POKE, lo(address), hi(address), value}, 4);
  driver->input(); 
  return true;
}

//------------------------------------------------------------------------------

bool xlink_stream_peek(int address, unsigned char* value) {

  driver->output();
  driver->send((char []) {XLINK_COMMAND_STREAM_PEEK, lo(address), hi(address)}, 3);
    
  driver->input();
  driver->strobe();

  driver->receive((char *)value, 1);
  driver->wait(0);

  return true;
}

//------------------------------------------------------------------------------

bool xlink_stream_close(void) {

  driver->output();  
  driver->send((char []) {XLINK_COMMAND_STREAM_CLOSE}, 1);
  
  driver->input();
  driver->close();
  return true;
}

//------------------------------------------------------------------------------


