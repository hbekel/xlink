#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#include "pp64.h"

#define pp64_send_ack _pp64_send_signal_input
#define pp64_send_strobe _pp64_send_signal_output

#define pp64_wait_ack _pp64_receive_signal
#define pp64_wait_strobe _pp64_receive_signal

#define PP64_LOAD 0x01
#define PP64_SAVE 0x02
#define PP64_POKE 0x03
#define PP64_PEEK 0x04
#define PP64_JUMP 0x05
#define PP64_RUN  0x06

#define PP64_PARPORT_CONTROL_IRQ 0x10
#define PP64_PARPORT_CONTROL_INPUT 0x20

const unsigned char pp64_ctrl_output = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT | 
                                  PP64_PARPORT_CONTROL_IRQ;

const unsigned char pp64_ctrl_input  = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT |
                                  PP64_PARPORT_CONTROL_INPUT |
                                  PP64_PARPORT_CONTROL_IRQ;

const unsigned char pp64_ctrl_reset  = PARPORT_CONTROL_SELECT | 
                                  PP64_PARPORT_CONTROL_IRQ;

const unsigned char pp64_ctrl_strobe = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT | 
                                  PARPORT_CONTROL_STROBE |
                                  PP64_PARPORT_CONTROL_IRQ; 

const unsigned char pp64_ctrl_ack    = PARPORT_CONTROL_SELECT | 
                                  PARPORT_CONTROL_INIT |
                                  PP64_PARPORT_CONTROL_INPUT | 
                                  PARPORT_CONTROL_STROBE |
                                  PP64_PARPORT_CONTROL_IRQ; 

int pp64_port;

int pp64_open() {
  
  if((pp64_port = open("/dev/parport0", O_RDWR)) == -1) {
    fprintf(stderr, "c64link: error: couldn't open /dev/parpp64_port0\n");
    return false;
  }
  
  ioctl(pp64_port, PPCLAIM);
  ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_output);  
  return true;
}

void pp64_close() {
  ioctl(pp64_port, PPRELEASE);
  close(pp64_port);
}

int pp64_load(unsigned char memory, 
	       unsigned char bank, 
	       int start, 
	       int end, 
	       char* data, int size) {

  unsigned char command = PP64_LOAD;
  
  if(pp64_open()) {

    if(!pp64_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_write(memory);
    pp64_write(bank);
    pp64_write(start & 0xff);
    pp64_write(start >> 8);
    pp64_write(end & 0xff);
    pp64_write(end >> 8);
  
    int i;
    for(i=0; i<size; i++)
      pp64_write(data[i]);

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

  unsigned char command = PP64_SAVE;
  
  if(pp64_open()) {

    if(!pp64_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_write(memory);
    pp64_write(bank);
    pp64_write(start & 0xff);
    pp64_write(start >> 8);
    pp64_write(end & 0xff);
    pp64_write(end >> 8);

    ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_input);
    pp64_send_strobe();

    int i;
    for(i=0; i<size; i++)
      data[i] = pp64_read();

    pp64_close();
    return true;
  }
  return false;
}

int pp64_peek(unsigned char memory, unsigned char bank, int address, unsigned char* value) {
  
  unsigned char command = PP64_PEEK;

  if(pp64_open()) {
  
    if(!pp64_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_write(memory);
    pp64_write(bank);
    pp64_write(address & 0xff);
    pp64_write(address >> 8);    

    ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_input);
    pp64_send_strobe();

    *value = pp64_read();

    pp64_close();

    return true;
  }
  return false;
}

int pp64_poke(unsigned char memory, unsigned char bank, int address, unsigned char value) {

  unsigned char command = PP64_POKE;

  if(pp64_open()) {
  
    if(!pp64_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      pp64_close();
      return false;
    }

    pp64_write(memory);
    pp64_write(bank);
    pp64_write(address & 0xff);
    pp64_write(address >> 8);    
    pp64_write(value);

    pp64_close();
    return true;
  }
  return false;
}

int pp64_jump(unsigned char memory, unsigned char bank, int address) {

  unsigned char command = PP64_JUMP;

  if(pp64_open()) {
  
    if(!pp64_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      pp64_close();
      return false;
    }
    pp64_write(memory);
    pp64_write(bank);
    pp64_write(address >> 8);    // target address is send MSB first (big-endian)
    pp64_write(address & 0xff);
    
    pp64_close();
    return true;
  }
  return false;
}

int pp64_run(void) {

  unsigned char command = PP64_RUN;

  if(pp64_open()) {     
    if(!pp64_write_with_timeout(command)) {
      fprintf(stderr, "c64link: error: no response from C64\n");
      return false;
    }
    pp64_close();
  }
  return true;
}

int pp64_reset(void) {
  if(pp64_open()) {
    ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_reset);
    usleep(100*1000);
    ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_output);
    pp64_close();
    return true;
  }
  return false;
}

inline unsigned char pp64_read(void) {
  char byte;
  pp64_wait_strobe(NULL);
  ioctl(pp64_port, PPRDATA, &byte);
  pp64_send_ack();
  return byte;
} 

inline void pp64_write(unsigned char byte) {
  ioctl(pp64_port, PPWDATA, &byte);
  pp64_send_strobe();
  pp64_wait_ack(NULL);  
}

inline int pp64_write_with_timeout(unsigned char byte) {

  struct timeval tv = { 0, 150*1000 };

  ioctl(pp64_port, PPWDATA, &byte);
  pp64_send_strobe();
  return pp64_wait_ack(&tv);  
}

inline void _pp64_send_signal_input(void) {
  ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_ack);
  ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_input);  
}

inline void _pp64_send_signal_output(void) {
  ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_strobe);
  ioctl(pp64_port, PPWCONTROL, &pp64_ctrl_output);
}

inline int _pp64_receive_signal(struct timeval* timeout) {  
  
  fd_set rfds;
  int ignored = 0;

  FD_ZERO(&rfds);
  FD_SET(pp64_port, &rfds);

  if(select(pp64_port + 1, &rfds, NULL, NULL, timeout)) {
    ioctl(pp64_port, PPCLRIRQ, &ignored);
    return true;
  }
  else {
    return false;
  }  
}

