#include "uuadapter.h"

static volatile uint8_t last;
static uint32_t Boot_Key ATTR_NO_INIT;

int main(void)
{    
  USB_Init();

  SetupHardware();

  GlobalInterruptEnable();
  
  for(;;) {
    USB_USBTask();
    wdt_reset();
  }
}

void SetupHardware() {

  wdt_enable(WDTO_1S);
  clock_prescale_set(clock_div_1);

  TristateRESET();

  SetupSTROBE();

  SetupACK();
  ReadACK();

  Input();
}

void EVENT_USB_Device_ControlRequest(void) {

  if (((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR) &&
      ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT) == REQREC_DEVICE)) {

    uint8_t byte = (uint8_t) (USB_ControlRequest.wValue & 0xff);
    uint16_t size = USB_ControlRequest.wLength;    

    switch(USB_ControlRequest.bRequest) {

    case USB_INIT:    Init();        break;
    case USB_RESET:   Reset();       break;
    case USB_STROBE:  Strobe();      break;
    case USB_ACKED:   Acked();       break;
    case USB_INPUT:   Input();       break;
    case USB_OUTPUT:  Output();      break;
    case USB_READ:    Read();        break;
    case USB_WRITE:   Write(byte);   break;
    case USB_SEND:    Send(size);    break;
    case USB_RECEIVE: Receive(size); break;
    case USB_BOOT:    Boot();       break;
    }
  }
}

void SetupSTROBE() {
  DDRC |= PIN_STROBE;  // STROBE as output
  PORTC |= PIN_STROBE; // set STROBE high
}

void SetupACK() {
  DDRB &= ~PIN_ACK; // ACK as input
  PORTB |= PIN_ACK; // with pullup
}

void ReadACK() { 
  last = PINB & PIN_ACK; // remember last ack value
}

void TristateRESET() {
  DDRC &= ~PIN_RESET; // RESET as input
  PORTC |= PIN_RESET; // with pullup
}

void AssertRESET() {
  DDRC |= PIN_RESET;   // RESET as output
  PORTC &= ~PIN_RESET; // pull reset low
}

void Init() {
  ReadACK(); 
  Input();
}

void Reset() {
  Endpoint_ClearSETUP();

  AssertRESET();
  Delay_MS(10);   
  TristateRESET();

  Endpoint_ClearOUT();
  Endpoint_ClearStatusStage();
}

void Strobe() { 
  Endpoint_ClearSETUP();

  PORTC &= ~PIN_STROBE;
  PORTC |= PIN_STROBE;

  Endpoint_ClearOUT();
  Endpoint_ClearStatusStage();
}

void Acked() {

  Endpoint_ClearSETUP();

  uint8_t acked = 0;
  uint8_t current = PINB & PIN_ACK;

  if(last != current) {
    acked = 1;
    last = current;
  }

  while(!Endpoint_IsINReady());
  Endpoint_Write_8(acked);
  Endpoint_ClearIN();

  while(!Endpoint_IsOUTReceived());
  Endpoint_ClearOUT();
}

void Input() {
  Endpoint_ClearSETUP();

  DDRD  = 0x00; // PORTD as input
  PORTD = 0xff; // with pullups

  Endpoint_ClearOUT();
  Endpoint_ClearStatusStage();
}

void Output() {
  Endpoint_ClearSETUP();

  DDRD = 0xff; // PORTD as output

  Endpoint_ClearOUT();
  Endpoint_ClearStatusStage();
}

void Write(uint8_t byte) {
  Endpoint_ClearSETUP();

  PORTD = byte;

  Endpoint_ClearOUT();
  Endpoint_ClearStatusStage();
}

void Read() {
  Endpoint_ClearSETUP();

  Endpoint_Write_8(PIND);

  Endpoint_ClearIN();
  Endpoint_ClearStatusStage();
}

void Send(uint16_t bytesToSend) {

 uint8_t i;
 uint8_t bytesInPacket;
 uint8_t current = last;

 Endpoint_ClearSETUP(); // ACK SETUP packet
 
 while(bytesToSend) {
   
   while(!Endpoint_IsOUTReceived()); // Wait for data packet
   
   bytesInPacket = Endpoint_BytesInEndpoint();

   // Process Data Packet;
   for(i=0; i<bytesInPacket; i++) {
     
     PORTD = Endpoint_Read_8();

     PORTC &= ~PIN_STROBE;
     PORTC |= PIN_STROBE;
     
     while(current == last) {
       current = PINB & PIN_ACK;
     }
     last = current;
   }
   bytesToSend -= bytesInPacket;

   Endpoint_ClearOUT(); // ACK data packet
 }

 // Now ack the whole control transfer...
 while(!Endpoint_IsINReady()); // wait for the host to be ready to receive ACK
 Endpoint_ClearIN(); // ack by sending an empty package TO the HOST
}

void Receive(uint16_t bytesToReceive) {

 uint8_t i;
 uint8_t bytesInPacket;
 uint8_t current = last;
 
 Endpoint_ClearSETUP(); // ACK SETUP packet

 while(bytesToReceive) {

   while(!Endpoint_IsINReady()); // Wait for host ready

   bytesInPacket = (bytesToReceive >= FIXED_CONTROL_ENDPOINT_SIZE) ?
     FIXED_CONTROL_ENDPOINT_SIZE : bytesToReceive;

   // Write data...
   for(i=0; i<bytesInPacket; i++) {

     while(current == last) {
       current = PINB & PIN_ACK;
     }
     last = current;
     
     Endpoint_Write_8(PIND);
     
     PORTC &= ~PIN_STROBE;
     PORTC |= PIN_STROBE;     
   }
   bytesToReceive -= bytesInPacket;
   
   Endpoint_ClearIN(); // send data packet
 }

 // Now ack the whole control transfer...
 while(!Endpoint_IsOUTReceived()); // wait for host to send STATUS ACK
 Endpoint_ClearOUT(); // clear host STATUS ACK
}

void BootCheck(void) {
  // If the reset source was the bootloader and the key is correct, clear it and jump to the bootloader
  if ((MCUSR & (1 << WDRF)) && (Boot_Key == MAGIC_BOOT_KEY)) {
    Boot_Key = 0;
    ((void (*)(void))BOOTLOADER_START_ADDRESS)();
  }
}
  
void Boot(void) {

  Endpoint_ClearSETUP();
  Endpoint_ClearOUT();
  Endpoint_ClearStatusStage();

  // detach from the bus and reset
  USB_Disable();
  
  // Disable all interrupts
  cli();
  
  // Wait two seconds for the USB detachment to register on the host
  Delay_MS(2000);

  // Set the bootloader key to the magic value and force a reset
  Boot_Key = MAGIC_BOOT_KEY;
  wdt_enable(WDTO_250MS);
  
  for(;;);
}

