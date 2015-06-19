#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif

#include <util/setbaud.h>

#include "xlink.h"
#include "../protocol.h"

//------------------------------------------------------------------------------

/* Pin Mapping

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
static volatile uint32_t hs = 0;
static uint32_t Boot_Key ATTR_NO_INIT;

//------------------------------------------------------------------------------

int main(void) {    

  uint8_t  cmd     = 0;
  uint8_t  byte    = 0;
  uint16_t size    = 0;
  uint16_t timeout = 0;
  
  SetupHardware();
  
  while(1) {
    cmd = byte = size = timeout = 0;
    
    cmd     |= ReadSerial();
    size    |= (byte |= ReadSerial());
    size    |= (ReadSerial() << 8);
    timeout |= ReadSerial();
    timeout |= (ReadSerial() << 8);

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
    case CMD_BOOT:    Boot();                 break;
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
  
  // start with /64 prescaler =~ 0.5secs
  TCCR1B |= (1 << CS10) | (1 << CS11);

  ResetTimer();
}

//------------------------------------------------------------------------------

void ResetTimer() {
  GlobalInterruptDisable();

  TCNT1=0x00;
  hs=0;
  elapsed=0;

  GlobalInterruptEnable();
}

//------------------------------------------------------------------------------

ISR(TIMER1_OVF_vect) {
  hs++;
  elapsed = hs/2;
}

//------------------------------------------------------------------------------

void SetupSerial(void) {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    
#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */ 
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */    
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
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

//------------------------------------------------------------------------------

void WriteSerial(uint8_t c) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
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

  uint8_t acked = 0;
  uint8_t current = PIND & PIN_ACK;

  if(last != current) {
    acked = 1;
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

void Send(uint16_t bytesToSend, uint16_t timeout) {

 uint8_t i;
 uint8_t current = last;

 for(i=0; i<bytesToSend; i++) {
     
   Write(ReadSerial());

   PORTD &= ~PIN_STROBE;
   PORTD |= PIN_STROBE;

   ResetTimer();
     
   while(current == last) {
     current = PIND & PIN_ACK;
     wdt_reset();
       
     if(timeout > 0 && elapsed >= timeout) {
       return;
     }
   }
   last = current;
 }
}

//------------------------------------------------------------------------------

void Receive(uint16_t bytesToReceive, uint16_t timeout) {
 uint8_t i;
 uint8_t current = last;

 for(i=0; i<bytesToReceive; i++) {

   ResetTimer();
   
   while(current == last) {
     current = PIND & PIN_ACK;
     wdt_reset();
       
     if(timeout > 0 && elapsed >= timeout) {
       return;
     }
   }
   last = current;

   WriteSerial(Read());

   PORTD &= ~PIN_STROBE;
   PORTD |= PIN_STROBE;   
 }
}

//------------------------------------------------------------------------------

void Reset() {
  AssertRESET();
  _delay_ms(10);   
  TristateRESET();
}

//------------------------------------------------------------------------------

void BootCheck(void) {

  if ((MCUSR & (1 << WDRF)) && (Boot_Key == MAGIC_BOOT_KEY)) {
    Boot_Key = 0;
    ((void (*)(void))BOOTLOADER_START_ADDRESS)();
  }
}

//------------------------------------------------------------------------------

void Boot(void) {

  GlobalInterruptDisable();
  Boot_Key = MAGIC_BOOT_KEY;
  wdt_enable(WDTO_250MS);  
  for(;;);
}

//------------------------------------------------------------------------------
