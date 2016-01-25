#ifndef SERIAL_H
#define SERIAL_H

#include "xlink.h"

bool driver_serial_open(void);
void driver_serial_close(void);
void driver_serial_strobe(void);
bool driver_serial_wait(int);
unsigned char driver_serial_read(void);
void driver_serial_write(unsigned char);
bool driver_serial_send(unsigned char*, int);
bool driver_serial_receive(unsigned char*, int);
void driver_serial_input(void);
void driver_serial_output(void);
bool driver_serial_ping(void);
void driver_serial_reset(void);
void driver_serial_boot(void);
void driver_serial_free(void);

#endif // SERIAL_H
