#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if linux
  #include <termios.h>
#endif

#include "target.h"
#include "error.h"
#include "xlink.h"
#include "driver.h"
#include "servant64.h"
#include "protocol.h"
#include "util.h"

#define BAUD B115200

extern Driver* driver;
static bool initialized = false;

static void serial_read(uchar* data, int size) {
  int bytesRead = 0;

  while(size > bytesRead) {
    bytesRead += read(driver->device, data+bytesRead, size-bytesRead);    
  }
}


static void serial_write(uchar* data, int size) {
#if linux

  bool write_chunk(ushort chunk) {
    write(driver->device, data, chunk);
    tcdrain(driver->device);
    data+=chunk;
  }

  chunked(&write_chunk, 64, size);
#endif
}

static bool send(uchar cmd, uint arg1, uint arg2) {
  uchar command[9] = { cmd, lo(arg1), hi(arg1), hlo(arg1), hhi(arg1), lo(arg2), hi(arg2), hlo(arg2), hhi(arg2) };
  serial_write(command, 9);  
  return true;
}

bool driver_servant64_open(void) {
  bool result = false;

  if(!initialized) {

#if linux      
    struct termios options;
    
    if((driver->device = open(driver->path, O_RDWR, O_NOCTTY)) < 0) goto error;

    options.c_cflag |= (BAUD | CS8 | CLOCAL | CREAD);
    options.c_cflag &= ~(HUPCL);
    options.c_iflag = IGNPAR;
    options.c_oflag &= ~(OPOST);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_cc[VTIME] = 10;
    options.c_cc[VMIN] = 0;
    
    tcflush(driver->device, TCIFLUSH);
    tcsetattr(driver->device, TCSANOW, &options);   
    
    initialized = true;
#endif    
  }
  
  driver->input();
  result = true;

 done:  
  CLEAR_ERROR_IF(result);
  return result;

#if linux
 error:
  SET_ERROR(XLINK_ERROR_FILE, strerror(errno));
  goto done;
#endif
}

//------------------------------------------------------------------------------

void driver_servant64_input(void) {
  send(CMD_INPUT, 0, 0);
}

//------------------------------------------------------------------------------

void driver_servant64_output(void) {
  send(CMD_OUTPUT, 0, 0);
}

//------------------------------------------------------------------------------

void driver_servant64_strobe(void) {
  send(CMD_STROBE, 0, 0);
}

//------------------------------------------------------------------------------

bool driver_servant64_wait(int timeout) {

  uchar response[1] = { 0 };
  
  bool acked() {
    send(CMD_ACKED, 0, 0);
    serial_read(response, 1);
    return response[0] == 0x55;
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
  uchar response[1] = { 0xff };
  send(CMD_READ, 0, 0);
  serial_read(response, 1);
  return response[0];
}

//------------------------------------------------------------------------------

void driver_servant64_write(unsigned char value) {
  send(CMD_WRITE, 0, 0);
}

//------------------------------------------------------------------------------

bool driver_servant64_send(unsigned char* data, int size) {
  unsigned int bytesSent;
  
  send(CMD_SEND, size, 2);
  serial_write(data, size);
  serial_read((uchar*) &bytesSent, 4);

  bool result = bytesSent == size;
  
  if(!result) {
    SET_ERROR(XLINK_ERROR_SERVANT64,
              "transfer timeout (%d of %d bytes sent)", bytesSent, size);
  }
  
  CLEAR_ERROR_IF(result);
  return result;
}
//------------------------------------------------------------------------------

bool driver_servant64_receive(unsigned char* data, int size) { 
  unsigned int bytesReceived;

  send(CMD_RECEIVE, size, 2);
  serial_read(data, size);
  serial_read((uchar*) &bytesReceived, 4);

  bool result = bytesReceived == size;
  
  if(!result) {
    SET_ERROR(XLINK_ERROR_SERVANT64,
              "transfer timeout (%d of %d bytes received)", bytesReceived, size);
  }
  
  CLEAR_ERROR_IF(result);
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
  send(CMD_RESET, 0, 0);
  usleep(250*1000);
}

//------------------------------------------------------------------------------

void driver_servant64_free(void) {
  close(driver->device);
}

//------------------------------------------------------------------------------

void driver_servant64_close(void) {
}

//------------------------------------------------------------------------------

void driver_servant64_boot(void) { }

//------------------------------------------------------------------------------
