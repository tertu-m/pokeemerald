# SFC32 RNG for Pokémon Emerald
This provides additional documentation for those wishing to make more involved changes to the codebase using these functions, or for people looking at the code.

## New basic functions and macros
* `void BurnRandom(void)`: Updates `gRngState` during VBlank. This is *necessary* to use in VBlank callbacks as it is possible for corruption to occur otherwise.
    * There is no `BurnRandom2()` because none of the vanilla VBlank callbacks update the second RNG state.
* `void BootSeedRng(void)`: Performs the initial RNG seed. You should not need to call this function yourself.
* `u16 ContestCompatRandom(void)`: Replaces `Random()` calls that were made in contest code, handling link compatibility. All contest code must use this call where it originally used `Random()` to remain link-compatible with vanilla games.
* `u16 CompactRandom(union CompactRandomState *)`: An alternate random number generator designed to use a 16-bit state.
    * This should be used only where the RNG state needs to fit into 16 bits or is used for procedural generation.
* `u16 CountLeadingZeroes(u32 x)`: Returns the number of leading zeroes in `x`, that is, the number of 0 bits before the first 1 bit. This is used by one of the `RandomRangeGood()` implementations.
* `u32 GetSeedSecondaryData(void)`: Returns 32 bits data suitable for seeding a random number generator.
    * If the RTC is working, this is the current number of seconds on the RTC, otherwise it is the count of frames since boot.
* `u32 Random2_32()`: Like `Random32()`, but uses the second RNG state instead.
* `u16 RandomBits(x)/Random2Bits(x)`: Returns a random value of `x` bits, where `x` is 1-16.
* `u32 RandomBits32(x)/Random2Bits32(x)`: Returns a random value of `x` bits, where `x` is 1-32.
* `u16 RandomRangeFast(x)/Random2RangeFast(x)`: Returns a random number from 0 to `x`-1.
    * This uses a very fast method of generating random numbers in a range that will return biased results if `x` is not a power of 2.
* `u16 RandomRangeGood(x)/Random2RangeGood(x)`: Returns a random number from 0 to `x`-1.
    * This removes bias from its results by calling `Random32()` multiple times in some circumstances, making it slower than `RandomRangeFast()`.
* `u16 RandomModTarget(modulo, target)/u32 RandomModTarget32(modulo, target)`: Returns a random number `x` between 0 and UINT16_MAX or UINT32_MAX (depending on the variant) that satisfies the equation `x % modulo == target`.
* `void SeedRng1(void)/void SeedRng2(void)`: Reseeds `gRngState` or `gRngState2`. You should not need to call this function yourself.
* `void StartSeedTimer(void)`: Sets up Timer 1 and Timer 2 for use in RNG seeding. You should not need to call this function yourself.
    * This is called in `AgbMain()` by default and must be called before any `SeedRng` functions are called.

## Removed or disabled functions
* `void SeedRngWithRtc(void)`: The `SeedRng` functions handle RNG seeding, including the RTC where appropriate.
* `void StartTimer1(void)`: Reworked into `StartSeedTimer()`.
* `void SeedRng(u32 x)/SeedRng2(u32 x)`: A 32-bit seed is no longer enough to seed SFC32.
    * Additionally, most of the vanilla uses of these functions were not strictly necessary. It is simple to build a private RNG state using `ISO_RANDOMIZE*` for these cases, and this was done in this patch.

## New advanced/internal functions and macros
These functions are more specialized or require extra care to use.<br>
There are additional functions present that are not documented.

* `u32 _GetRng32(struct RngState *state)`: Updates the RNG state at `state` and returns its next 32-bit output.
    * This is the core function of the new RNG system. Everything else is built around it.
    * This function, like all functions with the prefix `_GetRng`, does not lock or unlock `gRngState`, which is *necessary* to avoid state corruption. If you use it with `gRngState`, you must call `_LockRandom()` first and then call `_UnlockRandom()` when you are done.
* `u16 _GetRngRangeGood_Mask(struct RngState *state, u16 bound)`: Implements `RandomRangeGood(bound)` using bitmasked rejection sampling. This is faster when the argument is not a constant.
    * The `RandomRangeGood` macros will choose between this function and the following function automatically.
* `u16 _GetRngRangeGood_Multiply(struct RngState *state, u16 bound)`: Implements `RandomRangeGood(bound)` using Lemire's method. This is faster when the argument is a constant.