#ifndef DRIVER_H
#define DRIVER_H

#define XLINK_DRIVER_DEVICE_USB       189
#define XLINK_DRIVER_DEVICE_PARPORT   99
#define XLINK_DRIVER_DEVICE_SHM       -1
#define XLINK_DRIVER_DEVICE_SERVANT64 188

#define XLINK_DRIVER_STATE_IDLE   0x01
#define XLINK_DRIVER_STATE_INPUT  0x02
#define XLINK_DRIVER_STATE_OUTPUT 0x03

typedef struct {
  char* path;
  int device;
  int timeout;
  int state;

  bool (*_ready) (void);
  bool (*_open) (void);
  void (*_close) (void);
  void (*_strobe) (void);
  bool (*_wait) (int);
  unsigned char (*_read) (void);
  void (*_write) (unsigned char);
  bool (*_send) (unsigned char*, int);
  bool (*_receive) (unsigned char*, int);
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
  unsigned char (*read) (void);
  void (*write) (unsigned char);
  bool (*send) (unsigned char*, int);
  bool (*receive) (unsigned char*, int);
  void (*input) (void);
  void (*output) (void);
  bool (*ping) (void);
  void (*reset) (void);
  void (*boot) (void);
  void (*free) (void);
} Driver;

bool driver_setup(char*);
bool device_identify(char*, int*); 
bool device_is_supported(char*, int);
bool device_is_parport(int);
bool device_is_usb(int);
bool device_is_shm(int);
bool device_is_servant64(int);

bool _driver_setup_and_open(void);
bool _driver_ready(void);
bool _driver_open(void);
void _driver_close(void);
void _driver_strobe(void);
bool _driver_wait(int);
unsigned char _driver_read(void);
void _driver_write(unsigned char);
bool _driver_send(unsigned char*, int);
bool _driver_receive(unsigned char*, int);
void _driver_input(void);
void _driver_output(void);
bool _driver_ping(void);
void _driver_reset(void);
void _driver_boot(void);
void _driver_free();

#endif // DRIVER_H
