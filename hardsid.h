#ifndef HARDSID_H
#define HARDSID_H

#include "windows.h"
#define DLLEXPORT WINAPI DECLSPEC_EXPORT

#ifdef __cplusplus
extern "C" {
#endif

  // Legacy Interface
  void DLLEXPORT InitHardSID_Mapper (void);
  BYTE DLLEXPORT GetHardSIDCount    (void);
  void DLLEXPORT MuteHardSID_Line   (BOOL mute);
  void DLLEXPORT WriteToHardSID     (BYTE id, BYTE reg, BYTE data);
  BYTE DLLEXPORT ReadFromHardSID    (BYTE id, BYTE reg);
  void DLLEXPORT SetDebug           (BOOL enable);
  
  /*
  // Version 2.00 Extensions
  WORD DLLEXPORT HardSID_Version   (void);
  BYTE DLLEXPORT HardSID_Devices   (void);
  void DLLEXPORT HardSID_Delay     (BYTE id, WORD cycles);
  void DLLEXPORT HardSID_Filter    (BYTE id, BOOL filter);
  void DLLEXPORT HardSID_Flush     (BYTE id);
  void DLLEXPORT HardSID_SoftFlush (BYTE id);
  void DLLEXPORT HardSID_Mute      (BYTE id, BYTE channel, BOOL mute);
  void DLLEXPORT HardSID_MuteAll   (BYTE id, BOOL mute);
  void DLLEXPORT HardSID_Reset     (BYTE id);
  BYTE DLLEXPORT HardSID_Read      (BYTE id, WORD cycles, WORD reg);
  void DLLEXPORT HardSID_Sync      (BYTE id);
  void DLLEXPORT HardSID_Write     (BYTE id, WORD cycles, BYTE reg, BYTE data);
  void DLLEXPORT HardSID_AbortPlay (BYTE id);
  
  // Version 2.04 Extensions
  BOOL DLLEXPORT HardSID_Lock   (BYTE id);
  void DLLEXPORT HardSID_Unlock (BYTE id);
  void DLLEXPORT HardSID_Reset2 (BYTE id, BYTE volume);

  // Version 2.07 Extensions
  void DLLEXPORT HardSID_Mute2 (BYTE id, BYTE channel, BOOL mute, BOOL manual);

  // Rainers Un-offical extensions
  WORD DLLEXPORT GetDLLVersion (void);
  void DLLEXPORT MuteHardSID (BYTE id, BYTE channel, BOOL mute);
  void DLLEXPORT MuteHardSIDAll (BYTE id, BOOL mute);

  */

#ifdef __cplusplus
}
#endif

#endif //HARDSID_H
