#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "target.h"
#include "extension.h"
#include "extensions.c"
#include "pp64.h"

#if linux
  #include <sys/ioctl.h>
  #include <linux/parport.h>
  #include <linux/ppdev.h>

#elif windows
  #include <windows.h>

#endif

#define PP64_PARPORT_CONTROL_STROBE 0x01
#define PP64_PARPORT_CONTROL_AUTOFD 0x02
#define PP64_PARPORT_CONTROL_INIT   0x04
#define PP64_PARPORT_CONTROL_SELECT 0x08
#define PP64_PARPORT_CONTROL_IRQ    0x10
#define PP64_PARPORT_CONTROL_INPUT  0x20

#define PP64_COMMAND_LOAD         0x01
#define PP64_COMMAND_SAVE         0x02
#define PP64_COMMAND_POKE         0x03
#define PP64_COMMAND_PEEK         0x04
#define PP64_COMMAND_JUMP         0x05
#define PP64_COMMAND_RUN          0x06
#define PP64_COMMAND_EXTEND       0x07

static void pp64_init(void);
static int pp64_open(void);
static void pp64_close(void);
static unsigned char pp64_read(void);
static void pp64_write(unsigned char byte);
static void pp64_control(unsigned char ctrl);
static unsigned char pp64_status(void);
static unsigned char pp64_receive(void);
static void pp64_send(unsigned char byte);
static int pp64_send_with_timeout(unsigned char byte);
static void pp64_send_signal_input(void);
static void pp64_send_signal_output(void);
static int pp64_receive_signal(int timeout); 

#define pp64_send_ack          pp64_send_signal_input
#define pp64_send_strobe       pp64_send_signal_output
#define pp64_send_strobe_input pp64_send_signal_input

#define pp64_wait_ack    pp64_receive_signal
#define pp64_wait_strobe pp64_receive_signal

const unsigned char pp64_ctrl_output = PP64_PARPORT_CONTROL_SELECT | 
                                       PP64_PARPORT_CONTROL_INIT | 
                                       PP64_PARPORT_CONTROL_IRQ;

const unsigned char pp64_ctrl_input  = PP64_PARPORT_CONTROL_SELECT | 
                                       PP64_PARPORT_CONTROL_INIT |
                                       PP64_PARPORT_CONTROL_INPUT |
                                       PP64_PARPORT_CONTROL_IRQ;

const unsigned char pp64_ctrl_reset  = PP64_PARPORT_CONTROL_SELECT | 
                                       PP64_PARPORT_CONTROL_IRQ;

const unsigned char pp64_ctrl_strobe = PP64_PARPORT_CONTROL_SELECT | 
                                       PP64_PARPORT_CONTROL_INIT | 
                                       PP64_PARPORT_CONTROL_STROBE |
                                       PP64_PARPORT_CONTROL_IRQ; 

const unsigned char pp64_ctrl_ack    = PP64_PARPORT_CONTROL_SELECT | 
                                       PP64_PARPORT_CONTROL_INIT |
                                       PP64_PARPORT_CONTROL_INPUT | 
                                       PP64_PARPORT_CONTROL_STROBE |
                                       PP64_PARPORT_CONTROL_IRQ; 

#if linux
static char* pp64_device = (char*) "/dev/parport0";
#endif
static int pp64_port = 0x378;
static unsigned char pp64_stat;

#if windows
typedef BOOL	(__stdcall *pp64_lpDriverOpened)(void);
typedef void	(__stdcall *pp64_lpOutb)(short, short);
typedef short	(__stdcall *pp64_lpInb)(short);

static HINSTANCE pp64_inpout32 = NULL;
static pp64_lpOutb pp64_outb;
static pp64_lpInb pp64_inb;
static pp64_lpDriverOpened pp64_driverOpened;
#endif

int pp64_configure(char* spec) {

#if linux
  pp64_device = (char*) realloc(pp64_device, (strlen(spec)+1) * sizeof(char)); 
  strncpy(pp64_device, spec, strlen(spec)+1);

#elif windows
  if (strstr(spec, "0x") == spec && strlen(spec) == 5) {
    pp64_port = strtol(spec, NULL, 0);
  }
  else {
    fprintf(stderr, "pp64: error: port must be specified as 0xnnn\n");
    return false;
  }
#endif
  return true;
}

int pp64_ping(int timeout) {
  int pong = false;
  int start, now;
  unsigned char ping = 0xff;

  if(pp64_open()) {

    start = clock() / (CLOCKS_PER_SEC / 1000);
    
    while(!pong) {
      pong = pp64_send_with_timeout(ping); 
      now = clock() / (CLOCKS_PER_SEC / 1000);

      if(now - start > timeout)
	break;
    }
    pp64_close();
  }
  return pong;
}

int pp64_load(unsigned char memory, 
	       unsigned char bank, 
	       int start, 
	       int end, 
	       char* data, int size) {

  unsigned char command = PP64_COMMAND_LOAD;
  
  if(pp64_open()) {

    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_send(memory);
    pp64_send(bank);
    pp64_send(start & 0xff);
    pp64_send(start >> 8);
    pp64_send(end & 0xff);
    pp64_send(end >> 8);
  
    int i;
    for(i=0; i<size; i++)
      pp64_send(data[i]);

    pp64_close();
    return true;
  }
  return false;
}

