#ifndef XLINK_H
#define XLINK_H

#define GCC_MEMORY_BARRIER()             __asm__ __volatile__("" ::: "memory");
#define ATTR_NO_INIT                     __attribute__ ((section (".noinit")))
#define ATTR_INIT_SECTION(SectionIndex)  __attribute__ ((used, naked, section (".init" #SectionIndex )))

#define PIN_RESET  (1 << PINC4)
#define PIN_STROBE (1 << PIND4)
#define PIN_ACK    (1 << PIND3)

#define MAGIC_BOOT_KEY            0xFEEDBABE
#define BOOTLOADER_START_ADDRESS  (0x8000 - 0x800)


int main(void);

void SetupHardware(void);
void SetupSerial(void);
void SetupTimer(void);
void ResetTimer(void);
void SetupSTROBE(void);
void SetupACK(void);
void ReadACK(void);
void TristateRESET(void);
void AssertRESET(void);

uint8_t ReadSerial(void);
void WriteSerial(uint8_t data);

void Input(void);
void Output(void);

void Strobe(void);
void Acked(void);

uint8_t Read(void);
void Write(uint8_t);

void Send(uint16_t bytesToSend, uint16_t timeout);
void Receive(uint16_t byteToReceive, uint16_t timeout);

void Reset(void);

void BootCheck(void) ATTR_INIT_SECTION(3);
void BootCheck(void);
void Boot(void);


static inline void GlobalInterruptEnable(void) {
  GCC_MEMORY_BARRIER();
  sei();
  GCC_MEMORY_BARRIER();
}

static inline void GlobalInterruptDisable(void) {
  GCC_MEMORY_BARRIER(); 
  cli();
  GCC_MEMORY_BARRIER();
}

#endif // XLINK_H
