#ifndef DRIVER_H
#define DRIVER_H

typedef struct {
  char* path;
  int device;
  bool opened;

  bool (*_ready) (void);
  bool (*_open) (void);
  void (*_close) (void);
  void (*_strobe) (void);
  bool (*_wait) (int);
  char (*_read) (void);
  void (*_write) (char);
  void (*_send) (char*, int);
  void (*_receive) (char*, int);
  void (*_input) (void);
  void (*_output) (void);
  bool (*_ping) (void);
  void (*_reset) (void);
  void (*_boot) (void);
  void (*_free) (void);

  bool (*ready) (void);
  bool (*open) (void);
  void (*close) (void);
  void (*strobe) (void);
  bool (*wait) (int);
  char (*read) (void);
  void (*write) (char);
  void (*send) (char*, int);
  void (*receive) (char*, int);
  void (*input) (void);
  void (*output) (void);
  bool (*ping) (void);
  void (*reset) (void);
  void (*boot) (void);
  void (*free) (void);
} Driver;

Driver *driver_setup(char* path);
bool device_is_parport(char*);
bool device_is_usb(char*);

bool _driver_ready(void);
bool _driver_open(void);
void _driver_close(void);
void _driver_strobe(void);
bool _driver_wait(int);
char _driver_read(void);
void _driver_write(char);
void _driver_send(char*, int);
void _driver_receive(char*, int);
void _driver_input(void);
void _driver_output(void);
bool _driver_ping(void);
void _driver_reset(void);
void _driver_boot(void);
void _driver_free();

#endif // DRIVER_H
