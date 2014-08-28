#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#if linux
  #include <libusb-1.0/libusb.h>
#elif windows
  #include <usb.h>
#endif

#include <sys/time.h>

#include "xlink.h"
#include "driver.h"
#include "protocol.h"
#include "util.h"
#include "usb.h"

#if !(USB_VID + 0)
#error "Define an environment variable named USB_VID that contains the usb vendor id"
#endif

#if !(USB_PID + 0)
#error "Define an environment variable named USB_PID that contains the usb product id"
#endif

#define DRIVER_USB_MAX_PAYLOAD_SIZE 4096
#define MESSAGE_TIMEOUT 60000

extern Driver* driver;

static usb_dev_handle *handle = NULL;
static char response[1];

//------------------------------------------------------------------------------
// USB driver implementation
//------------------------------------------------------------------------------

bool driver_usb_open() {
  
  handle = _driver_usb_open_device(USB_VID, "BREADBIN", USB_PID, "USB2C64");

  if(handle == NULL) {
    logger->error("could not find USB device");
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
  usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, 
                  USB_WRITE, value, 0, NULL, 0, MESSAGE_TIMEOUT);
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
  if(handle != NULL) 
    usb_close(handle);
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
  return usbMessageEndpoint(message, buffer, size, USB_ENDPOINT_IN);
}

int usbMessageEndpointOut(int message, char *buffer, int size) {
  return usbMessageEndpoint(message, buffer, size, USB_ENDPOINT_OUT);
}

int usbMessageEndpoint(int message, char *buffer, int size, int direction) {
  return usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | direction, 
                         message, 0, 0, (char *)buffer, size, MESSAGE_TIMEOUT);
}

//------------------------------------------------------------------------------

int _driver_usb_get_descriptor_string(usb_dev_handle *dev, int index, int langid, char *buf, int buflen) {

    char buffer[256];
    int rval, i;

    rval = usb_control_msg(dev, USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
			   USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, 
			   buffer, sizeof(buffer), 1000);
        
    if(rval < 0) 
      return rval;
	
    if((unsigned char)buffer[0] < rval)
      rval = (unsigned char)buffer[0];
    
    if(buffer[1] != USB_DT_STRING)
      return 0;
    
    rval /= 2;
    
    for(i = 1; i < rval && i < buflen; i++) {
      if(buffer[2 * i + 1] == 0)
	buf[i-1] = buffer[2 * i];
      else
	buf[i-1] = '?';
    }
    buf[i-1] = 0;
    
    return i-1;
}

//------------------------------------------------------------------------------

usb_dev_handle* _driver_usb_open_device(int vendor, char *vendorName, int product, char *productName) {

  struct usb_bus *bus;
  struct usb_device *dev;
  char devVendor[256], devProduct[256];
  
  usb_dev_handle * handle = NULL;
  
  usb_init();
  usb_find_busses();
  usb_find_devices();
  
  for(bus=usb_get_busses(); bus; bus=bus->next) {
    for(dev=bus->devices; dev; dev=dev->next) {			
      if(dev->descriptor.idVendor != vendor ||
         dev->descriptor.idProduct != product)
        continue;
      
      if(!(handle = usb_open(dev))) {
        logger->warn("could not open USB device: %s", usb_strerror());
        continue;
      }
      
      if(_driver_usb_get_descriptor_string(handle, dev->descriptor.iManufacturer, 
                                           0x0409, devVendor, sizeof(devVendor)) < 0) {
        logger->warn("could not query manufacturer for device: %s",  usb_strerror());
        usb_close(handle);
        continue;
      }
      
      if(_driver_usb_get_descriptor_string(handle, dev->descriptor.iProduct, 
                                           0x0409, devProduct, sizeof(devVendor)) < 0) {
        logger->warn("could not query product for device: %s", usb_strerror());
        usb_close(handle);
        continue;
      }
      
      if(strcmp(devVendor, vendorName) == 0 && 
         strcmp(devProduct, productName) == 0)
        return handle;
      else
        usb_close(handle);
    }
  } 
  return NULL;
}