int pp64_save(unsigned char memory, 
	       unsigned char bank, 
	       int start, 
	       int end, 
	       char* data, int size) {

  unsigned char command = PP64_COMMAND_SAVE;
  
  if(pp64_open()) {

    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_send(memory);
    pp64_send(bank);
    pp64_send(start & 0xff);
    pp64_send(start >> 8);
    pp64_send(end & 0xff);
    pp64_send(end >> 8);

    pp64_control(pp64_ctrl_input);
    pp64_send_strobe_input();

    int i;
    for(i=0; i<size; i++)
      data[i] = pp64_receive();

    pp64_close();
    return true;
  }
  return false;
}

int pp64_peek(unsigned char memory, unsigned char bank, int address, unsigned char* value) {
  
  unsigned char command = PP64_COMMAND_PEEK;

  if(pp64_open()) {
  
    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_send(memory);
    pp64_send(bank);
    pp64_send(address & 0xff);
    pp64_send(address >> 8);    

    pp64_control(pp64_ctrl_input);
    pp64_send_strobe_input();

    *value = pp64_receive();

    pp64_close();

    return true;
  }
  return false;
}

int pp64_poke(unsigned char memory, unsigned char bank, int address, unsigned char value) {

  unsigned char command = PP64_COMMAND_POKE;

  if(pp64_open()) {
  
    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_send(memory);
    pp64_send(bank);
    pp64_send(address & 0xff);
    pp64_send(address >> 8);    
    pp64_send(value);

    pp64_close();
    return true;
  }
  return false;
}

int pp64_jump(unsigned char memory, unsigned char bank, int address) {

  unsigned char command = PP64_COMMAND_JUMP;

  if(pp64_open()) {
  
    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      pp64_close();
      return false;
    }
    pp64_send(memory);
    pp64_send(bank);
    pp64_send(address >> 8);    // target address is send MSB first (big-endian)
    pp64_send(address & 0xff);
    
    pp64_close();
    return true;
  }
  return false;
}

int pp64_run(void) {

  unsigned char command = PP64_COMMAND_RUN;

  if(pp64_open()) {     
    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      return false;
    }
    pp64_close();
  }
  return true;
}

int pp64_drive_status(unsigned char* status) {

  unsigned char byte;
  int result = false;

  Extension *lib = EXTENSION_LIB;
  Extension *drive_status = EXTENSION_DRIVE_STATUS;

  if (extension_load(lib) && extension_load(drive_status) && extension_init(drive_status)) {

    if (pp64_open()) {
      
      pp64_control(pp64_ctrl_input);
      pp64_send_strobe_input();

      int i;
      for(i=0; (byte = pp64_receive()) != 0xff; i++) {
	status[i] = byte;
      }

      pp64_wait_ack(0);
      pp64_control(pp64_ctrl_output);

      pp64_close();
      result = true;
    }
  }

  extension_free(lib);
  extension_free(drive_status);

  return result;
}

int pp64_dos(char* cmd) {

  int result = false;

  Extension *lib = EXTENSION_LIB;
  Extension *dos_command = EXTENSION_DOS_COMMAND;

  if (extension_load(lib) && extension_load(dos_command) && extension_init(dos_command)) {

    if (pp64_open()) {

      pp64_send(strlen(cmd));

      int i;
      for(i=0; i<strlen(cmd); i++) {
	pp64_send(toupper(cmd[i]));
      }

      pp64_wait_ack(0);

      pp64_close();
      result = true;
    }
  }

  extension_free(lib);
  extension_free(dos_command);
  
  return result;
}

int pp64_sector_read(unsigned char track, unsigned char sector, unsigned char* data) {
  
  int result = false;
  char U1[13];

  Extension *lib = EXTENSION_LIB;
  Extension *sector_read = EXTENSION_SECTOR_READ;

  if (extension_load(lib) && extension_load(sector_read) && extension_init(sector_read)) {

    if (pp64_open()) {
      
      sprintf(U1, "U1 2 0 %02d %02d", track, sector);

      int i;
      for(i=0; i<strlen(U1); i++)
	pp64_send(U1[i]);      
      
      pp64_control(pp64_ctrl_input);
      pp64_send_strobe_input();
      
      for(i=0; i<256; i++)
	data[i] = pp64_receive();
      
      pp64_wait_ack(0);
      pp64_control(pp64_ctrl_output);
      
      pp64_close();
      
      result = true;
    }
  }

  extension_free(lib);
  extension_free(sector_read);

  return result;
}

int pp64_sector_write(unsigned char track, unsigned char sector, unsigned char *data) {

  int result = false;
  char U2[13];

  Extension *lib = EXTENSION_LIB;
  Extension *sector_write = EXTENSION_SECTOR_WRITE;

  if (extension_load(lib) && extension_load(sector_write) && extension_init(sector_write)) {

    if (pp64_open()) {
      sprintf(U2, "U2 2 0 %02d %02d", track, sector);

      int i;
      for(i=0; i<256; i++)
	pp64_send(data[i]);
      
      for(i=0; i<strlen(U2); i++)
	pp64_send(U2[i]);
      
      pp64_wait_ack(0);
      
      pp64_close();
      result = true;
    }
  }
  
  extension_free(lib);
  extension_free(sector_write);

  return result;
}

