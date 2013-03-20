#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "target.h"
#include "pp64.h"

#if linux
  #include <sys/ioctl.h>
  #include <linux/parport.h>
  #include <linux/ppdev.h>

#elif windows
  #include <windows.h>

#endif

#define pp64_send_ack    pp64_send_signal_input
#define pp64_send_strobe pp64_send_signal_output

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

char* pp64_device = (char*) "/dev/parport0";
int pp64_port = 0x378;
unsigned char pp64_stat;

#if windows
typedef BOOL	(__stdcall *pp64_lpDriverOpened)(void);
typedef void	(__stdcall *pp64_lpOutb)(short, short);
typedef short	(__stdcall *pp64_lpInb)(short);

HINSTANCE pp64_inpout32 = NULL;
pp64_lpOutb pp64_outb;
pp64_lpInb pp64_inb;
pp64_lpDriverOpened pp64_driverOpened;
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

int pp64_open() {

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
    pp64_send_strobe();

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
    pp64_send_strobe();

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

int pp64_reset(void) {
  if(pp64_open()) {

    pp64_control(pp64_ctrl_reset);
    usleep(100*1000);
    pp64_control(pp64_ctrl_output);

    pp64_close();
    return true;
    #ifdef WIN32
    printf("win!\n");
    #endif
  }
  return false;
}

int pp64_ping(int timeout) {
  int ready = false;
  int start, now;
  unsigned char ping = 0xff;

  if(pp64_open()) {

    start = clock() / (CLOCKS_PER_SEC / 1000);
    
    while(!ready) {
      ready = pp64_send_with_timeout(ping); 
      now = clock() / (CLOCKS_PER_SEC / 1000);

      if(now - start > timeout)
	break;
    }
    pp64_close();
  }
  return ready;
}

inline unsigned char pp64_receive(void) {
  char byte;
  pp64_wait_strobe(0);
  byte = pp64_read();
  pp64_send_ack();
  return byte;
} 

inline void pp64_send(unsigned char byte) {
  pp64_write(byte);
  pp64_send_strobe();
  pp64_wait_ack(0);  
}

int pp64_send_with_timeout(unsigned char byte) {

  pp64_write(byte);
  pp64_send_strobe();
  return pp64_wait_ack(250);  
}

void pp64_close() {
#if linux
  ioctl(pp64_port, PPRELEASE);
  close(pp64_port);
#endif
}

inline void pp64_init(void) {
  pp64_control(pp64_ctrl_output);
  pp64_stat = pp64_status();
}

inline unsigned char pp64_read(void) {
  unsigned char byte = 0;
#if linux
  ioctl(pp64_port, PPRDATA, &byte);
#elif windows
  byte = pp64_inb(pp64_port);
#endif  
  return byte;
}

inline void pp64_write(unsigned char byte) {
#if linux
  ioctl(pp64_port, PPWDATA, &byte);
#elif windows
  pp64_outb(pp64_port, byte);
#endif
}

inline void pp64_control(unsigned char ctrl) {
#if linux
  ioctl(pp64_port, PPWCONTROL, &ctrl);
#elif windows
  pp64_outb(pp64_port+2, ctrl);
#endif

}

inline unsigned char pp64_status(void) {
  unsigned char status = 0;
#if linux
  ioctl(pp64_port, PPRSTATUS, &status);
#elif windows
  status = pp64_inb(pp64_port+1);
#endif  
  return status;
}

inline void pp64_send_signal_input(void) {
  pp64_control(pp64_ctrl_ack);
  pp64_control(pp64_ctrl_input);  
}

inline void pp64_send_signal_output(void) {
  pp64_control(pp64_ctrl_strobe);
  pp64_control(pp64_ctrl_output);
}

inline int pp64_receive_signal(int timeout) {  
  
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


