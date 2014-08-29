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

Driver* driver = NULL;

Driver *driver_setup(char* path) {

  if(!(device_is_parport(path) || device_is_usb(path))) {
    logger->error("%s: neither parallel port nor usb device", path);

    if(driver != NULL) {
      driver->free();
    }
  }

  if (driver == NULL) {

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
 }

  driver->path = (char*) realloc((void*) driver->path, (strlen(path)+1) * sizeof(char));
  strcpy(driver->path, path);

  if(device_is_parport(driver->path)) {

      logger->trace("using parallel port driver");
  
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
  
  } else if(device_is_usb(driver->path)) {

      logger->trace("using usb driver");

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
  }    

  return driver;
}

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
  return major(device.st_rdev) == 189;

#elif windows
  return !device_is_parport(path);
#endif
}

bool _driver_ready() {
  bool result = false;

  if(driver->_open == NULL) {
    return result;
  }

  if((result = driver->open())) {
    driver->close();
  }
  return result;
}

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

void _driver_free() {
  driver->_free();
  free(driver->path);
  free(driver);
  driver = NULL;
}
