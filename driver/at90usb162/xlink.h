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

#include "../protocol.h"

#define PIN_RESET  (1 << PINC5)
#define PIN_STROBE (1 << PINC2)
#define PIN_ACK    (1 << PINB2)

#define MAGIC_BOOT_KEY            0xFEEDBABE
#define BOOTLOADER_START_ADDRESS  0x3000

void SetupHardware(void);
void SetupTimer(void);
void ResetTimer(void);
void SetupSTROBE(void);
void SetupACK(void);
void ReadACK(void);
void TristateRESET(void);
void AssertRESET(void);

void Reset(void);
void Strobe(void);
void Acked(void);
void Input(void);
void Output(void);
void Read(void);
void Write(uint8_t byte);
void Send(uint16_t size, uint16_t timeout);
void Receive(uint16_t size, uint16_t timeout);

void BootCheck(void) ATTR_INIT_SECTION(3);
void BootCheck(void);
void Boot(void);

void EVENT_USB_Device_ControlRequest(void);

#endif // XLINK_H
