#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <libusb-1.0/libusb.h>

#include "xlink.h"
#include "driver.h"
#include "protocol.h"
#include "usb.h"
#include "util.h"

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
// USB driver implementation
//------------------------------------------------------------------------------

bool driver_usb_open() {

  int result;
  const struct libusb_version* version;
  
  if((result = libusb_init(NULL)) < 0) {
    logger->debug("could not initialize libusb-1.0: %d", result);
    return false;
  }

  version = libusb_get_version();
  
  logger->debug("successfully initialized libusb-%d.%d.%d.%d",
		version->major, version->minor, version->micro, version->nano);
  
  handle = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID);

  if(handle == NULL) {
    logger->debug("could not find USB device %04x:%04x", USB_VID, USB_PID);
    libusb_exit(NULL);
    return false;
  }
  usbMessage(USB_INIT);
  return true;
}

//------------------------------------------------------------------------------

void driver_usb_strobe() {
  usbMessage(USB_STROBE);
}

//------------------------------------------------------------------------------

bool driver_usb_wait(int timeout) {
  
  response[0] = 0;

  bool acked() {
    if(usbMessageEndpointIn(USB_ACKED, response, sizeof(response))) {
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
  usbMessageEndpointOutWithValue(USB_WRITE, value);
} 

//------------------------------------------------------------------------------

char driver_usb_read() {
  usbMessageEndpointIn(USB_READ, response, sizeof(response));
  return response[0];
} 

//------------------------------------------------------------------------------

void driver_usb_send(char* data, int size) {

  while(size > DRIVER_USB_MAX_PAYLOAD_SIZE) {

    usbMessageEndpointOut(USB_SEND, data, DRIVER_USB_MAX_PAYLOAD_SIZE);
    
    data += DRIVER_USB_MAX_PAYLOAD_SIZE; 
    size -= DRIVER_USB_MAX_PAYLOAD_SIZE;
  }
  usbMessageEndpointOut(USB_SEND, data, size);
}

//------------------------------------------------------------------------------

void driver_usb_receive(char* data, int size) {

  while(size > DRIVER_USB_MAX_PAYLOAD_SIZE) {

    usbMessageEndpointIn(USB_RECEIVE, data, DRIVER_USB_MAX_PAYLOAD_SIZE);
    
    data += DRIVER_USB_MAX_PAYLOAD_SIZE; 
    size -= DRIVER_USB_MAX_PAYLOAD_SIZE;
  }
  usbMessageEndpointIn(USB_RECEIVE, data, size);
}

//------------------------------------------------------------------------------

void driver_usb_input() {
  usbMessage(USB_INPUT);
}

//------------------------------------------------------------------------------

void driver_usb_output() {
  usbMessage(USB_OUTPUT);
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
  usbMessage(USB_RESET);
}

//------------------------------------------------------------------------------

void driver_usb_boot() { 
  usbMessage(USB_BOOT);
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

int usbMessage(int message) {
  return usbMessageEndpointOut(message, NULL, 0);
}

int usbMessageEndpointIn(int message, char *buffer, int size) {
  return usbMessageEndpoint(message, buffer, size, LIBUSB_ENDPOINT_IN);
}

int usbMessageEndpointOut(int message, char *buffer, int size) {
  return usbMessageEndpoint(message, buffer, size, LIBUSB_ENDPOINT_OUT);
}

int usbMessageEndpointOutWithValue(int message, int value) {
  return libusb_control_transfer(handle,
				 LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_DEVICE |
				 LIBUSB_ENDPOINT_OUT, 
				 message, value, 0, NULL, 0, MESSAGE_TIMEOUT);
  
}

int usbMessageEndpoint(int message, char *buffer, int size, int direction) {
  return libusb_control_transfer(handle,
				 LIBUSB_REQUEST_TYPE_VENDOR |
				 LIBUSB_RECIPIENT_DEVICE |
				 direction, 
				 message, 0, 0,
				 (unsigned char *)buffer, size, MESSAGE_TIMEOUT);
}

//------------------------------------------------------------------------------
