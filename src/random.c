#include "global.h"
#include "random.h"
#if MODERN
#include <alloca.h>
#endif

// IWRAM common
struct RngState gRngState;
struct RngState gRng2State;

enum RandomStatus {
    UNINITIALIZED,
    UNLOCKED,
    LOCKED
};

static volatile EWRAM_DATA enum RandomStatus sRngStatus = UNINITIALIZED;

#define SEED_ITERATIONS 20
static inline void Seed_Internal(struct RngState *state, u32 seed)
{
    u32 i;

    state->a = 0xBA5EBA11u;
    state->b = 1349209454u; // 'PkMn'
    state->c = seed;
    state->ctr = 1;

    for(i = 0; i < SEED_ITERATIONS; i++)
    {
        SFC32_Next(state);
    }
}

u32 Random32(void)
{
    u32 result;

    sRngStatus = LOCKED;
    result = SFC32_Next(&gRngState);
    sRngStatus = UNLOCKED;

    return result;
}

u32 Random2_32(void)
{
   return SFC32_Next(&gRng2State);
}

void SeedRng(u32 seed)
{
    Seed_Internal(&gRngState, seed);
    sRngStatus = UNLOCKED;
}

void SeedRng2(u32 seed)
{
    Seed_Internal(&gRng2State, seed);
}

void AdvanceRandom(void)
{
    if (sRngStatus == UNLOCKED)
    {
        SFC32_Next(&gRngState);
    }
}

#define SHUFFLE_IMPL \
    u32 tmp; \
    --n; \
    while (n > 1) \
    { \
        int j = (Random() * (n+1)) >> 16; \
        SWAP(data[n], data[j], tmp); \
        --n; \
    }

void Shuffle8(void *data_, size_t n)
{
    u8 *data = data_;
    SHUFFLE_IMPL;
}

void Shuffle16(void *data_, size_t n)
{
    u16 *data = data_;
    SHUFFLE_IMPL;
}

void Shuffle32(void *data_, size_t n)
{
    u32 *data = data_;
    SHUFFLE_IMPL;
}

void ShuffleN(void *data, size_t n, size_t size)
{
    void *tmp = alloca(size);
    --n;
    while (n > 1)
    {
        int j = (Random() * (n+1)) >> 16;
        memcpy(tmp, (u8 *)data + n*size, size); // tmp = data[n];
        memcpy((u8 *)data + n*size, (u8 *)data + j*size, size); // data[n] = data[j];
        memcpy((u8 *)data + j*size, tmp, size); // data[j] = tmp;
        --n;
    }
}

__attribute__((weak, alias("RandomUniformDefault")))
u32 RandomUniform(enum RandomTag tag, u32 lo, u32 hi);

__attribute__((weak, alias("RandomUniformExceptDefault")))
u32 RandomUniformExcept(enum RandomTag, u32 lo, u32 hi, bool32 (*reject)(u32));

__attribute__((weak, alias("RandomWeightedArrayDefault")))
u32 RandomWeightedArray(enum RandomTag tag, u32 sum, u32 n, const u8 *weights);

__attribute__((weak, alias("RandomElementArrayDefault")))
const void *RandomElementArray(enum RandomTag tag, const void *array, size_t size, size_t count);

u32 RandomUniformDefault(enum RandomTag tag, u32 lo, u32 hi)
{
    return lo + (((hi - lo + 1) * Random()) >> 16);
}

u32 RandomUniformExceptDefault(enum RandomTag tag, u32 lo, u32 hi, bool32 (*reject)(u32))
{
    while (TRUE)
    {
        u32 n = RandomUniformDefault(tag, lo, hi);
        if (!reject(n))
            return n;
    }
}

u32 RandomWeightedArrayDefault(enum RandomTag tag, u32 sum, u32 n, const u8 *weights)
{
    s32 i, targetSum;
    targetSum = (sum * Random()) >> 16;
    for (i = 0; i < n - 1; i++)
    {
        targetSum -= weights[i];
        if (targetSum < 0)
            return i;
    }
    return n - 1;
}

const void *RandomElementArrayDefault(enum RandomTag tag, const void *array, size_t size, size_t count)
{
    return (const u8 *)array + size * RandomUniformDefault(tag, 0, count - 1);
}
