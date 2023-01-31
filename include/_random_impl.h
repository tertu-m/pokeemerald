// DO NOT INCLUDE THIS FILE DIRECTLY!!

#if !defined(RANDOM_IMPL_CONST) || !defined(RANDOM_IMPL_NONCONST)
#error _random_impl.h was included incorrectly.
#endif

RANDOM_IMPL_NONCONST u32 _GetRng32(struct RngState *state) {
    u32 b, c, result;

    b = state->b;
    c = state->c;
    result = state->a + b + state->counter++;

    state->a = b ^ (b >> 9);
    state->b = c * 9;
    state->c = ((c << 21) | (c >> 11)) + result;

    return result;
}


RANDOM_IMPL_NONCONST u32 Random32(void) {
    u32 result;

    _LockRandom();
    result = _GetRng32(&gRngState);
    _UnlockRandom();

    return result;
}

RANDOM_IMPL_NONCONST u32 Random2_32(void) {
    return _GetRng32(&gRng2State);
}

RANDOM_IMPL_CONST const u16 CountLeadingZeroes(const u32 value)
{
    u32 modified_value;

    if (value == 0)
        return 0;

    modified_value = value | (value >> 1);
    modified_value |= modified_value >> 2;
    modified_value |= modified_value >> 4;
    modified_value |= modified_value >> 8;
    modified_value |= modified_value >> 16;

    return clz_Lookup[(modified_value * 0x07C4ACDDU) >> 27];
}

RANDOM_IMPL_NONCONST u16 _GetRngRangeGood_Multiply(struct RngState *state, const u16 range)
{
    u16 scaled_lower_half, smallest_lower_half, random;
    u32 scaled_random;

    if (range == 0) return 0;

    // This lets us compute (UINT16_MAX+1) % range with 16-bit modulo.
    // The compiler should optimize this out, but in case it doesn't...
    smallest_lower_half = (u16)(~range+1) % range;

    do {
        random = (u16)(_GetRng32(state) >> 16);
        scaled_random = (u32)random * (u32)range;
        scaled_lower_half = (u16)scaled_random;
    } while (scaled_lower_half < smallest_lower_half);

    return (u16)(scaled_random >> 16);
}

RANDOM_IMPL_NONCONST u32 _GetRngRangeGood_Mask(struct RngState *state, const u16 range)
{
    u32 mask, candidate;
    u32 adjusted_range;

    if (range == 0)
        return 0;

    adjusted_range = range - 1;
    mask = ~0U;
    mask >>= CountLeadingZeroes((u32)range);

    do {
        candidate = _GetRng32(state) & mask;
    } while (candidate > adjusted_range);

    return candidate;
}

// Returns a u16 result chosen so result % modulo == target.
RANDOM_IMPL_NONCONST u16 _GetRngModTarget16(struct RngState *state, const u16 modulo, const u16 target)
{
    u16 maximum, difference, baseValue;
    maximum = UINT16_MAX / modulo;
    difference = UINT16_MAX - (maximum * modulo);

    if (target <= difference)
        maximum++;

    return _GetRngRangeGood_Multiply(state, maximum) * modulo + target;
}

RANDOM_IMPL_NONCONST u32 _GetRngModTarget32(struct RngState *state, const u32 modulo, const u32 target)
{
    u32 maximum, difference, baseValue, adjusted_range, mask, candidate;
    maximum = UINT32_MAX / modulo;
    difference = UINT32_MAX - (maximum * modulo);

    if (target <= difference)
        maximum++;

    return _GetRngRangeGood_Mask(state, maximum) * modulo + target;
}

RANDOM_IMPL_NONCONST u16 _RandomRangeGood_Multiply(const u16 range)
{
    u16 result;
    _LockRandom();
    result = _GetRngRangeGood_Multiply(&gRngState, range);
    _UnlockRandom();
    return result;
}

RANDOM_IMPL_NONCONST u16 _RandomRangeGood_Mask(const u16 range)
{
    u16 result;
    _LockRandom();
    result = (u16)_GetRngRangeGood_Mask(&gRngState, range);
    _UnlockRandom();
    return result;
}

RANDOM_IMPL_NONCONST u16 _Random2RangeGood_Multiply(const u16 range)
{
    return _GetRngRangeGood_Multiply(&gRng2State, range);
}

RANDOM_IMPL_NONCONST u16 _Random2RangeGood_Mask(const u16 range)
{
    return (u16)_GetRngRangeGood_Mask(&gRng2State, range);
}

#undef RANDOM_IMPL_CONST
#undef RANDOM_IMPL_NONCONST
