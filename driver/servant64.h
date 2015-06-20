#ifndef SERVANT64_H
#define SERVANT64_H

#include "xlink.h"

bool driver_servant64_open(void);
void driver_servant64_close(void);
void driver_servant64_strobe(void);
bool driver_servant64_wait(int);
unsigned char driver_servant64_read(void);
void driver_servant64_write(unsigned char);
bool driver_servant64_send(unsigned char*, int);
bool driver_servant64_receive(unsigned char*, int);
void driver_servant64_input(void);
void driver_servant64_output(void);
bool driver_servant64_ping(void);
void driver_servant64_reset(void);
void driver_servant64_boot(void);
void driver_servant64_free(void);

#endif // SERVANT64_H
