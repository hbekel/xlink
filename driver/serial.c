#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "target.h"
#include "error.h"
#include "xlink.h"
#include "driver.h"
#include "serial.h"
#include "protocol.h"
#include "util.h"

#if posix

  #if mac
    #define B500000 500000
  #endif

  #include <termios.h>
  #define BAUD B500000

#elif windows
  #include <windows.h> 
  static HANDLE hSerial;
#endif

extern Driver* driver;
static bool initialized = false;

//------------------------------------------------------------------------------

static void serial_read(uchar* data, int size) {

  logger->debug("Serial read %d bytes", size);
  
#if posix
  int bytesRead = 0;
  while(size > bytesRead) {
    bytesRead += read(driver->device, data+bytesRead, size-bytesRead);
  }
  
#elif windows
  DWORD bytesReadTotal = 0;
  DWORD bytesRead = 0;

  while(size > bytesReadTotal) {
    ReadFile(hSerial, data+bytesReadTotal, size-bytesReadTotal, &bytesRead, NULL);
    bytesReadTotal += bytesRead;
  }  
#endif
}

//------------------------------------------------------------------------------

static bool write_chunk(ushort chunk, void *context) {

  Transfer *transfer = (Transfer*) context;

#if posix
  write(driver->device, transfer->data, chunk);
  tcdrain(driver->device);
  
#elif windows
  DWORD bytesWritten;
  WriteFile(hSerial, transfer->data, chunk, &bytesWritten, NULL);
  FlushFileBuffers(hSerial);
#endif
  
  transfer->data += chunk;
  return true;
}

static void serial_write(uchar* data, int size) {

  logger->debug("Serial write %d bytes", size);  

  Transfer transfer;
  transfer.data = data;
  transfer.completed = 0;
  chunked(&write_chunk, &transfer, 16, size);
}

//------------------------------------------------------------------------------

static bool cmd(uchar cmd, uint arg1, uint arg2) {
  uchar command[9] = { cmd,
		       lo(arg1), hi(arg1), hlo(arg1), hhi(arg1),
		       lo(arg2), hi(arg2), hlo(arg2), hhi(arg2) };
  serial_write(command, 9);  
  return true;
}

//------------------------------------------------------------------------------

bool driver_serial_open(void) {
  bool result = false;

  if(!initialized) {

#if posix      
    struct termios options;
    
    if((driver->device = open(driver->path, O_RDWR, O_NOCTTY)) < 0) goto error;

    options.c_cflag |= (BAUD | CS8 | CLOCAL | CREAD);
    options.c_cflag &= ~(HUPCL | CRTSCTS);
    options.c_iflag = IGNPAR;
    options.c_oflag &= ~(OPOST);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_cc[VTIME] = 10;
    options.c_cc[VMIN] = 0;
    
    tcflush(driver->device, TCIFLUSH);
    tcsetattr(driver->device, TCSANOW, &options);   
    
#elif windows
    hSerial = CreateFile(driver->path,
			 GENERIC_READ | GENERIC_WRITE,
			 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if(hSerial == INVALID_HANDLE_VALUE) goto error;

    DCB options = {0};
    options.DCBlength=sizeof(options);

    if(!GetCommState(hSerial, &options)) goto error;

    options.BaudRate = 500000;
    options.ByteSize = 8;
    options.StopBits = ONESTOPBIT;
    options.Parity   = NOPARITY;

    if(!SetCommState(hSerial, &options)) goto error;

    COMMTIMEOUTS timeouts = {0};

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if(!SetCommTimeouts(hSerial, &timeouts)) goto error;
       
#endif
    initialized = true;
  }
  
  driver->input();
  result = true;

 done:  
  CLEAR_ERROR_IF(result);
  return result;

 error:  
#if posix
  SET_ERROR(XLINK_ERROR_FILE, strerror(errno));
#elif windows
  SET_ERROR(XLINK_ERROR_FILE, strerror(GetLastError()));
#endif
  goto done;
}

//------------------------------------------------------------------------------

void driver_serial_input(void) {
  cmd(CMD_INPUT, 0, 0);
}

//------------------------------------------------------------------------------

void driver_serial_output(void) {
  cmd(CMD_OUTPUT, 0, 0);
}

//------------------------------------------------------------------------------

void driver_serial_strobe(void) {
  cmd(CMD_STROBE, 0, 0);
}

//------------------------------------------------------------------------------

static bool acked(void) {
  uchar response[1] = { 0 };
  cmd(CMD_ACKED, 0, 0);
  serial_read(response, 1);
  return response[0] == 0x55;
}

bool driver_serial_wait(int timeout) {
  
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

unsigned char driver_serial_read(void) {
  uchar response[1] = { 0xff };
  cmd(CMD_READ, 0, 0);
  serial_read(response, 1);
  return response[0];
}

//------------------------------------------------------------------------------

void driver_serial_write(unsigned char value) {
  cmd(CMD_WRITE, value, 0);
}

//------------------------------------------------------------------------------

bool driver_serial_send(unsigned char* data, int size) {
  unsigned int bytesSent;
  
  cmd(CMD_SEND, size, 2);
  serial_write(data, size);
  serial_read((uchar*) &bytesSent, 4);

  bool result = bytesSent == size;
  
  if(!result) {
    SET_ERROR(XLINK_ERROR_SERIAL,
              "transfer timeout (%d of %d bytes sent)", bytesSent, size);
  }
  
  CLEAR_ERROR_IF(result);
  return result;
}
//------------------------------------------------------------------------------

bool driver_serial_receive(unsigned char* data, int size) { 
  unsigned int bytesReceived;

  cmd(CMD_RECEIVE, size, 2);
  serial_read(data, size);
  serial_read((uchar*) &bytesReceived, 4);

  bool result = bytesReceived == size;
  
  if(!result) {
    SET_ERROR(XLINK_ERROR_SERIAL,
              "transfer timeout (%d of %d bytes received)", bytesReceived, size);
  }
  
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool driver_serial_ping() { 
  driver->output();
  driver->write(XLINK_COMMAND_PING);
  driver->strobe();
  return driver->wait(250);
}

//------------------------------------------------------------------------------

void driver_serial_reset(void) {
  cmd(CMD_RESET, 0, 0);
  usleep(250*1000);
}

//------------------------------------------------------------------------------

void driver_serial_free(void) {
#if posix
  close(driver->device);
#elif windows
  CloseHandle(hSerial);
#endif
}

//------------------------------------------------------------------------------

void driver_serial_close(void) {
}

//------------------------------------------------------------------------------

void driver_serial_boot(void) { }

//------------------------------------------------------------------------------
