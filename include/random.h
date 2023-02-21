#ifndef GUARD_RANDOM_H
#define GUARD_RANDOM_H

#include "global.h"

struct RngState {
    u32 a;
    u32 b;
    u32 c;
    u32 counter;
};

enum RngStatus {
    UNINITIALIZED,
    IDLE,
    BUSY
};

extern const u16 clz_Lookup[];
extern struct RngState gRngState;
extern struct RngState gRng2State;
extern volatile enum RngStatus gRngStatus;

#if MODERN
#define RANDOM_IMPL_NONCONST extern inline __attribute__((gnu_inline))
#define RANDOM_IMPL_CONST extern inline __attribute__((const,gnu_inline))
#define RANDOM_NONCONST extern inline __attribute__((gnu_inline))
#else
#define RANDOM_IMPL_NONCONST extern inline
#define RANDOM_IMPL_CONST extern inline __attribute__((const))
#define RANDOM_NONCONST extern inline
#endif

union CompactRandomState {
    u16 state;
    s16 state_signed;
};

// A 16-bit state version of Wyrand.
RANDOM_NONCONST u16 CompactRandom(union CompactRandomState *s)
{
    u32 hash;
    u16 temp;
    temp = s->state;
    temp += 0xFC15;
    hash = (u32)temp * 0x2AB;
    s->state = temp;
    return (u16)((hash >> 16) ^ hash);
}

RANDOM_NONCONST void _LockRandom()
{
    gRngStatus = BUSY;
}

RANDOM_NONCONST void _UnlockRandom()
{
    gRngStatus = IDLE;
}

#include "_random_impl.h"

// Returns x random bits, where x is a number between 1 and 16.
// You can pass arguments up to 32, but don't do that.
#define RandomBits(x) ((u16)(Random32() >> (32-(x))))
#define Random2Bits(x) ((u16)(Random2_32() >> (32-(x))))
// The same but arguments between 1 and 32 are valid.
#define RandomBits32(x) (Random32() >> (32-(x)))
#define Random2Bits32(x) (Random32() >> (32-(x)))

//Returns a 16-bit pseudorandom number
RANDOM_NONCONST u16 Random(void) {
    return RandomBits(16);
}

RANDOM_NONCONST u16 Random2(void) {
    return Random2Bits(16);
}

/* Burns a random number if the RNG isn't currently in use.

This is necessary because updating the RNG isn't a one-write process.
With the LCG, nothing seriously bad could happen if the VBlank interrupt
happened while the game was in Random: it would just duplicate effort.
With SFC32, as the state requires 4 writes, it's possible to corrupt the RNG
state if it's in the process of being updated, so this function checks that
that isn't happening currently before stepping the RNG.

This function cannot be guaranteed to return a result, so it never does.
*/
void BurnRandom(void);

// Generates a random number in a range from 0 to x-1.
// This approach is very fast but will be biased if x is not a power of 2 and
// should be used with caution.
#define RandomRangeFast(x) ((u16)(((u32)Random()*(u32)(x)) >> 16))
#define Random2RangeFast(x) ((u16)(((u32)Random2()*(u32)(x)) >> 16))

#define RandomBool() ((bool8)(Random32() >> 31))

// The number 1103515245 comes from the example implementation of rand and srand
// in the ISO C standard.
#define RAND_MULT 1103515245
#define ISO_RANDOMIZE1(val)(RAND_MULT * (val) + 24691)
#define ISO_RANDOMIZE2(val)(RAND_MULT * (val) + 12345)

//Sets the initial seed value of the pseudorandom number generator
void BootSeedRng(void);
void SeedRng1(void);
void SeedRng2(void);

void StartSeedTimer(void);
u32 GetSeedSecondaryData(void);

RANDOM_NONCONST void SeedRngIfNeeded(void) {
    if (gRngStatus == UNINITIALIZED)
        BootSeedRng();
}


RANDOM_NONCONST u16 RandomModTarget(const u16 modulo, const u16 target)
{
    u16 result;
    _LockRandom();
    result = _GetRngModTarget16(&gRngState, modulo, target);
    _UnlockRandom();
    return result;
}

RANDOM_NONCONST u32 RandomModTarget32(const u32 modulo, const u32 target)
{
    u32 result;
    _LockRandom();
    result = _GetRngModTarget32(&gRngState, modulo, target);
    _UnlockRandom();
    return result;
}

// Taken from Linux. Devised by Martin Uecker.
#define __is_constexpr(x) \
    (sizeof(int) == sizeof(*(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))

#define RandomRangeGood(x) (__is_constexpr((x)) ? _RandomRangeGood_Multiply((x)) : _RandomRangeGood_Mask((x)))
#define Random2RangeGood(x) (__is_constexpr((x)) ? _Random2RangeGood_Multiply((x)) : _Random2RangeGood_Mask((x)))
#define RandomPercentageGood() (RandomRangeGood(100))

#undef RANDOM_NONCONST

#endif // GUARD_RANDOM_H
