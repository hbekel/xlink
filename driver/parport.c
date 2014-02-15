#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "target.h"
#include "driver.h"
#include "parport.h"

#if linux
  #include <sys/io.h>
  #include <sys/ioctl.h>
  #include <linux/parport.h>
  #include <linux/ppdev.h>
#elif windows
  #include <windows.h>
#endif

extern Driver* driver;

int _driver_parport_port;

bool driver_parport_open() { return false; }
void driver_parport_close() { } 
void driver_parport_strobe() { }
bool driver_parport_wait(int timeout) { return false; }
char driver_parport_read(void) { return 0xff; }
void driver_parport_write(char value) { }
void driver_parport_send(char* data, int size) { }
void driver_parport_receive(char* data, int size) { }
void driver_parport_input() { }
void driver_parport_output() { }
bool driver_parport_ping() { return false; }
void driver_parport_reset() { }
void driver_parport_flash() { }
void driver_parport_free() { }

void _driver_parport_frob_control(unsigned char mask, unsigned char val) {

#if linux
  struct ppdev_frob_struct frob;
  frob.mask = mask;
  frob.val = val; 

  ioctl(_driver_parport_port, PPFCONTROL, &frob);
#endif
}

