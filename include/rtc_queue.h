#include "global.h"

// These return the value FALSE on failure.
bool8 AddRtcTrigger(u16 secondsFromNow, const u8 *script, bool8 immediate);
bool8 AddRtcTriggerAtTime(u32 time, const u8 *script, bool8 immediate);
void Task_HandleRtcTriggers(u8 taskId);