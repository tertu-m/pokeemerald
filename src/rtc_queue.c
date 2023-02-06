#include "rtc_queue.h"
static EWRAM_DATA struct RtcTriggerEntry interactiveRtcTriggers[32];
static EWRAM_DATA struct RtcTriggerEntry immediateRtcTriggers[16];
static u32 lastUpdateTime;

struct RtcTriggerEntry {
    u32 createTime;
    u32 triggerTime;
    u8* triggerScript;
    u8* backdateScript;
};

union UnsignedSigned {
    i32 _signed;
    u32 _unsigned;
}

// this seems
static u32 unsigned_abs(u32 x)
{
    i32 mask;
    union UnsignedSigned pun;
    pun._unsigned = x;
    mask = pun._signed >> 31;

}

static u32 GetRtcChange(u32 new, u32 old, bool8 *direction)
{

}

static void HandleBackdate()