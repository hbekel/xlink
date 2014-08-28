#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "target.h"
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
#endif

#define HIGH 1
#define LOW 0

extern Driver* driver;
static unsigned char _driver_parport_last_status;

#if windows
  typedef BOOL (__stdcall *lpDriverOpened)(void);
  typedef void (__stdcall *lpOutb)(short, short);
  typedef short (__stdcall *lpInb)(short);
 
  static HINSTANCE inpout32 = NULL;
  static lpOutb outb;
  static lpInb inb;
  static lpDriverOpened driverOpened;
#endif

//------------------------------------------------------------------------------

static unsigned char _driver_parport_read_status() {
  unsigned char status = 0xff;

#if linux
  ioctl(driver->device, PPRSTATUS, &status);
#elif windows
  status = inb(driver->device+1);
#endif

  return status;
}

//------------------------------------------------------------------------------

static void _driver_parport_set_control(unsigned char control) {
#if linux
  ioctl(driver->device, PPWCONTROL, &control);
#elif windows
  outb(driver->device+2, control);
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
  unsigned char previous = inb(driver->device+2);
  unsigned char frobbed = (previous & ~mask) | (state == HIGH ? mask : 0);
  outb(driver->device+2, frobbed);
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

#if linux
  if((driver->device = open(driver->path, O_RDWR)) == -1) {

    logger->error("Couldn't open %s", driver->path);
    return false;
  }  
  
  if(ioctl(driver->device, PPCLAIM) == 0) {
    _driver_parport_init();
    return true;
  }
  logger->error("Couldn't claim %s", driver->path);
  return false;

#elif windows
  if(inpout32 == NULL) {
    
    inpout32 = LoadLibrary( "inpout32.dll" ) ;	
    
    if (inpout32 != NULL) {
      
      driverOpened = (lpDriverOpened) GetProcAddress(inpout32, "IsInpOutDriverOpen");
      outb = (lpOutb) GetProcAddress(inpout32, "Out32");
      inb = (lpInb) GetProcAddress(inpout32, "Inp32");		
      
      if (driverOpened()) {
        _driver_parport_init();
        return true;
      }
      else {
        logger->error("failed to open inpout32 parallel port driver\n");
      }		
    }
    else {
      logger->error("xlink: error: failed to load inpout32.dll\n\n"
                    "Inpout32 is required for parallel port access:\n\n"    
                    "    http://www.highrez.co.uk/Downloads/InpOut32/\n\n");	
    }
    return false;
  }  
  _driver_parport_init();
  return true;
#endif
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
#elif windows
  value = inb(driver->device);
#endif

  return value;
}

//------------------------------------------------------------------------------

void driver_parport_write(char value) { 
#if linux
  ioctl(driver->device, PPWDATA, &value);
#elif windows
  outb(driver->device, value);
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