int pp64_extend(int address) {

  unsigned char command = PP64_COMMAND_EXTEND;

  if(pp64_open()) {
  
    if(!pp64_send_with_timeout(command)) {
      fprintf(stderr, "pp64: error: no response from C64\n");
      pp64_close();
      return false;
    }
    // send the address-1 high byte first, so the server can 
    // just push it on the stack and rts
    
    pp64_send(address >> 8);       // first the highbyte,
    pp64_send((address & 0xff)-1); // then the lowbyte-1
    
    pp64_close();
    return true;
  }
  return false;
}

int pp64_reset(void) {
  if(pp64_open()) {

    pp64_control(pp64_ctrl_reset);
    usleep(100*1000);
    pp64_control(pp64_ctrl_output);

    pp64_close();
    return true;
  }
  return false;
}

static inline unsigned char pp64_receive(void) {
  char byte;
  pp64_wait_strobe(0);
  byte = pp64_read();
  pp64_send_ack();
  return byte;
} 

static inline void pp64_send(unsigned char byte) {
  pp64_write(byte);
  pp64_send_strobe();
  pp64_wait_ack(0);  
}

static inline int pp64_send_with_timeout(unsigned char byte) {

  pp64_write(byte);
  pp64_send_strobe();
  return pp64_wait_ack(250);  
}

static int pp64_open() {

#if linux  
  if((pp64_port = open(pp64_device, O_RDWR)) == -1) {
    fprintf(stderr, "pp64: error: couldn't open %s\n", pp64_device);
    return false;
  }  
  ioctl(pp64_port, PPCLAIM);
  pp64_init();
  return true;

#elif windows
  if(pp64_inpout32 == NULL) {
    
    pp64_inpout32 = LoadLibrary( "inpout32.dll" ) ;	
    
    if (pp64_inpout32 != NULL) {
      
      pp64_driverOpened = (pp64_lpDriverOpened) GetProcAddress(pp64_inpout32, "IsInpOutDriverOpen");
      pp64_outb = (pp64_lpOutb) GetProcAddress(pp64_inpout32, "Out32");
      pp64_inb = (pp64_lpInb) GetProcAddress(pp64_inpout32, "Inp32");		
      
      if (pp64_driverOpened) {
	pp64_init();
	return true;
      }
      else {
	fprintf(stderr, "pp64: error: failed to start inpout32 driver\n");
      }		
    }
    else {
      fprintf(stderr, "pp64: error: failed to load inpout32.dll\n\n");
      fprintf(stderr, "Inpout32 is required for parallel port access:\n\n");    
      fprintf(stderr, "    http://www.highrez.co.uk/Downloads/InpOut32/\n\n");	
    }
  }
  return false;
#endif
}

static inline void pp64_close() {
#if linux
  ioctl(pp64_port, PPRELEASE);
  close(pp64_port);
#endif
}

static inline void pp64_init(void) {
  pp64_control(pp64_ctrl_output);
  pp64_stat = pp64_status();
}

static inline unsigned char pp64_read(void) {
  unsigned char byte = 0;
#if linux
  ioctl(pp64_port, PPRDATA, &byte);
#elif windows
  byte = pp64_inb(pp64_port);
#endif  
  return byte;
}

static inline void pp64_write(unsigned char byte) {
#if linux
  ioctl(pp64_port, PPWDATA, &byte);
#elif windows
  pp64_outb(pp64_port, byte);
#endif
}

static inline void pp64_control(unsigned char ctrl) {
#if linux
  ioctl(pp64_port, PPWCONTROL, &ctrl);
#elif windows
  pp64_outb(pp64_port+2, ctrl);
#endif
}

static inline unsigned char pp64_status(void) {
  unsigned char status = 0;
#if linux
  ioctl(pp64_port, PPRSTATUS, &status);
#elif windows
  status = pp64_inb(pp64_port+1);
#endif  
  return status;
}

static inline void pp64_send_signal_input(void) {
  pp64_control(pp64_ctrl_ack);
  pp64_control(pp64_ctrl_input);  
}

static inline void pp64_send_signal_output(void) {
  pp64_control(pp64_ctrl_strobe);
  pp64_control(pp64_ctrl_output);
}

static inline int pp64_receive_signal(int timeout) {  
  
  unsigned char current = pp64_stat;
  clock_t start, now;

  if (timeout <= 0) {
    while (current == pp64_stat) {
      current = pp64_status();
    }
  }
  else {
    start = clock() / (CLOCKS_PER_SEC / 1000);
    
    while (current == pp64_stat) {
      current = pp64_status();

      now = clock() / (CLOCKS_PER_SEC / 1000);  
      if (now - start >= timeout)
	return false;
    }
  }
  pp64_stat = current;

  return true;
}

