#ifndef USB_H
#define USB_H

#include <usb.h>

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

usb_dev_handle* _driver_usb_open_device(int vendorId, int productId);

#endif // USB_H
