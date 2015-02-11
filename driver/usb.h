#ifndef USB_H
#define USB_H

#include <libusb-1.0/libusb.h>

typedef struct {
  unsigned int vid;
  unsigned int pid;
  int bus;
  int address;
  char *serial;
} DeviceInfo;

void driver_usb_lookup(char* path, DeviceInfo* info);
libusb_device_handle* driver_usb_open_device(libusb_context* context, DeviceInfo *info);

bool driver_usb_open(void);
void driver_usb_close(void);
void driver_usb_strobe (void);
bool driver_usb_wait(int);
unsigned char driver_usb_read(void);
void driver_usb_write(unsigned char);
void driver_usb_send(unsigned char*, int);
void driver_usb_receive(unsigned char*, int);
void driver_usb_input(void);
void driver_usb_output(void);
bool driver_usb_ping(void);
void driver_usb_reset(void);
void driver_usb_boot(void);
void driver_usb_free(void);

int control(int message);
int controlEndpointIn(int message, unsigned char *buffer, int size);
int controlEndpointOut(int message, unsigned char *buffer, int size);
int controlEndpointOutWithValue(int message, int value);
int controlEndpoint(int message, unsigned char *buffer, int size, int direction);

#endif // USB_H
