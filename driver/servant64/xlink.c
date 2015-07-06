#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 500000UL
#endif

#include "uart.h"
#include "xlink.h"
#include "../protocol.h"

//------------------------------------------------------------------------------
/*

CBM  |  Atmega
===========
PB0  ->  PD5
PB1  ->  PD6
PB2  ->  PD7

PB3  ->  PB0
PB4  ->  PB1

PB5  ->  PC0
PB6  ->  PC1
PB7  ->  PC2

FLAG ->  PD4  
PA2  ->  PD3
RES  ->  PC4

*/
//------------------------------------------------------------------------------

static volatile uint8_t last;
static volatile uint16_t elapsed = 0;
static volatile uint32_t qs = 0;

//------------------------------------------------------------------------------

int main(void) {    

  uint8_t  cmd     = 0;
  uint8_t  byte    = 0;
  uint32_t size    = 0UL;
  uint32_t timeout = 0UL;
  
  SetupHardware();
  
  while(1) {
    cmd = byte = size = timeout = 0;
    
    cmd     |= ReadSerial();
    byte    |= ReadSerial();
    size    |= (uint32_t) byte;
    size    |= ((uint32_t)(ReadSerial()) << 8UL);
    size    |= (((uint32_t)ReadSerial()) << 16UL);
    size    |= (((uint32_t)ReadSerial()) << 24UL);    
    timeout |= (uint32_t) ReadSerial();
    timeout |= (((uint32_t)ReadSerial()) << 8UL);
    timeout |= (((uint32_t)ReadSerial()) << 16UL);
    timeout |= (((uint32_t)ReadSerial()) << 24UL);    

    switch(cmd) {

    case CMD_RESET:   Reset();                break;
    case CMD_STROBE:  Strobe();               break;
    case CMD_ACKED:   Acked();                break;
    case CMD_INPUT:   Input();                break;
    case CMD_OUTPUT:  Output();               break;
    case CMD_READ:    WriteSerial(Read());    break;
    case CMD_WRITE:   Write(byte);            break;
    case CMD_SEND:    Send(size, timeout);    break;
    case CMD_RECEIVE: Receive(size, timeout); break;
    default: break;
    }
    wdt_reset();
  }
  
  return 0;
}

//------------------------------------------------------------------------------

void SetupHardware() {

  wdt_enable(WDTO_1S);
  clock_prescale_set(clock_div_1);

  SetupTimer();

  TristateRESET();
  
  SetupSerial();

  SetupSTROBE();

  SetupACK();
  ReadACK();

  Input();
}

//------------------------------------------------------------------------------

void SetupTimer() {

  GlobalInterruptDisable();
  
  // enable timer overflow for TIMER1
  TIMSK1 = (1<<TOIE1);

  // set initial value to 0
  TCNT1 = 0x00;
  
  // start with /64 prescaler =~ 0.25secs
  TCCR1B |= (1 << CS10) | (1 << CS11);

  ResetTimer();
}

//------------------------------------------------------------------------------

void ResetTimer() {
  GlobalInterruptDisable();

  TCNT1=0x00;
  qs=0;
  elapsed=0;

  GlobalInterruptEnable();
}

//------------------------------------------------------------------------------

ISR(TIMER1_OVF_vect) {
  qs++;
  elapsed = qs/4;
}

//------------------------------------------------------------------------------

void SetupSerial(void) {
  uart_init(UART_BAUD_SELECT(BAUD, F_CPU));
}

//------------------------------------------------------------------------------

void SetupSTROBE() {
  DDRD  |= PIN_STROBE;  // STROBE as output
  PORTD |= PIN_STROBE;  // set STROBE high
}

//------------------------------------------------------------------------------

void SetupACK() {
  DDRD  &= ~PIN_ACK; // ACK as input
  PORTD |= PIN_ACK;  // with pullup
}

//------------------------------------------------------------------------------

void ReadACK() { 
  last = PIND & PIN_ACK; // remember last ack value
}

//------------------------------------------------------------------------------

void TristateRESET() {
  DDRC &= ~PIN_RESET; // RESET as input
  PORTC |= PIN_RESET; // with pullup
}

