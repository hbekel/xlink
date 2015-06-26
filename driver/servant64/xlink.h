#ifndef XLINK_H
#define XLINK_H

#define PIN_RESET  (1 << PINC4)
#define PIN_STROBE (1 << PIND4)
#define PIN_ACK    (1 << PIND3)

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

void Send(uint32_t bytesToSend, uint32_t timeout);
void Receive(uint32_t byteToReceive, uint32_t timeout);

void Reset(void);

#define GCC_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory");

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
