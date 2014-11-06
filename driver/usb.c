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

#if !(USB_VID + 0)
#error "Define an environment variable named USB_VID that contains the usb vendor id"
#endif

#if !(USB_PID + 0)
#error "Define an environment variable named USB_PID that contains the usb product id"
#endif

#define DRIVER_USB_MAX_PAYLOAD_SIZE 4096
#define MESSAGE_TIMEOUT 60000

extern Driver* driver;

static libusb_device_handle *handle = NULL;
static char response[1];

//------------------------------------------------------------------------------
// USB device discovery
//------------------------------------------------------------------------------

void driver_usb_lookup(char *path, DeviceInfo *info) {

  info->vid     = USB_VID;
  info->pid     = USB_PID;
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

  logger->trace("%s: vid: %04x pid: %04x bus: %d addr: %d serial: %s",
		path, info->vid, info->pid, info->bus, info->address, info->serial);
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

  const struct libusb_version* version;
  DeviceInfo info;
  int result;

  if((result = libusb_init(NULL)) < 0) {
    SET_ERROR(XLINK_ERROR_LIBUSB, "could not initialize libusb-1.0: %d", result);
    return false;
  }
  version = libusb_get_version();
  
  logger->trace("initialized libusb-%d.%d.%d.%d",
		version->major, version->minor, version->micro, version->nano);

  driver_usb_lookup(driver->path, &info);
  
  handle = driver_usb_open_device(NULL, &info);

  if(handle == NULL) {
    SET_ERROR(XLINK_ERROR_LIBUSB, "could not open device \"%s\"", driver->path);

    libusb_exit(NULL);
    return false;
  }

  control(USB_INIT);
  
  CLEAR_ERROR;
  return true;
}

//------------------------------------------------------------------------------

void driver_usb_strobe() {
  control(USB_STROBE);
}

//------------------------------------------------------------------------------

bool driver_usb_wait(int timeout) {
  
  response[0] = 0;

  bool acked() {
    if(controlEndpointIn(USB_ACKED, response, sizeof(response))) {
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

void driver_usb_write(char value) {
  controlEndpointOutWithValue(USB_WRITE, value);
} 

//------------------------------------------------------------------------------

char driver_usb_read() {
  controlEndpointIn(USB_READ, response, sizeof(response));
  return response[0];
} 

//------------------------------------------------------------------------------

void driver_usb_send(char* data, int size) {

  while(size > DRIVER_USB_MAX_PAYLOAD_SIZE) {

    controlEndpointOut(USB_SEND, data, DRIVER_USB_MAX_PAYLOAD_SIZE);
    
    data += DRIVER_USB_MAX_PAYLOAD_SIZE; 
    size -= DRIVER_USB_MAX_PAYLOAD_SIZE;
  }
  controlEndpointOut(USB_SEND, data, size);
}

//------------------------------------------------------------------------------

void driver_usb_receive(char* data, int size) {

  while(size > DRIVER_USB_MAX_PAYLOAD_SIZE) {

    controlEndpointIn(USB_RECEIVE, data, DRIVER_USB_MAX_PAYLOAD_SIZE);
    
    data += DRIVER_USB_MAX_PAYLOAD_SIZE; 
    size -= DRIVER_USB_MAX_PAYLOAD_SIZE;
  }
  controlEndpointIn(USB_RECEIVE, data, size);
}

//------------------------------------------------------------------------------

void driver_usb_input() {
  control(USB_INPUT);
}

//------------------------------------------------------------------------------

void driver_usb_output() {
  control(USB_OUTPUT);
}

//------------------------------------------------------------------------------

bool driver_usb_ping() { 
  driver->output();
  driver->write(0xff);
  driver->strobe();
  return driver->wait(XLINK_PING_TIMEOUT);
}

//------------------------------------------------------------------------------

void driver_usb_reset() { 
  control(USB_RESET);
}

//------------------------------------------------------------------------------

void driver_usb_boot() { 
  control(USB_BOOT);
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

int controlEndpointIn(int message, char *buffer, int size) {
  return controlEndpoint(message, buffer, size, LIBUSB_ENDPOINT_IN);
}

int controlEndpointOut(int message, char *buffer, int size) {
  return controlEndpoint(message, buffer, size, LIBUSB_ENDPOINT_OUT);
}

int controlEndpointOutWithValue(int message, int value) {
  return libusb_control_transfer(handle,
				 LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_DEVICE |
				 LIBUSB_ENDPOINT_OUT, 
				 message, value, 0, NULL, 0, MESSAGE_TIMEOUT);
  
}

int controlEndpoint(int message, char *buffer, int size, int direction) {
  return libusb_control_transfer(handle,
				 LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_DEVICE |
				 direction, 
				 message, 0, 0,
				 (unsigned char *)buffer, size, MESSAGE_TIMEOUT);
}

//------------------------------------------------------------------------------
