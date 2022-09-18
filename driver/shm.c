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
#include "shm.h"
#include "util.h"

#if posix
  #include <sys/shm.h>
  #include <signal.h>
#elif windows
  #include <windows.h>
#endif

extern Driver* driver;
static xlink_port_t *port;
static char* shmname = "/tmp/xlink";

#if posix
  static int shmid;
#elif windows
  static HANDLE hMapFile;
#endif

static int direction = XLINK_DRIVER_STATE_INPUT;
static bool initialized = false;
static uchar last;

bool driver_shm_open(void) {
  bool result = false;
  if(!initialized) {
    
#if posix
    int fd = open(shmname, O_CREAT | O_RDWR, S_IRWXU);
    close(fd);

    key_t key = ftok(shmname, 1);

    shmid = shmget(key, sizeof(xlink_port_t),
		   IPC_CREAT | S_IRUSR | S_IWUSR);

    if(shmid == -1) goto error;
    
    port = (xlink_port_t*) shmat(shmid, NULL, 0);
    
    if((long)port == -1) goto error;

#elif windows
    
    hMapFile =
      OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shmname);

    if(hMapFile == NULL) goto error;

    port = (xlink_port_t*) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS,
					 0, 0, sizeof(xlink_port_t));

    if(port == NULL) goto error;    

#endif
        
    port->flag = 0;    
    initialized = true;
  }

  driver->input();
  last = port->pa2;
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

void driver_shm_input(void) {
  direction = XLINK_DRIVER_STATE_INPUT;
}

//------------------------------------------------------------------------------

void driver_shm_output(void) {
  direction = XLINK_DRIVER_STATE_OUTPUT;
}

//------------------------------------------------------------------------------

void driver_shm_strobe(void) {
  port->flag++;
}

//------------------------------------------------------------------------------

bool driver_shm_wait(int timeout) {

  bool result = true;
  unsigned char current = last;
  Watch* watch = watch_new();

  if (timeout <= 0) {
    while (current == last) {
      if((current = port->pa2) != last) {
	last = current;
	break;
      }
      usleep(0);
    }
  }
  else {
    watch_start(watch); 
    
    while(current == last) {
      if((current = port->pa2) != last) {
	last = current;
        break;
      }
      usleep(0);
      
      if(watch_elapsed(watch) >= timeout) {
        result = false;
        goto done;
      }
    }
  }
 done:

  watch_free(watch);
  return result;
}

//------------------------------------------------------------------------------

unsigned char driver_shm_read(void) {
  return (direction == XLINK_DRIVER_STATE_INPUT) ? port->data : 0xff;
}

//------------------------------------------------------------------------------

void driver_shm_write(unsigned char value) {
  port->data = (direction == XLINK_DRIVER_STATE_OUTPUT) ? value : 0xff;
}

//------------------------------------------------------------------------------

bool driver_shm_send(unsigned char* data, int size) {

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

bool driver_shm_receive(unsigned char* data, int size) { 

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

bool driver_shm_ping() { 
  driver->output();
  driver->write(XLINK_COMMAND_PING);
  driver->strobe();
  return driver->wait(250);
}

//------------------------------------------------------------------------------

void driver_shm_reset(void) {

#if linux
  ushort peer = (ushort) strtol(port->id, NULL, 0);

  if(peer) {
    kill(peer, SIGUSR1);
  }

#elif windows
  HWND peer = NULL;
  UINT WM_USER_RESET = 0x584c;
  
  BOOL CALLBACK EnumWindowsProc(HWND parent, long unused) {
    peer = FindWindowExA(parent, NULL, port->id, NULL);
    return peer == NULL;
  }
  EnumWindows(EnumWindowsProc, 0);
    
  if(peer != NULL) {
    SendMessage(peer, WM_USER_RESET, 0, 0);
  }
  else {
    logger->error("Could not find child window with id %s "
                  "in any toplevel window", port->id);
  }
#endif
}

//------------------------------------------------------------------------------

void driver_shm_free(void) {

#if posix
    shmdt(&port);
#elif windows
    UnmapViewOfFile(port);
#endif
}

//------------------------------------------------------------------------------

void driver_shm_close(void) { /* nothing to close */ }
void driver_shm_boot(void) { /* nothing to boot */ };
