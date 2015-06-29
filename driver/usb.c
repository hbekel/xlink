#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <libusb-1.0/libusb.h>

#include "xlink.h"
#include "error.h"
#include "driver.h"
#include "protocol.h"
#include "usb.h"
#include "util.h"
#include "target.h"

#define MAX_PAYLOAD_SIZE 4096

extern Driver* driver;

static libusb_device_handle *handle = NULL;
static unsigned char response[1];

//------------------------------------------------------------------------------
// USB device discovery
//------------------------------------------------------------------------------

void driver_usb_lookup(char *path, DeviceInfo *info) {

  info->vid     = 0x1d50;
  info->pid     = 0x60c8;
  info->bus     = -1;
  info->address = -1;
  info->serial  = NULL;
  
#if linux

  char* real = realpath(path, NULL);
  char* tmp = real;
  char* bus;
  char* address;
  
  char prefix[] = "/dev/bus/usb/";
  
  if(strstr(real, prefix) == real) {

    tmp += strlen(prefix);
    tmp[3] = '\0';

    bus = tmp;
    address = tmp+4;

    info->bus = strtol(bus, NULL, 10);
    info->address = strtol(address, NULL, 10);
  }
  
  free(real);

#elif windows

  char *colon;
  if((colon = strstr(path, ":")) != NULL) {
    info->serial = colon+1;
  }
#endif
}

//------------------------------------------------------------------------------

libusb_device_handle* driver_usb_open_device(libusb_context* context, DeviceInfo *info) {

  libusb_device **devices;
  libusb_device *device;
  struct libusb_device_descriptor descriptor;
  libusb_device_handle* handle = NULL;
  
  char serial[256];
  int result;
  int i = 0;
  
  if((result = libusb_get_device_list(context, &devices)) < 0) {
    SET_ERROR(XLINK_ERROR_LIBUSB, "could not get usb device list: %d", result);
    return NULL;
  }
  
  while ((device = devices[i++]) != NULL) {
    
    if((result = libusb_get_device_descriptor(device, &descriptor)) < 0) {
      logger->debug("could not get usb device descriptor: %d", result);
      continue;
    }
    
    if(descriptor.idVendor == info->vid &&
       descriptor.idProduct == info->pid) {
      
      if(info->bus > -1) {
        if(libusb_get_bus_number(device) != info->bus) {
          continue;
        }
      }
      
      if(info->address > -1) {
        if(libusb_get_device_address(device) != info->address) {
          continue;
        }
      }
      
      if(info->serial != NULL) {
        
        if(descriptor.iSerialNumber != 0) {
          
          if((result = libusb_open(device, &handle)) < 0) {
            logger->debug("could not open usb device %03d/%03d",
                          libusb_get_bus_number(device),
                          libusb_get_device_address(device));
            continue;
          }
          
          result = libusb_get_string_descriptor_ascii(handle, descriptor.iSerialNumber,
                                                      (unsigned char *) &serial, sizeof(serial));
          
          if(result < 0) {
            logger->debug("could not get serial number from device: %d", result);
            goto skip;
          }
          
          if(strcmp(serial, info->serial) == 0) {
            goto done;
          }
          
        skip:
          libusb_close(handle);
          handle = NULL;
          continue;
        }
      }
      
      if(handle == NULL) {
        if((result = libusb_open(device, &handle)) < 0) {
          SET_ERROR(XLINK_ERROR_LIBUSB,
                    "could not open usb device %03d/%03d",
                    libusb_get_bus_number(device),
                    libusb_get_device_address(device));
          
          handle = NULL;
        }
      }
      goto done;
    }
  }
  
 done:
  libusb_free_device_list(devices, true);

  CLEAR_ERROR_IF(handle != NULL);
  return handle;
}

//------------------------------------------------------------------------------
// USB driver implementation
//------------------------------------------------------------------------------

bool driver_usb_open() {

  DeviceInfo info;
  int result;

  if((result = libusb_init(NULL)) < 0) {
    SET_ERROR(XLINK_ERROR_LIBUSB, "could not initialize libusb-1.0: %d", result);
    return false;
  }
  
  driver_usb_lookup(driver->path, &info);
  
  handle = driver_usb_open_device(NULL, &info);

  if(handle == NULL) {
    SET_ERROR(XLINK_ERROR_LIBUSB, "could not open device \"%s\"", driver->path);

    libusb_exit(NULL);
    return false;
  }

  control(CMD_INPUT);

  CLEAR_ERROR;
  return true;
}

