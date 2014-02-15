#ifndef PARPORT_H
#define PARPORT_H

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
void driver_parport_flash(void);
void driver_parport_free(void);


#endif // PARPORT_H
