#include <unistd.h>

#include "hardsid.h"
#include "util.h"
#include "windows.h"
#include "xlink.h"

static BOOL xlink_stream_opened = false;

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved ) {
  switch(nReason) {

   case DLL_PROCESS_ATTACH:
     DisableThreadLibraryCalls(hDllHandle);
     break;
 
   case DLL_PROCESS_DETACH:
     break;
  }
  return true;
}

void DLLEXPORT InitHardSID_Mapper (void) {
  logger->info("InitHardSID_Mapper");
}
BYTE DLLEXPORT GetHardSIDCount (void) { 
  logger->info("GetHardSIDCount");
  return 1; 
}

void DLLEXPORT MuteHardSID_Line (BOOL mute) {
  logger->info("MuteHardSID_LINE(%d)", mute);
}

void DLLEXPORT WriteToHardSID (BYTE id, BYTE reg, BYTE data) { 
  logger->info("WriteToHardSID(%d, %d, %d)", id, reg, data);
}

BYTE DLLEXPORT ReadFromHardSID (BYTE id, BYTE reg) { 
  logger->info("ReadFromHardSID(%d, %d)", id, reg);
  return 0; 
}
void DLLEXPORT SetDebug (BOOL enable) {
  logger->info("SetDebug(%d)", enable);
}

// Version 2.00 Extensions
WORD DLLEXPORT HardSID_Version (void) { 
  logger->info("HardSID_Version");
  return 0x0200; 
}
BYTE DLLEXPORT HardSID_Devices (void) { 
  logger->info("HardSID_Devices");
  return 1; 
}

void DLLEXPORT HardSID_Delay (BYTE id, WORD cycles) { 
  usleep(cycles);
  logger->info("HardSID_Delay(%d, %d)", id, cycles);
}

void DLLEXPORT HardSID_Filter (BYTE id, BOOL filter) {
  logger->info("HardSID_Filter(%d, %d)", id, filter);
}

void DLLEXPORT HardSID_Flush (BYTE id) { 
  logger->info("HardSID_Flush(%d)", id);
}

void DLLEXPORT HardSID_SoftFlush (BYTE id) {
  logger->info("HardSID_SoftFlush(%d)", id);
}

void DLLEXPORT HardSID_Mute (BYTE id, BYTE channel, BOOL mute) { 
  logger->info("HardSID_Mute(%d, %d, %d)", id, channel, mute);
}

void DLLEXPORT HardSID_MuteAll (BYTE id, BOOL mute) {
  logger->info("HardSID_MuteAll(%d, %d)", id, mute);
}

void DLLEXPORT HardSID_Reset (BYTE id) {
  logger->info("HardSID_Reset(%d)", id);
  xlink_stream_opened = xlink_stream_opened || xlink_stream_open();
}

BYTE DLLEXPORT HardSID_Read (BYTE id, WORD cycles, WORD reg) { 

  BYTE value = 0;

  logger->info("HardSID_Read(%d, %d, %d)", id, cycles, reg);
  
  HardSID_Delay(id, cycles);

  if (xlink_stream_opened)
    xlink_stream_peek(0xd400 | reg, &value); 
  
  return value;
}

void DLLEXPORT HardSID_Sync (BYTE id) {
  logger->info("HardSID_Sync(%d)", id);
}

void DLLEXPORT HardSID_Write (BYTE id, WORD cycles, BYTE reg, BYTE data) { 
  logger->info("HardSID_Write(%d, %d, %d, %d)", id, cycles, reg, data);

  HardSID_Delay(id, cycles);

  if (xlink_stream_opened)
    xlink_stream_poke(0xd400 | reg, data);
}

void DLLEXPORT HardSID_AbortPlay(BYTE id) {
  logger->info("HardSID_AbortPlay(%d)", id);
}

// Version 2.04 Extensions
BOOL DLLEXPORT HardSID_Lock (BYTE id)  { 
  logger->info("HardSID_Lock(%d)", id);
  return true; 
}

void DLLEXPORT HardSID_Unlock (BYTE id) {
  logger->info("HardSID_Unlock(%d)", id);
}

void DLLEXPORT HardSID_Reset2 (BYTE id, BYTE volume) {
  logger->info("HardSID_Reset2(%d, %d)", id, volume);
}

// Version 2.07 Extensions
void DLLEXPORT HardSID_Mute2 (BYTE id, BYTE channel, BOOL mute, BOOL manual) {
  logger->info("HardSID_Mute2(%d, %d, %d, %d)", id, channel, mute, manual);
}

// Rainers Un-offical extensions
WORD DLLEXPORT GetDLLVersion (void) {
  logger->info("GetDLLVersion");
  return 0x0200;
}
void DLLEXPORT MuteHardSID (BYTE id, BYTE channel, BOOL mute) {
  logger->info("MuteHardSID(%d, %d, %d)", id, channel, mute);
}

void DLLEXPORT MuteHardSIDAll (BYTE id, BOOL mute) { 
  logger->info("MuteHardSIDAll(%d, %d)", id, mute);
}