//------------------------------------------------------------------------------

void AssertRESET() {
  DDRC |= PIN_RESET;   // RESET as output
  PORTC &= ~PIN_RESET; // pull reset low
}

//------------------------------------------------------------------------------

uint8_t ReadSerial(void) {
  while(!uart_available()) { wdt_reset(); }
  return uart_getc() & 0xff;
}

//------------------------------------------------------------------------------

void WriteSerial(uint8_t c) {
  uart_putc(c);
}

//------------------------------------------------------------------------------

void Input(void) {
  DDRD  &= 0b00011111;
  PORTD |= 0b11100000;
  
  DDRB  &= 0b11111100;
  PORTB |= 0b00000011;
  
  DDRC  &= 0b11111000;
  PORTC |= 0b00000111;
}

//------------------------------------------------------------------------------

void Output(void) {
  DDRD |= 0b11100000;
  DDRB |= 0b00000011;
  DDRC |= 0b00000111;
}

//------------------------------------------------------------------------------

void Strobe() { 
  PORTD &= ~PIN_STROBE;
  PORTD |= PIN_STROBE;
}

//------------------------------------------------------------------------------

void Acked() {

  uint8_t acked = 0xaa;
  uint8_t current = PIND & PIN_ACK;

  if(last != current) {
    acked = 0x55;
    last = current;
  }
  WriteSerial(acked);
}

//------------------------------------------------------------------------------

uint8_t Read(void) {
  uint8_t byte = 0;

  byte |= (PIND >> 5);
  byte |= ((PINB & 0b00000011) << 3);
  byte |= (PINC << 5);

  return byte;
}

//------------------------------------------------------------------------------

void Write(uint8_t byte) {
  PORTD &= 0b00011111;
  PORTD |= ((byte & 0b00000111) << 5);

  PORTB &= 0b11111100;
  PORTB |= ((byte & 0b00011000) >> 3);

  PORTC &= 0b11111000;
  PORTC |= ((byte & 0b11100000) >> 5);
}

//------------------------------------------------------------------------------

void Send(uint32_t bytesToSend, uint32_t timeout) {

  uint32_t bytesSent = bytesToSend;
  uint8_t current = last;

  uint32_t i;
  
  for(i=0; i<bytesToSend; i++) {
    
    Write(ReadSerial());

    PORTD &= ~PIN_STROBE;
    PORTD |= PIN_STROBE;
    
    ResetTimer();
    
    while(current == last) {
      current = PIND & PIN_ACK;
      wdt_reset();
      
      if(timeout > 0 && elapsed >= timeout) {
        bytesSent = i+1;
        uart_flush();
        goto done;
      }
    }
    last = current;
  }

 done:
  WriteSerial(bytesSent);
  WriteSerial(bytesSent >> 8);
  WriteSerial(bytesSent >> 16);
  WriteSerial(bytesSent >> 24);
}

//------------------------------------------------------------------------------

void Receive(uint32_t bytesToReceive, uint32_t timeout) {

  uint32_t bytesReceived = bytesToReceive;
  uint8_t current = last;

  uint32_t i;  
  for(i=0; i<bytesToReceive; i++) {

    ResetTimer();
    
    while(current == last) {
      current = PIND & PIN_ACK;
      wdt_reset();
      
      if(timeout > 0 && elapsed >= timeout) {
        bytesReceived = i+1;
        for(;i<bytesToReceive; i++) {
          WriteSerial(0xff);
          wdt_reset();
        }
        goto done;
      }
    }
    last = current;
    
    WriteSerial(Read());
    
    PORTD &= ~PIN_STROBE;
    PORTD |= PIN_STROBE;
  }
  
 done:
  WriteSerial(bytesReceived);
  WriteSerial(bytesReceived >> 8);
  WriteSerial(bytesReceived >> 16);
  WriteSerial(bytesReceived >> 24);
}

//------------------------------------------------------------------------------

void Reset() {
  AssertRESET();
  _delay_ms(10);   
  TristateRESET();
}

//------------------------------------------------------------------------------
