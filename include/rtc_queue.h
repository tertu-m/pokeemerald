#include "global.h"

// These return the value 0xFF on failure.
u8 AddRtcTrigger(u16 secondsFromNow, u8* script);
u8 AddRtcTriggerAtTime(u32 time, u8* script, bool8 deleteInPrehistory);
u8
void HandleRtcTriggers(void);