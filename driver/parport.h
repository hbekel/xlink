#ifndef PARPORT_H
#define PARPORT_H

#define DRIVER_PARPORT_CONTROL_STROBE 0x01
#define DRIVER_PARPORT_CONTROL_INIT   0x04
#define DRIVER_PARPORT_CONTROL_SELECT 0x08
#define DRIVER_PARPORT_CONTROL_IRQ    0x10
#define DRIVER_PARPORT_CONTROL_INPUT  0x20

bool driver_parport_open(void);
void driver_parport_close(void);
void driver_parport_strobe(void);
bool driver_parport_wait(int);
char driver_parport_read(void);
void driver_parport_write(char);
void driver_parport_send(char*, int);
void driver_parport_receive(char*, int);
void driver_parport_input(void);
void driver_parport_output(void);
bool driver_parport_ping(void);
void driver_parport_reset(void);
void driver_parport_boot(void);
void driver_parport_free(void);

#endif // PARPORT_H
