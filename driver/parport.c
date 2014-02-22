#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "target.h"
#include "pp64.h"
#include "driver.h"
#include "parport.h"
#include "util.h"

#if linux
  #include <sys/io.h>
  #include <sys/ioctl.h>
  #include <linux/parport.h>
  #include <linux/ppdev.h>
#elif windows
  #include <windows.h>
#endif

#define HIGH 1
#define LOW 0

extern Driver* driver;
static unsigned char _driver_parport_last_status;

//------------------------------------------------------------------------------

static unsigned char _driver_parport_read_status() {
  unsigned char status = 0xff;

#if linux
  ioctl(driver->device, PPRSTATUS, &status);
#endif

  return status;
}

//------------------------------------------------------------------------------

static void _driver_parport_set_control(unsigned char control) {
#if linux
  ioctl(driver->device, PPWCONTROL, &control);
#endif
}

//------------------------------------------------------------------------------

static void _driver_parport_frob_control(unsigned char mask, unsigned char state) {
#if linux
  struct ppdev_frob_struct frob;
  frob.mask = mask;
  frob.val = state == HIGH ? mask : LOW;

  ioctl(driver->device, PPFCONTROL, &frob);
#endif
}

//------------------------------------------------------------------------------

static void _driver_parport_init() {

  _driver_parport_set_control(
                               DRIVER_PARPORT_CONTROL_SELECT | 
                               DRIVER_PARPORT_CONTROL_INIT |
                               DRIVER_PARPORT_CONTROL_INPUT |
                               DRIVER_PARPORT_CONTROL_IRQ);

  
  _driver_parport_last_status = _driver_parport_read_status();

#if linux
  ioctl(driver->device, PPSETFLAGS, PP_FASTWRITE | PP_FASTREAD);
#endif
}

//------------------------------------------------------------------------------

bool driver_parport_open() {
#if linux
  if((driver->device = open(driver->path, O_RDWR)) == -1) {

    logger->error("Couldn't open %s", driver->path);
    return false;
  }  
  ioctl(driver->device, PPCLAIM);
 
  _driver_parport_init();
  return true;
#endif  
  return false;
}

//------------------------------------------------------------------------------

void driver_parport_close() {
#if linux
  ioctl(driver->device, PPRELEASE);
  close(driver->device);
#endif
} 

//------------------------------------------------------------------------------

void driver_parport_strobe() {
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_STROBE, LOW);
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_STROBE, HIGH);
}

//------------------------------------------------------------------------------

bool driver_parport_wait(int timeout) {

  bool result = true;
  unsigned char current = _driver_parport_last_status;
  Watch* watch = watch_new();

  if (timeout <= 0) {
    while (current == _driver_parport_last_status) {
      current = _driver_parport_read_status();
    }
  }
  else {
    watch_start(watch); 
    
    while (current == _driver_parport_last_status) {
      current = _driver_parport_read_status();
      
      if (watch_elapsed(watch) >= timeout) {
        result = false;
        goto done;
      }
    }
  }
  _driver_parport_last_status = current;

 done:
  watch_free(watch);
  return result;
}

//------------------------------------------------------------------------------

char driver_parport_read(void) {
  
  char value = 0xff;

#if linux
  ioctl(driver->device, PPRDATA, &value);
#endif

  return value;
}

//------------------------------------------------------------------------------

void driver_parport_write(char value) { 
#if linux
  ioctl(driver->device, PPWDATA, &value);
#endif
}

//------------------------------------------------------------------------------

void driver_parport_send(char* data, int size) {

  for(int i=0; i<size; i++) {
    driver->write(data[i]);
    driver->strobe();
    driver->wait(0);
  }
}

//------------------------------------------------------------------------------

void driver_parport_receive(char* data, int size) { 

  for(int i=0; i<size; i++) {
    driver->wait(0);
    data[i] = driver->read();
    driver->strobe();
  }
}

//------------------------------------------------------------------------------

void driver_parport_input() { 
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_INPUT, HIGH);
}

//------------------------------------------------------------------------------

void driver_parport_output() {
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_INPUT, LOW);
}

//------------------------------------------------------------------------------

bool driver_parport_ping() { 
  driver->output();
  driver->write(0xff);
  driver->strobe();
  return driver->wait(PP64_PING_TIMEOUT);
}
 
//------------------------------------------------------------------------------

void driver_parport_reset() {
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_INIT, LOW);
  usleep(10*1000);
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_INIT, HIGH);
}

//------------------------------------------------------------------------------

 void driver_parport_flash() { /* nothing to flash */ }

//------------------------------------------------------------------------------

 void driver_parport_free() { /* nothing to free */ }

