#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if linux
  #include <sys/sysmacros.h>
#endif

#include "target.h"
#include "error.h"
#include "util.h"
#include "driver.h"
#include "parport.h"
#include "usb.h"
#include "shm.h"
#include "serial.h"

extern Driver* driver;

//------------------------------------------------------------------------------

bool _driver_setup_and_open(void) {

  bool result = false;
  bool autodetect = false;

#if linux
  char default_usb_device[] = "/dev/xlink";
  char default_parport_device[] = "/dev/parport0";

#elif mac
  char default_usb_device[] = "usb";

#elif windows
  char default_usb_device[] = "usb";
  char default_parport_device[] = "0x378";
#endif

  if (getenv("XLINK_DEVICE") != NULL) {
    result = driver_setup(getenv("XLINK_DEVICE"));    
  }
  else {
    autodetect = true;

    if(!(result = driver_setup(default_usb_device))) {

#if linux || windows      
      result = driver_setup(default_parport_device);

      if(result) {
        logger->info("using default parallel port device %s",
                     default_parport_device);
      }
#endif
      
    }
    else {
      logger->info("using default usb device \"%s\"",
                   default_usb_device);
    }
  }
  
  if(result) {
    result = driver->open();
  }
#if linux || windows
  else if(autodetect) {
    SET_ERROR(XLINK_ERROR_DEVICE, 
	      "failed to initialize \"%s\" or \"%s\" (autodetect)",
	      default_usb_device, default_parport_device);    
  }
#endif
  
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool driver_setup(char* path) {

  bool result = false;
  int type;

  driver->path = (char *) realloc(driver->path, strlen(path)+1);
  strcpy(driver->path, path);
  
  if(!device_identify(path, &type)) {
    goto done;
  }

  if(!device_is_supported(path, type)) {
    goto done;
  }

  if(device_is_parport(type)) {

    logger->debug("trying to use parallel port device \"%s\"...", driver->path);
  
    driver->_open    = &driver_parport_open;
    driver->_close   = &driver_parport_close;    
    driver->_strobe  = &driver_parport_strobe;    
    driver->_wait    = &driver_parport_wait;    
    driver->_read    = &driver_parport_read;    
    driver->_write   = &driver_parport_write;    
    driver->_send    = &driver_parport_send;    
    driver->_receive = &driver_parport_receive;    
    driver->_input   = &driver_parport_input;    
    driver->_output  = &driver_parport_output;    
    driver->_ping    = &driver_parport_ping;    
    driver->_reset   = &driver_parport_reset;    
    driver->_boot    = &driver_parport_boot;    
    driver->_free    = &driver_parport_free;
    
    result = driver->ready();
    
    if(result) {
      logger->debug("using parallel port device \"%s\"", driver->path);
    }
    else {
      SET_ERROR(XLINK_ERROR_DEVICE, "failed to initialize parallel port device \"%s\"", driver->path);
    }	
    
  } else if(device_is_usb(type)) {
    
    logger->debug("trying to use usb device \"%s\"...",  driver->path);

    driver->_open    = &driver_usb_open;
    driver->_close   = &driver_usb_close;    
    driver->_strobe  = &driver_usb_strobe;    
    driver->_wait    = &driver_usb_wait;
    driver->_read    = &driver_usb_read;    
    driver->_write   = &driver_usb_write;    
    driver->_send    = &driver_usb_send;    
    driver->_receive = &driver_usb_receive;    
    driver->_input   = &driver_usb_input;    
    driver->_output  = &driver_usb_output;    
    driver->_ping    = &driver_usb_ping;    
    driver->_reset   = &driver_usb_reset;    
    driver->_boot    = &driver_usb_boot;    
    driver->_free    = &driver_usb_free;
    
    result = driver->ready();
    
    if(result) {
      logger->debug("using usb device \"%s\"", driver->path);
    }
    else {
      SET_ERROR(XLINK_ERROR_DEVICE, "failed to initialize usb device \"%s\"", driver->path);
    }

  } else if(device_is_shm(type)) {
    
    logger->debug("trying to use shm device \"%s\"...",  driver->path);

    driver->_open    = &driver_shm_open;
    driver->_close   = &driver_shm_close;    
    driver->_strobe  = &driver_shm_strobe;    
    driver->_wait    = &driver_shm_wait;
    driver->_read    = &driver_shm_read;    
    driver->_write   = &driver_shm_write;    
    driver->_send    = &driver_shm_send;    
    driver->_receive = &driver_shm_receive;    
    driver->_input   = &driver_shm_input;    
    driver->_output  = &driver_shm_output;    
    driver->_ping    = &driver_shm_ping;    
    driver->_reset   = &driver_shm_reset;    
    driver->_boot    = &driver_shm_boot;    
    driver->_free    = &driver_shm_free;
    
    result = driver->ready();
    
    if(result) {
      logger->debug("using shm device \"%s\"", driver->path);
    }
    else {
      SET_ERROR(XLINK_ERROR_DEVICE, "failed to initialize shm device \"%s\"", driver->path);
    }

  } else if(device_is_serial(type)) {
    
    logger->debug("trying to use serial device \"%s\"...",  driver->path);

    driver->_open    = &driver_serial_open;
    driver->_close   = &driver_serial_close;    
    driver->_strobe  = &driver_serial_strobe;    
    driver->_wait    = &driver_serial_wait;
    driver->_read    = &driver_serial_read;    
    driver->_write   = &driver_serial_write;    
    driver->_send    = &driver_serial_send;    
    driver->_receive = &driver_serial_receive;    
    driver->_input   = &driver_serial_input;    
    driver->_output  = &driver_serial_output;    
    driver->_ping    = &driver_serial_ping;    
    driver->_reset   = &driver_serial_reset;    
    driver->_boot    = &driver_serial_boot;    
    driver->_free    = &driver_serial_free;
    
    result = driver->ready();
    
    if(result) {
      logger->debug("using serial device \"%s\"", driver->path);
    }
    else {
      SET_ERROR(XLINK_ERROR_DEVICE, "failed to initialize serial device \"%s\"", driver->path);
    }
  }  
  
 done:
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool device_identify(char* path, int* type) {

  if(strncmp(path, "shm", 3) == 0) {
    (*type) = XLINK_DRIVER_DEVICE_SHM;
    return true;
  }

#if linux
  struct stat device;

  if(stat(path, &device) == -1) {

    SET_ERROR(XLINK_ERROR_DEVICE, "%s: couldn't stat: %s", path, strerror(errno));
    return false;
  }
  
  if(!S_ISCHR(device.st_mode)) {

    SET_ERROR(XLINK_ERROR_DEVICE, "%s: not a character device", path);
    return false;
  }

  (*type) = major(device.st_rdev);

  CLEAR_ERROR;
  return true;

#elif mac
  (*type) = XLINK_DRIVER_DEVICE_USB;

  if(strncmp(path, "usb", 3) == 0) {
    return true;
  }
  
  struct stat device;

  if(stat(path, &device) == -1) {

    SET_ERROR(XLINK_ERROR_DEVICE, "%s: couldn't stat: %s", path, strerror(errno));
    return false;
  }

  if(!S_ISCHR(device.st_mode)) {

    SET_ERROR(XLINK_ERROR_DEVICE, "%s: not a character device", path);
    return false;
  }

  (*type) = major(device.st_rdev);

  CLEAR_ERROR;
  return true;

#elif windows 

  (*type) = XLINK_DRIVER_DEVICE_USB;
  
  if(strncmp(path, "COM", 3) == 0) {
    (*type) = XLINK_DRIVER_DEVICE_SERIAL;
    return true;
  }

  errno = 0;
  if ((strtol(path, NULL, 0) > 0) && (errno == 0)) {
    (*type) = XLINK_DRIVER_DEVICE_PARPORT;
  }

  return true;
#endif
}

//------------------------------------------------------------------------------

bool device_is_supported(char *path, int type) {
#if linux

  if(!(device_is_parport(type) ||
       device_is_usb(type) ||       
       device_is_shm(type) ||
       device_is_serial(type))) {

    SET_ERROR(XLINK_ERROR_DEVICE, 
              "%s: unsupported device major number: %d", path, type);
    return false;
  }

#endif
  return true;
}

//------------------------------------------------------------------------------

bool device_is_parport(int type) {
  return type == XLINK_DRIVER_DEVICE_PARPORT;
}

//------------------------------------------------------------------------------

bool device_is_usb(int type) {
  return type == XLINK_DRIVER_DEVICE_USB;
}

//------------------------------------------------------------------------------

bool device_is_shm(int type) {
  return type == XLINK_DRIVER_DEVICE_SHM;
}

//------------------------------------------------------------------------------

bool device_is_serial(int type) {
  return type == XLINK_DRIVER_DEVICE_SERIAL;
}

//------------------------------------------------------------------------------

bool _driver_ready() {
  bool result = false;
  
  if((result = driver->open())) {
    driver->close();
  }
  
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool _driver_open()                                 { return driver->_open(); }
void _driver_close()                                { driver->input(); driver->_close(); }
void _driver_strobe()                               { driver->_strobe(); }
unsigned char _driver_read(void)                    { return driver->_read(); }
void _driver_write(unsigned char value)             { driver->_write(value); }
bool _driver_send(unsigned char* data, int size)    { return driver->_send(data, size); }
bool _driver_receive(unsigned char* data, int size) { return driver->_receive(data, size); }
bool _driver_wait(int timeout)                      { return driver->_wait(timeout); }
void _driver_input()                                { driver->_input(); }
void _driver_output()                               { driver->_output(); }
bool _driver_ping()                                 { return driver->_ping(); }
void _driver_boot()                                 { driver->_boot(); }
void _driver_reset()                                { driver->_reset(); }

//------------------------------------------------------------------------------

void _driver_free() {

  if(driver->_free != NULL) {
    driver->_free();
  }
  free(driver->path);
  free(driver);
  driver = NULL;
}

//------------------------------------------------------------------------------
