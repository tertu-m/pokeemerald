#ifndef GUARD_RANDOM_H
#define GUARD_RANDOM_H

extern u32 gRngValue;
extern u32 gRng2Value;

//Returns a 16-bit pseudorandom number
u16 Random(void);
u16 Random2(void);

//Returns a 32-bit pseudorandom number
#define Random32() (Random() | (Random() << 16))

// The number 1103515245 comes from the example implementation of rand and srand
// in the ISO C standard.
#define ISO_RANDOMIZE1(val)(1103515245 * (val) + 24691)
#define ISO_RANDOMIZE2(val)(1103515245 * (val) + 12345)

//Sets the initial seed value of the pseudorandom number generator
void SeedRng(u16 seed);
void SeedRng2(u16 seed);

/* Structured random number generator.
 * Instead of the caller converting bits from Random() to a meaningful
 * value, the caller provides metadata that is used to return the
 * meaningful value directly. This allows code to interpret the random
 * call, for example, battle tests know what the domain of a random call
 * is, and can exhaustively test it.
 *
 * RandomTag identifies the purpose of the value. By convention, tags
 * for RandomUniform start with URNG and tags for RandomWeighted start
 * with WRNG.
 *
 * RandomUniform(tag, lo, hi) returns a number from lo to hi inclusive.
 *
 * RandomWeighted(tag, w0, w1, ... wN) returns a number from 0 to N
 * inclusive. The return value is proportional to the weights, e.g.
 * RandomWeighted(..., 1, 1) returns 50% 0s and 50% 1s.
 * RandomWeighted(..., 3, 1) returns 75% 0s and 25% 1s. */

enum RandomTag
{
    RNG_NONE,

    // Uniform tags.
    URNG_DAMAGE_MODIFIER,

    URNG_FORCE_RANDOM_SWITCH,
    URNG_RAMPAGE_TURNS,
    URNG_SLEEP_TURNS,

    // Weighted tags.
    WRNG_ACCURACY,
    WRNG_CONFUSION,
    WRNG_CRITICAL_HIT,
    WRNG_FROZEN,
    WRNG_INFATUATION,
    WRNG_PARALYSIS,
    WRNG_SECONDARY_EFFECT,
    WRNG_SPEED_TIE,

    WRNG_HOLD_EFFECT_FLINCH,

    WRNG_CUTE_CHARM,
    WRNG_FLAME_BODY,
    WRNG_POISON_POINT,
    WRNG_STATIC,
    WRNG_STENCH,
};

#define RandomWeighted(tag, ...) \
    ({ \
        const u8 weights[] = { __VA_ARGS__ }; \
        RandomWeightedArray(tag, ARRAY_COUNT(weights), weights); \
    })

u32 RandomUniform(enum RandomTag, u32 lo, u32 hi);
u32 RandomWeightedArray(enum RandomTag, u32 n, const u8 *weights);

u32 RandomUniformDefault(enum RandomTag, u32 lo, u32 hi);
u32 RandomWeightedArrayDefault(enum RandomTag, u32 n, const u8 *weights);

#endif // GUARD_RANDOM_H
