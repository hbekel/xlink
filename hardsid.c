#include <unistd.h>
#include <string.h>
#include <windows.h>

#include "hardsid.h"
#include "util.h"
#include "xlink.h"

#define HARDSID_VERSION 0x0207

static BYTE sids = 0;
static WORD sid[4] = { 0xd400, 0xd400, 0xd400, 0xd400 };
static BOOL opened = false;

static  void addSid(char *address) {
  if(sids < 4) {
    if(strtol(address, NULL, 0) != 0) {
      sid[sids++] = strtol(address, NULL, 0);
    }
  }
}

static void configure() {

  logger->enter("xlink-hardsid");

  if (strtol(getenv("XLINK_HARDSID_DEBUG"), NULL, 0) != 0) {
    logger->set("DEBUG");
  }
  
  if (getenv("XLINK_HARDSIDS") != NULL) {

    char *address = strtok(getenv("XLINK_HARDSIDS"), ",");

    while(address != NULL) {
      addSid(address);
      address = strtok(NULL, ",");
    }		   		 
  }
  else{
    addSid("0xd400");
  }

  logger->debug("Number of sids: %d", sids);
  
  for(int i=0; i<sids; i++) {
    logger->debug("sid[%d] = $%04X", i, sid[i]);
  }
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved ) {
  switch(nReason) {

   case DLL_PROCESS_ATTACH:
     DisableThreadLibraryCalls(hDllHandle);
     configure();
     break;
 
   case DLL_PROCESS_DETACH:
     xlink_stream_close();
     break;
  }
  return true;
}

void DLLEXPORT InitHardSID_Mapper (void) {
  logger->debug("InitHardSID_Mapper");

  for(int i=0; i<sids; i++) {
    HardSID_Reset(i);
  }
}
BYTE DLLEXPORT GetHardSIDCount (void) { 
  logger->debug("GetHardSIDCount");
  return sids;
}

void DLLEXPORT MuteHardSID_Line (BOOL mute) {
  logger->debug("MuteHardSID_LINE(%d)", mute);
}

void DLLEXPORT WriteToHardSID (BYTE id, BYTE reg, BYTE data) { 
  logger->debug("WriteToHardSID(%d, %d, %d)", id, reg, data);
  HardSID_Write(id, 0, reg, data);
}

BYTE DLLEXPORT ReadFromHardSID (BYTE id, BYTE reg) { 
  logger->debug("ReadFromHardSID(%d, %d)", id, reg);
  return HardSID_Read(id, 0, reg);
}
void DLLEXPORT SetDebug (BOOL enable) {
  
  enable ? logger->set("DEBUG") : logger->set("INFO");
  
  logger->debug("SetDebug(%d)", enable);
}

// Version 2.00 Extensions
WORD DLLEXPORT HardSID_Version (void) { 
  logger->debug("HardSID_Version");
  return HARDSID_VERSION; 
}
BYTE DLLEXPORT HardSID_Devices (void) { 
  logger->debug("HardSID_Devices");
  return sids;
}

void DLLEXPORT HardSID_Delay (BYTE id, WORD cycles) { 
  usleep(cycles*0.985248);
  logger->debug("HardSID_Delay(%d, %d)", id, cycles);
}

void DLLEXPORT HardSID_Filter (BYTE id, BOOL filter) {
  logger->debug("HardSID_Filter(%d, %d)", id, filter);
}

void DLLEXPORT HardSID_Flush (BYTE id) { 
  logger->debug("HardSID_Flush(%d)", id);
}

void DLLEXPORT HardSID_SoftFlush (BYTE id) {
  logger->debug("HardSID_SoftFlush(%d)", id);
}

void DLLEXPORT HardSID_Mute (BYTE id, BYTE channel, BOOL mute) { 
  logger->debug("HardSID_Mute(%d, %d, %d)", id, channel, mute);
}

void DLLEXPORT HardSID_MuteAll (BYTE id, BOOL mute) {
  logger->debug("HardSID_MuteAll(%d, %d)", id, mute);
}

void DLLEXPORT HardSID_Reset (BYTE id) {
  logger->debug("HardSID_Reset(%d)", id);
  
  if (xlink_ready()) {
    opened = opened || xlink_stream_open();
    
    if (!opened) {
      logger->warn("Failed to open xlink stream");
    }
  }
  else {
    logger->warn("C64 not ready");
  }
}

BYTE DLLEXPORT HardSID_Read (BYTE id, WORD cycles, WORD reg) { 

  BYTE value = 0;

  logger->debug("HardSID_Read(%d, %d, %d)", id, cycles, reg);
  
  HardSID_Delay(id, cycles);

  if (opened) {
    xlink_stream_peek(sid[id] | reg, &value); 
  }
  return value;
}

void DLLEXPORT HardSID_Sync (BYTE id) {
  logger->debug("HardSID_Sync(%d)", id);
}

void DLLEXPORT HardSID_Write (BYTE id, WORD cycles, BYTE reg, BYTE data) { 
  logger->debug("HardSID_Write(%d, %d, %d, %d)", id, cycles, reg, data);

  HardSID_Delay(id, cycles);

  if (opened) {
    xlink_stream_poke(sid[id] | reg, data);
  }
}

void DLLEXPORT HardSID_AbortPlay(BYTE id) {
  logger->debug("HardSID_AbortPlay(%d)", id);

  opened = !xlink_stream_close();
}

// Version 2.04 Extensions
BOOL DLLEXPORT HardSID_Lock (BYTE id)  { 
  logger->debug("HardSID_Lock(%d)", id);
  return true; 
}

void DLLEXPORT HardSID_Unlock (BYTE id) {
  logger->debug("HardSID_Unlock(%d)", id);
}

void DLLEXPORT HardSID_Reset2 (BYTE id, BYTE volume) {
  logger->debug("HardSID_Reset2(%d, %d)", id, volume);
  HardSID_Reset(id);
}

// Version 2.07 Extensions
void DLLEXPORT HardSID_Mute2 (BYTE id, BYTE channel, BOOL mute, BOOL manual) {
  logger->debug("HardSID_Mute2(%d, %d, %d, %d)", id, channel, mute, manual);
}

// Rainers Un-offical extensions
WORD DLLEXPORT GetDLLVersion (void) {
  logger->debug("GetDLLVersion");
  return HARDSID_VERSION;
}
void DLLEXPORT MuteHardSID (BYTE id, BYTE channel, BOOL mute) {
  logger->debug("MuteHardSID(%d, %d, %d)", id, channel, mute);
}

void DLLEXPORT MuteHardSIDAll (BYTE id, BOOL mute) { 
  logger->debug("MuteHardSIDAll(%d, %d)", id, mute);
}
