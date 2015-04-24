#ifndef SHM_H
#define SHM_H

#include "xlink.h"

typedef struct {
  uchar flag;
  uchar pa2;
  uchar data;
  char id[256];
} xlink_port_t;    


bool driver_shm_open(void);
void driver_shm_close(void);
void driver_shm_strobe(void);
bool driver_shm_wait(int);
unsigned char driver_shm_read(void);
void driver_shm_write(unsigned char);
bool driver_shm_send(unsigned char*, int);
bool driver_shm_receive(unsigned char*, int);
void driver_shm_input(void);
void driver_shm_output(void);
bool driver_shm_ping(void);
void driver_shm_reset(void);
void driver_shm_boot(void);
void driver_shm_free(void);

#endif // SHM_H
