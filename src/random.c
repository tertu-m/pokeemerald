#include "global.h"
#include "random.h"
#include "main.h"
#include "rtc.h"

struct RngState gRngState;
struct RngState gRng2State;
volatile enum RngStatus gRngStatus;

#define RANDOM_IMPL_NONCONST
#define RANDOM_IMPL_CONST __attribute__((const))
#include "_random_impl.h"

void BurnRandom(void) {
    if (gRngStatus == IDLE)
        Random32();
}

const u16 clz_Lookup[] = {31, 22, 30, 21, 18, 10, 29, 2, 20, 17, 15, 13, 9,
    6, 28, 1, 23, 19, 11, 3, 16, 14, 7, 24, 12, 4, 8, 25, 5, 26, 27, 0};

static inline void SeedRngState(struct RngState *state, u32 seed_c, u32 seed_b, u32 seed_a)
{
    u32 i;

    state->c = seed_c;
    state->b = seed_b;
    state->a = seed_a;
    state->counter = 1;

    for (i = 0; i < 20; i++)
        _GetRng32(state);
}

u32 GetSeedSecondaryData()
{
    u32 seconds;
    struct SiiRtcInfo rtc;

    // If the RTC isn't working properly, use a fallback.
    if (RtcGetErrorStatus() & RTC_ERR_FLAG_MASK)
        return gMain.vblankCounter2;

    // Get the number of seconds the RTC has been on.
    // This will always fit in a u32.
    RtcGetInfo(&rtc);
    seconds = (u32)RtcGetDayCount(&rtc) * 86400u;
    seconds += rtc.hour * 3600u;
    seconds += rtc.minute * 60u;
    seconds += rtc.second;

    return seconds;

}

// Random and Random2 use (almost) the same seed data, but with different
// padding so that they don't produce the same results.
static const u32 RANDOM_PADDING = 0xBA5EBA11u;
static const u32 RANDOM_2_PADDING = 0x5CAFF01Du;

static u32 ReadTimers(void)
{
    u32 result;

    REG_TM1CNT_H = 0;
    result = ((u32)REG_TM2CNT_L << 16) | REG_TM1CNT_L;
    REG_TM1CNT_H = 0x80;
    return result;
}


void SeedRng1(void)
{
    gRngStatus = UNINITIALIZED;
    SeedRngState(&gRngState, ReadTimers(), GetSeedSecondaryData(), RANDOM_PADDING);
    gRngStatus = IDLE;
}

void SeedRng2(void)
{
    SeedRngState(&gRng2State, ReadTimers(), GetSeedSecondaryData(), RANDOM_2_PADDING);
}
#undef READ_TIMERS

void BootSeedRng(void)
{
    SeedRng1();
    SeedRng2();
}

// Starts running the seed timers. After this point they are free running.
void StartSeedTimer(void)
{
    gRngStatus = UNINITIALIZED;
    REG_TM1CNT_H = 0x80;
    REG_TM2CNT_H = 0x84;
}
