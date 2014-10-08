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
    result = driver_setup(getenv("XLINK_DEVICE"));
  }
  else {

    if(!(result = driver_setup(default_usb_device))) {

      result = driver_setup(default_parport_device);

      if(result) {
	logger->warn("using default parallel port device %s instead",
		     default_parport_device);
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

bool driver_setup(char* path) {

  bool result = false;
  
  if(!(device_is_parport(path) || device_is_usb(path))) {
    logger->error("%s: neither parallel port nor usb device", path);
    return result;
  }

  driver->path = (char *) realloc(driver->path, strlen(path)+1);
  strcpy(driver->path, path);

  if(device_is_parport(driver->path)) {

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
    
  } else if(device_is_usb(driver->path)) {
    
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
  return result;
}

//------------------------------------------------------------------------------

bool device_is_parport(char* path) {

#if linux
  struct stat device;
  
  if(stat(path, &device) == -1) {
    logger->error("couldn't stat %s: %s", path, strerror(errno));
    return false;
  }
  
  if(!S_ISCHR(device.st_mode)) {

    logger->error("%s: not a character device", path);
    return false;
  }
  
  return major(device.st_rdev) == 99;

#elif windows
  errno = 0;
  return (strtol(path, NULL, 0) > 0) && (errno == 0);   
#endif
}

//------------------------------------------------------------------------------

bool device_is_usb(char* path) {

#if linux
  struct stat device;
  
  if(stat(path, &device) == -1) {
    logger->error("couldn't stat %s: %s", path, strerror(errno));
    return false;
  }
  
  if(!S_ISCHR(device.st_mode)) {

    logger->error("%s: not a character device", path);
    return false;
  }
  if(major(device.st_rdev) != 189) {
    logger->error("%s: not a USB device", path);
    return false;
  }
  return true;

#elif windows
  return !device_is_parport(path);
#endif
}

//------------------------------------------------------------------------------

bool _driver_ready() {
  bool result = false;

  if((result = driver->open())) {
    driver->close();
  }
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
