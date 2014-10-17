#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "target.h"
#include "error.h"
#include "util.h"
#include "driver.h"
#include "parport.h"
#include "usb.h"

extern Driver* driver;

//------------------------------------------------------------------------------

bool _driver_setup_and_open(void) {

  bool result = false;
  
#if linux
  char default_usb_device[] = "/dev/xlink";
  char default_parport_device[] = "/dev/parport0";

#elif windows
  char default_usb_device[] = "usb";
  char default_parport_device[] = "0x378";
#endif

  if (getenv("XLINK_DEVICE") != NULL) {
    result = driver_setup(getenv("XLINK_DEVICE"), false);
  }
  else {

    if(!(result = driver_setup(default_usb_device, true))) {

      result = driver_setup(default_parport_device, true);

      if(result) {
        logger->info("using default parallel port device %s",
                     default_parport_device, true);
      }
    }
    else {
      logger->info("using default usb device \"%s\"",
                   default_usb_device);
    }
  }
  
  if(result) {
    result = driver->open();
  }

  return result;
}

//------------------------------------------------------------------------------

bool driver_setup(char* path, bool quiet) {

  bool result = false;
  int type;

  if(quiet) {
    logger->suspend();
  }

  if(!device_identify(path, &type)) {
    goto done;
  }

  if(!device_is_supported(path, type)) {
    goto done;
  }

  driver->path = (char *) realloc(driver->path, strlen(path)+1);
  strcpy(driver->path, path);

  if(device_is_parport(type)) {

    logger->debug("trying to use parallel port device %s...", driver->path);
  
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
      logger->debug("using parallel port device %s", driver->path);
    }
    
  } else if(device_is_usb(type)) {
    
    logger->debug("trying to use usb device \"%s\"...", driver->path);

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
  }

 done:

  if(quiet) {
    logger->resume();
  }

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool device_identify(char* path, int* type) {
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

  CLEAR_ERROR_IF(true);
  return true;

#elif windows

  (*type) = XLINK_DRIVER_DEVICE_USB;

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

  if(!(device_is_parport(type) || device_is_usb(type))) {

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

bool _driver_ready() {
  bool result = false;

  logger->suspend();

  if((result = driver->open())) {
    driver->close();
  }
  
  logger->resume();

  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool _driver_open()                        { return driver->_open(); }
void _driver_close()                       { driver->_close(); }
void _driver_strobe()                      { driver->_strobe(); }
char _driver_read(void)                    { return driver->_read(); }
void _driver_write(char value)             { driver->_write(value); }
void _driver_send(char* data, int size)    { driver->_send(data, size); }
void _driver_receive(char* data, int size) { driver->_receive(data, size); }
bool _driver_wait(int timeout)             { return driver->_wait(timeout); }
void _driver_input()                       { driver->_input(); }
void _driver_output()                      { driver->_output(); }
bool _driver_ping(void)                    { return driver->_ping(); }
void _driver_boot()                        { driver->_boot(); }
void _driver_reset()                       { driver->_reset(); }

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
