#include <unistd.h>
#include <string.h>
#include <windows.h>

#include "hardsid.h"
#include "util.h"
#include "xlink.h"

#define HARDSID_VERSION 0x0207
#define XLINK_POKE_DELAY 112

static BYTE sids = 0;
static WORD sid[4] = { 0xd400, 0xd400, 0xd400, 0xd400 };
static BYTE regs[28];
static BOOL opened = false;

//static WORD penalty = 0;
  
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

  for(int i=0; i<28; i++) {
    regs[i] = 0;
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

BYTE DLLEXPORT GetHardSIDCount (void) { 
  return sids;
}

void DLLEXPORT MuteHardSID_Line (BOOL mute) { }

void DLLEXPORT WriteToHardSID (BYTE id, BYTE reg, BYTE data) { 
  if (opened) {
    if(regs[reg] != data) {

      xlink_stream_poke(sid[id] | reg, data);
      regs[reg] = data;
    }
  }
}

BYTE DLLEXPORT ReadFromHardSID (BYTE id, BYTE reg) { 
  return 0;
}
void DLLEXPORT SetDebug (BOOL enable) {
  
  enable ? logger->set("DEBUG") : logger->set("INFO");
  
  logger->debug("SetDebug(%d)", enable);
}

/*

// Version 2.00 Extensions
WORD DLLEXPORT HardSID_Version (void) { 
  return HARDSID_VERSION; 
}

BYTE DLLEXPORT HardSID_Devices (void) { 
  return sids;
}

void DLLEXPORT HardSID_Delay (BYTE id, WORD cycles) { 
  
  if(cycles > 13) {
    if(cycles > penalty) {
      cycles -= penalty;
    }
    penalty = 0;
  }
  usleep(cycles);
}

void DLLEXPORT HardSID_Filter (BYTE id, BOOL filter) { }

void DLLEXPORT HardSID_Flush (BYTE id) {  }

void DLLEXPORT HardSID_SoftFlush (BYTE id) { }

void DLLEXPORT HardSID_Mute (BYTE id, BYTE channel, BOOL mute) { }

void DLLEXPORT HardSID_MuteAll (BYTE id, BOOL mute) { }

void DLLEXPORT HardSID_Reset (BYTE id) {
  
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

  // BYTE value = 0;
  //
  // if (opened) {
  //   xlink_stream_peek(sid[id] | reg, &value); 
  // }
  // return value;

  return 0;
}

void DLLEXPORT HardSID_Sync (BYTE id) { }

void DLLEXPORT HardSID_Write (BYTE id, WORD cycles, BYTE reg, BYTE data) { 

  if (opened) {
    if(regs[reg] != data) {

      xlink_stream_poke(sid[id] | reg, data);
      regs[reg] = data;

      penalty += XLINK_POKE_DELAY;
    }
  }
}

void DLLEXPORT HardSID_AbortPlay(BYTE id) {
  opened = !xlink_stream_close();
}

// Version 2.04 Extensions
BOOL DLLEXPORT HardSID_Lock (BYTE id)  { 
  return true; 
}

void DLLEXPORT HardSID_Unlock (BYTE id) { }

void DLLEXPORT HardSID_Reset2 (BYTE id, BYTE volume) {
  HardSID_Reset(id);
}

// Version 2.07 Extensions
void DLLEXPORT HardSID_Mute2 (BYTE id, BYTE channel, BOOL mute, BOOL manual) { }

// Rainers Un-offical extensions
WORD DLLEXPORT GetDLLVersion (void) {
  return HARDSID_VERSION;
}
void DLLEXPORT MuteHardSID (BYTE id, BYTE channel, BOOL mute) { }

void DLLEXPORT MuteHardSIDAll (BYTE id, BOOL mute) { }
*/
