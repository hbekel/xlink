#ifndef XLINK_H
#define XLINK_H

#include <stdbool.h> 
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <LUFA/Common/Common.h>
#include <LUFA/Drivers/USB/USB.h>

#include "descriptors.h"

#define USB_INIT     0x00
#define USB_RESET    0x01
#define USB_STROBE   0x02
#define USB_ACKED    0x03 
#define USB_INPUT    0x04
#define USB_OUTPUT   0x05
#define USB_READ     0x06
#define USB_WRITE    0x07
#define USB_SEND     0x08
#define USB_RECEIVE  0x09
#define USB_BOOT    0x0a

#define PIN_RESET  (1 << PINC5)
#define PIN_STROBE (1 << PINC2)
#define PIN_ACK    (1 << PINB2)

#define MAGIC_BOOT_KEY            0xFEEDBABE
#define BOOTLOADER_START_ADDRESS  0x3000

void SetupHardware(void);
void SetupSTROBE(void);
void SetupACK(void);
void ReadACK(void);
void TristateRESET(void);
void AssertRESET(void);

void Init(void);
void Reset(void);
void Strobe(void);
void Acked(void);
void Input(void);
void Output(void);
void Read(void);
void Write(uint8_t byte);
void Send(uint16_t size);
void Receive(uint16_t size);

void BootCheck(void) ATTR_INIT_SECTION(3);
void BootCheck(void);
void Boot(void);

void EVENT_USB_Device_ControlRequest(void);

#endif // XLINK_H
