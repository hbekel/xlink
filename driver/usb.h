#ifndef USB_H
#define USB_H

#include "../target.h"

#if linux
  #include <libusb-1.0/libusb.h>
#elif windows
  #include <usb.h>
#endif

bool driver_usb_open(void);
void driver_usb_close(void);
void driver_usb_strobe (void);
bool driver_usb_wait(int);
char driver_usb_read(void);
void driver_usb_write(char);
void driver_usb_send(char*, int);
void driver_usb_receive(char*, int);
void driver_usb_input(void);
void driver_usb_output(void);
bool driver_usb_ping(void);
void driver_usb_reset(void);
void driver_usb_boot(void);
void driver_usb_free(void);

int usbMessage(int message);
int usbMessageEndpointIn(int message, char *buffer, int size);
int usbMessageEndpointOut(int message, char *buffer, int size);
int usbMessageEndpoint(int message, char *buffer, int size, int direction);

int _driver_usb_get_descriptor_string(usb_dev_handle *dev, int index, int langid, char *buf, int buflen);
usb_dev_handle* _driver_usb_open_device(int vendor, char *vendorName, int product, char *productName);

#endif // USB_H
