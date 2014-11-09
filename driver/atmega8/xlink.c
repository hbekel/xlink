#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "../protocol.h"
#include "xlink.h"

#define F_CPU 16000000
#include <util/delay.h> 

static volatile uchar result[1];
static volatile uchar last = 0;
static volatile int received, sent, expected;

void TristateRESET() {
  DDRD &= 0x00;  // tristate RESET
  PORTD |= PIN_RESET; 
}

void AssertRESET() {
  DDRD |= PIN_RESET;
  PORTD &= ~PIN_RESET;
}

void SetupACK() {
  DDRC &= ~PIN_ACK;    // ACK as input
  PORTC |= PIN_ACK;    // pullup ACK
}

void SetupSTROBE() {
  PORTC |= PIN_STROBE; // STROBE high
}

void ReadACK() {
  last = PINC & PIN_ACK; // remember last ACK state
}

void Input() {
  DDRB = 0x00;
  DDRC = 0x00 | PIN_STROBE;
}

void Output() {
  DDRB = 0x0f;
  DDRC = 0x0f | PIN_STROBE;
}

void Reset() {
  AssertRESET();
  _delay_ms(10);
  TristateRESET();
}

uint8_t Read() {
  result[0] = 0x00; 
  result[0] |= PINB & 0x0f;
  result[0] |= PINC << 4;
  return sizeof(result);
}

void Write(uint8_t data) {
  PORTB = data & 0x0f;
  PORTC = (data >> 4);
}

void Init() {
  ReadACK(); 
  Input();
}

uint8_t Acked() {
  uint8_t acked = 0;
  uint8_t current = PINC & PIN_ACK;

  if(last != current) {
    acked = 1;
    last = current;
  }
  result[0] = acked;
  return sizeof(result);
}

void Strobe() {
  PORTC &= ~PIN_STROBE;
  PORTC |= PIN_STROBE;
}

void Boot() {
  // do nothing
}

void SetupHardware() {

  TristateRESET();
  
  SetupACK();
  SetupSTROBE();
  
  ReadACK();
  
  Input();
}

USB_PUBLIC usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  usbRequest_t *request = (void*) data;
  usbMsgPtr = (usbMsgPtr_t) result;

  uchar value = request->wValue.bytes[0];

  switch(request->bRequest) {
  case USB_INIT:   Init(); return 0;
  case USB_RESET:  Reset(); return 0;
  case USB_STROBE: Strobe(); return 0;      
  case USB_ACKED:  return Acked();
  case USB_INPUT:  Input(); return 0;
  case USB_OUTPUT: Output(); return 0;
  case USB_READ:   return Read();
  case USB_WRITE:  Write(value); return 0;

  case USB_SEND:
    received = request->wLength.word;
    sent = 0;
    return USB_NO_MSG;  

  case USB_RECEIVE:
    expected = request->wLength.word;
    received = 0;
    
    return USB_NO_MSG;

  case USB_BOOT: Boot(); return 0;
  }

  return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {

  uchar i;
  uchar current = last;

  for(i = 0; sent < received && i < len; i++, sent++) {

    // write
    PORTB = data[i] & 0x0f;
    PORTC = (data[i] >> 4);
    
    // strobe
    PORTC &= ~0x10;
    PORTC |= 0x10;

    // wait
    while(current == last) {
      current = PINC & 0x20;
    }
    last = current;
  }

  return (sent == received);
}

USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len) {

  uchar i;
  uchar current = last;

  for(i = 0; received < expected && i < len; i++, received++) {

    // wait
    while(current == last) {
      current = PINC & 0x20;
    }
    last = current;

    // read
    data[i] = 0x00; 
    data[i] |= PINB & 0x0f;
    data[i] |= PINC << 4;

    // strobe
    PORTC &= ~0x10;
    PORTC |= 0x10;
  }
  return i;
}

int main(void) {  

  wdt_enable(WDTO_1S); // enable 1s watchdog timer

  usbInit();
  
  usbDeviceDisconnect(); // enforce re-enumeration
  
  for(uchar i=0; i<250; i++) { // wait 500ms
    wdt_reset(); // keep watchdog happy 
    _delay_ms(2);
  }

  usbDeviceConnect();

  SetupHardware();
  
  sei();
  while(1) {
    wdt_reset(); // keep watchdog happy 
    usbPoll();
  }

  return 0;           
}