//------------------------------------------------------------------------------

void driver_usb_strobe() {
  control(CMD_STROBE);
}

//------------------------------------------------------------------------------

bool driver_usb_wait(int timeout) {
  
  response[0] = 0;

  bool acked() {
    if(controlEndpointIn(CMD_ACKED, response, sizeof(response))) {
      return response[0] == 1;
    }
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

void driver_usb_write(unsigned char value) {
  controlEndpointOutWithValue(CMD_WRITE, value);
} 

//------------------------------------------------------------------------------

unsigned char driver_usb_read() {
  controlEndpointIn(CMD_READ, response, sizeof(response));
  return response[0];
} 

//------------------------------------------------------------------------------

bool driver_usb_send(unsigned char* data, int size) {

  int sent = 0;
  
  bool send(ushort chunk) {

    int transfered = controlEndpointOut(CMD_SEND, data, chunk);

    if(transfered < 0) {
      return false;
    }

    if (transfered < chunk) { 
      sent += transfered;
      return false;
    }
    
    data += chunk;
    sent += chunk;
    return true;
  }

  chunked(send, MAX_PAYLOAD_SIZE, size);

  bool result = sent == size;
  
  if(!result) {
    SET_ERROR(XLINK_ERROR_LIBUSB,
              "transfer timeout (%d of %d bytes sent)", sent, size);
  }
  
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

bool driver_usb_receive(unsigned char* data, int size) {
  int received = 0;
  
  bool receive(ushort chunk) {

    int transfered = controlEndpointIn(CMD_RECEIVE, data, chunk);

    if(transfered < 0) { 
      return false;
    }

    if (transfered < chunk) { 
      received += transfered;
      return false;
    }
    
    data += chunk;
    received += chunk;
    return true;
  }

  chunked(receive, MAX_PAYLOAD_SIZE, size);

  bool result = received == size;
  
  if(!result) {
    SET_ERROR(XLINK_ERROR_LIBUSB,
              "transfer timeout (%d of %d bytes received)", received, size);
  }
  
  CLEAR_ERROR_IF(result);
  return result;
}

//------------------------------------------------------------------------------

void driver_usb_input() {
  control(CMD_INPUT);
}

//------------------------------------------------------------------------------

void driver_usb_output() {
  control(CMD_OUTPUT);
}

//------------------------------------------------------------------------------

bool driver_usb_ping() { 
  driver->output();
  driver->write(XLINK_COMMAND_PING);
  driver->strobe();
  return driver->wait(250);
}

//------------------------------------------------------------------------------

void driver_usb_reset() { 
  control(CMD_RESET);
}

//------------------------------------------------------------------------------

void driver_usb_boot() { 
  control(CMD_BOOT);
}

//------------------------------------------------------------------------------

void driver_usb_close() {
  if(handle != NULL) {
    libusb_close(handle);
    libusb_exit(NULL);
  }
}

//------------------------------------------------------------------------------

void driver_usb_free() {
  handle = NULL;
}

//------------------------------------------------------------------------------
// USB utility functions
//------------------------------------------------------------------------------

int control(int message) {
  return controlEndpointOut(message, NULL, 0);
}

int controlEndpointIn(int message, unsigned char *buffer, int size) {
  return controlEndpoint(message, buffer, size, LIBUSB_ENDPOINT_IN);
}

int controlEndpointOut(int message, unsigned char *buffer, int size) {
  return controlEndpoint(message, buffer, size, LIBUSB_ENDPOINT_OUT);
}

int controlEndpointOutWithValue(int message, int value) {
  return libusb_control_transfer(handle,
                                 LIBUSB_REQUEST_TYPE_VENDOR |
                                 LIBUSB_RECIPIENT_DEVICE |
                                 LIBUSB_ENDPOINT_OUT, 
                                 message, value, driver->timeout,
                                 NULL, 0, (driver->timeout+1)*1036);
  
}

int controlEndpoint(int message, unsigned char *buffer, int size, int direction) {
  return libusb_control_transfer(handle,
                                 LIBUSB_REQUEST_TYPE_VENDOR |
                                 LIBUSB_RECIPIENT_DEVICE |
                                 direction, 
                                 message, 0, driver->timeout,
                                 buffer, size, (driver->timeout+1)*1036);
}

//------------------------------------------------------------------------------
