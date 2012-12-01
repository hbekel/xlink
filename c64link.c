#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#include "c64link.h"

#define cable_send_ack _cable_send_signal_input
#define cable_send_strobe _cable_send_signal_output

#define cable_wait_ack _cable_receive_signal
#define cable_wait_strobe _cable_receive_signal

#define CABLE_LOAD 0x01
#define CABLE_SAVE 0x02
#define CABLE_PEEK 0x03
#define CABLE_POKE 0x04
#define CABLE_JUMP 0x05
#define CABLE_RUN  0x06

#define PARPORT_CONTROL_IRQ 0x10
#define PARPORT_CONTROL_INPUT 0x20

const unsigned char ctrl_output = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT | 
                                  PARPORT_CONTROL_IRQ;

const unsigned char ctrl_input  = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT |
                                  PARPORT_CONTROL_INPUT |
                                  PARPORT_CONTROL_IRQ;

const unsigned char ctrl_reset  = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_IRQ;

const unsigned char ctrl_strobe = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT | 
                                  PARPORT_CONTROL_STROBE |
                                  PARPORT_CONTROL_IRQ; 

const unsigned char ctrl_ack    = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT |
                                  PARPORT_CONTROL_INPUT | 
                                  PARPORT_CONTROL_STROBE |
                                  PARPORT_CONTROL_IRQ; 

int port;

int cable_open() {
  
  if((port = open("/dev/parport0", O_RDWR)) == -1) {
    fprintf(stderr, "c64link: error: couldn't open /dev/parport0\n");
    return false;
  }
  
  ioctl(port, PPCLAIM);
  ioctl(port, PPWCONTROL, &ctrl_output);  
  return true;
}

void cable_close() {
  ioctl(port, PPRELEASE);
  close(port);
}

int cable_load(unsigned char memory, 
	       unsigned char bank, 
	       int start, 
	       int end, 
	       char* data, int size) {

  unsigned char command = CABLE_LOAD;
  
  if(cable_open()) {

    if(!cable_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      cable_close();
      return false;
    }

    cable_write(memory);
    cable_write(bank);
    cable_write(start & 0xff);
    cable_write(start >> 8);
    cable_write(end & 0xff);
    cable_write(end >> 8);
  
    int i;
    for(i=0; i<size; i++)
      cable_write(data[i]);

    cable_close();
    return true;
  }
  return false;
}

int cable_save(unsigned char memory, 
	       unsigned char bank, 
	       int start, 
	       int end, 
	       char* data, int size) {

  unsigned char command = CABLE_SAVE;
  
  if(cable_open()) {

    if(!cable_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      cable_close();
      return false;
    }

    cable_write(memory);
    cable_write(bank);
    cable_write(start & 0xff);
    cable_write(start >> 8);
    cable_write(end & 0xff);
    cable_write(end >> 8);

    ioctl(port, PPWCONTROL, &ctrl_input);
    cable_send_strobe();

    int i;
    for(i=0; i<size; i++)
      data[i] = cable_read();

    cable_close();
    return true;
  }
  return false;
}

unsigned char cable_peek(unsigned char memory, unsigned char bank, int address) {
  
  unsigned char command = CABLE_PEEK;
  unsigned char result;

  if(cable_open()) {
  
    if(!cable_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      cable_close();
      exit(EXIT_FAILURE);
    }

    cable_write(memory);
    cable_write(bank);
    cable_write(address & 0xff);
    cable_write(address >> 8);    

    ioctl(port, PPWCONTROL, &ctrl_input);
    cable_send_strobe();

    result = cable_read();

    cable_close();

    return result;
  }
  return 0;
}

int cable_poke(unsigned char memory, unsigned char bank, int address, unsigned char value) {

  unsigned char command = CABLE_POKE;

  if(cable_open()) {
  
    if(!cable_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      cable_close();
      return false;
    }

    cable_write(memory);
    cable_write(bank);
    cable_write(address & 0xff);
    cable_write(address >> 8);    
    cable_write(value);

    cable_close();
    return true;
  }
  return false;
}

int cable_jump(unsigned char memory, unsigned char bank, int address) {

  unsigned char command = CABLE_JUMP;

  if(cable_open()) {
  
    if(!cable_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      cable_close();
      return false;
    }
    cable_write(memory);
    cable_write(bank);
    cable_write(address >> 8);    // target address is send MSB first (big-endian)
    cable_write(address & 0xff);
    
    cable_close();
    return true;
  }
  return false;
}

void cable_run(void) {

  unsigned char command = CABLE_RUN;

  if(cable_open()) {     
    if(!cable_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
    }
    cable_close();
  }
}

void cable_reset(void) {
  if(cable_open()) {
    ioctl(port, PPWCONTROL, &ctrl_reset);
    usleep(100*1000);
    ioctl(port, PPWCONTROL, &ctrl_output);
    cable_close();
  }
}

inline unsigned char cable_read(void) {
  char byte;
  cable_wait_strobe(NULL);
  ioctl(port, PPRDATA, &byte);
  cable_send_ack();
  return byte;
} 

inline void cable_write(unsigned char byte) {
  ioctl(port, PPWDATA, &byte);
  cable_send_strobe();
  cable_wait_ack(NULL);  
}

inline int cable_write_with_timeout(unsigned char byte) {

  struct timeval tv = { 0, 150*1000 };

  ioctl(port, PPWDATA, &byte);
  cable_send_strobe();
  return cable_wait_ack(&tv);  
}

inline void _cable_send_signal_input(void) {
  ioctl(port, PPWCONTROL, &ctrl_ack);
  ioctl(port, PPWCONTROL, &ctrl_input);  
}

inline void _cable_send_signal_output(void) {
  ioctl(port, PPWCONTROL, &ctrl_strobe);
  ioctl(port, PPWCONTROL, &ctrl_output);
}

inline int _cable_receive_signal(struct timeval* timeout) {  
  
  fd_set rfds;
  int ignored = 0;

  FD_ZERO(&rfds);
  FD_SET(port, &rfds);

  if(select(port + 1, &rfds, NULL, NULL, timeout)) {
    ioctl(port, PPCLRIRQ, &ignored);
    return true;
  }
  else {
    return false;
  }  
}

