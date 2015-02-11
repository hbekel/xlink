#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "target.h"
#include "error.h"
#include "xlink.h"
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
  #include "../inpout32.h"
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
#elif windows
  status = Inp32(driver->device+1);
#endif

  return status;
}

//------------------------------------------------------------------------------

static void _driver_parport_set_control(unsigned char control) {
#if linux
  ioctl(driver->device, PPWCONTROL, &control);
#elif windows
  Out32(driver->device+2, control);
#endif
}

//------------------------------------------------------------------------------

static void _driver_parport_frob_control(unsigned char mask, unsigned char state) {
#if linux
  struct ppdev_frob_struct frob;
  frob.mask = mask;
  frob.val = state == HIGH ? mask : LOW;

  ioctl(driver->device, PPFCONTROL, &frob);
#elif windows
  unsigned char previous = Inp32(driver->device+2);
  unsigned char frobbed = (previous & ~mask) | (state == HIGH ? mask : 0);
  Out32(driver->device+2, frobbed);
#endif
}

//------------------------------------------------------------------------------

static void _driver_parport_init() {

#if linux
  ioctl(driver->device, PPSETFLAGS, PP_FASTWRITE | PP_FASTREAD);

#elif windows
  driver->device = strtol(driver->path, NULL, 0);
#endif

  _driver_parport_set_control(
                               DRIVER_PARPORT_CONTROL_SELECT | 
                               DRIVER_PARPORT_CONTROL_INIT |
                               DRIVER_PARPORT_CONTROL_INPUT |
                               DRIVER_PARPORT_CONTROL_IRQ);

  
  _driver_parport_last_status = _driver_parport_read_status();
}

//------------------------------------------------------------------------------

bool driver_parport_open() {

  bool result = false;
  
#if linux
  if((driver->device = open(driver->path, O_RDWR)) == -1) {
    SET_ERROR(XLINK_ERROR_PARPORT, "Couldn't open %s", driver->path);
    goto done;
  }  
  
  if(!ioctl(driver->device, PPCLAIM) == 0) {
    SET_ERROR(XLINK_ERROR_PARPORT, "Couldn't claim %s", driver->path);
    goto done;
  }

#elif windows
  if(!IsInpOutDriverOpen()) {

    logger->resume();
    
    SET_ERROR(XLINK_ERROR_PARPORT, "InpOut32 driver is required for parallel port access");
    
    printf("\nTo enable parallel port access on windows xp and later\n");
    printf("xlink.exe needs to be run as Administrator once. This will\n");
    printf("automatically install the necessary drivers. For details see\n\n");
    printf("    http://www.highrez.co.uk/Downloads/InpOut32/\n\n");
    goto done;
  }
#endif
  
 result = true;
  
 done:
  if(result) {
    _driver_parport_init();
  }

  CLEAR_ERROR_IF(result);
  return result;
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

unsigned char driver_parport_read(void) {
  
  unsigned char value = 0xff;

#if linux
  ioctl(driver->device, PPRDATA, &value);
#elif windows
  value = Inp32(driver->device);
#endif

  return value;
}

//------------------------------------------------------------------------------

void driver_parport_write(unsigned char value) { 
#if linux
  ioctl(driver->device, PPWDATA, &value);
#elif windows
  Out32(driver->device, value);
#endif
}

//------------------------------------------------------------------------------

void driver_parport_send(unsigned char* data, int size) {

  for(int i=0; i<size; i++) {
    driver->write(data[i]);
    driver->strobe();
    driver->wait(0);
  }
}

//------------------------------------------------------------------------------

void driver_parport_receive(unsigned char* data, int size) { 

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
  return driver->wait(XLINK_PING_TIMEOUT);
}
 
//------------------------------------------------------------------------------

void driver_parport_reset() {
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_INIT, LOW);
  usleep(10*1000);
  _driver_parport_frob_control(DRIVER_PARPORT_CONTROL_INIT, HIGH);
}

//------------------------------------------------------------------------------

 void driver_parport_boot() { /* nothing to boot */ }

//------------------------------------------------------------------------------

 void driver_parport_free() { /* nothing to free */ }

