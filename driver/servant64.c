#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "target.h"
#include "error.h"
#include "xlink.h"
#include "driver.h"
#include "servant64.h"
#include "util.h"

extern Driver* driver;

static int direction = XLINK_DRIVER_STATE_INPUT;
static bool initialized = false;

bool driver_servant64_open(void) {
  bool result = false;

  if(!initialized) {    
    initialized = true;
  }

  driver->input();
  result = true;
  
  CLEAR_ERROR_IF(result);
  return result;  
}

//------------------------------------------------------------------------------

void driver_servant64_input(void) {
  direction = XLINK_DRIVER_STATE_INPUT;
}

//------------------------------------------------------------------------------

void driver_servant64_output(void) {
  direction = XLINK_DRIVER_STATE_OUTPUT;
}

//------------------------------------------------------------------------------

void driver_servant64_strobe(void) {
  
}

//------------------------------------------------------------------------------

bool driver_servant64_wait(int timeout) {

  bool acked() {
    return false;
  }

  bool result = false;

  if(timeout <= 0) {
    while(!acked());
    result = true;
  }
  else {
    while(timeout && !(result = acked())) {      
      usleep(10*1000);     
      timeout-=10;
    }
  }
  return result;
}

//------------------------------------------------------------------------------

unsigned char driver_servant64_read(void) {
  return 0xff;
}

//------------------------------------------------------------------------------

void driver_servant64_write(unsigned char value) {

}

//------------------------------------------------------------------------------

bool driver_servant64_send(unsigned char* data, int size) {

  bool result = false;
  
  for(int i=0; i<size; i++) {
    driver->write(data[i]);
    driver->strobe();    
    result = driver->wait(driver->timeout*1083);

    if(!result) {
      SET_ERROR(XLINK_ERROR_FILE,
                "transfer timeout (%d of %d bytes sent)", i, size);
      break;
    }
  }
  return result;
}

//------------------------------------------------------------------------------

bool driver_servant64_receive(unsigned char* data, int size) { 

  bool result = false;
  
  for(int i=0; i<size; i++) {
    result = driver->wait(driver->timeout*1083);

    if(!result) {
      SET_ERROR(XLINK_ERROR_FILE,
                "transfer timeout (%d of %d bytes received)", i, size);
      break;
    }

    data[i] = driver->read();
    driver->strobe();
  }
  return result;
}

//------------------------------------------------------------------------------

bool driver_servant64_ping() { 
  driver->output();
  driver->write(XLINK_COMMAND_PING);
  driver->strobe();
  return driver->wait(250);
}

//------------------------------------------------------------------------------

void driver_servant64_reset(void) {
}

//------------------------------------------------------------------------------

void driver_servant64_free(void) {
}

//------------------------------------------------------------------------------

void driver_servant64_close(void) {  }

//------------------------------------------------------------------------------

void driver_servant64_boot(void) {  }

//------------------------------------------------------------------------------
