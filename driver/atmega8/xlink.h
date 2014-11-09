#ifndef XLINK_H
#define XLINK_H

#include <stdbool.h> 
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include "../protocol.h"

#define PIN_RESET  (1 << PIND0)
#define PIN_STROBE (1 << PINC4)
#define PIN_ACK    (1 << PINC5)

void SetupHardware(void);
void SetupSTROBE(void);
void SetupACK(void);
void ReadACK(void);
void TristateRESET(void);
void AssertRESET(void);

void Init(void);
void Reset(void);
void Strobe(void);
uint8_t Acked(void);
void Input(void);
void Output(void);
uint8_t Read(void);
void Write(uint8_t byte);
void Send(uint16_t size);
void Receive(uint16_t size);
void Boot(void);

#endif // XLINK_H
