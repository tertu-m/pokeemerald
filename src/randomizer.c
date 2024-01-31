#include "randomizer.h"

#if RZ_ENABLE == TRUE
#include "main.h"
#include "new_game.h"
#include "item.h"
#include "event_data.h"
#include "field_control_avatar.h"
#include "pokemon.h"
#include "script.h"
#include "data.h"

bool8 RandomizerFeatureEnabled(enum RandomizerFeature feature)
{
    switch(feature)
    {
        case RZ_WILD_MON:
            #ifdef RZ_WILD_MON_FORCE
                return RZ_WILD_MON_FORCE;
            #else
                return FlagGet(RZ_FLAG_WILD_MON);
            #endif
        case RZ_FIELD_ITEMS:
            #ifdef RZ_FIELD_ITEMS_FORCE
                return RZ_FIELD_ITEMS_FORCE;
            #else
                return FlagGet(RZ_FLAG_FIELD_ITEMS);
            #endif
        case RZ_TRAINER_MON:
            #ifdef RZ_TRAINER_MON_FORCE
                return RZ_TRAINER_MON_FORCE;
            #else
                return FlagGet(RZ_FLAG_TRAINER_MON);
            #endif
        case RZ_FIXED_MON:
            #ifdef RZ_FIXED_MON_FORCE
                return RZ_FIXED_MON_FORCE;
            #else
                return FlagGet(RZ_FLAG_FIXED_MON);
            #endif
        case RZ_STARTERS:
            #ifdef RZ_STARTERS_FORCE
                return RZ_STARTERS_FORCE;
            #else
                return FlagGet(RZ_FLAG_STARTERS);
            #endif
        default:
            return FALSE;
    }
}

u32 GetRandomizerSeed(void)
{
    #if RZ_TRAINER_ID_IS_SEED == TRUE
        return GetTrainerId(gSaveBlock2Ptr->playerTrainerId);
    #else
        u32 result;
        result = ((u32)VarGet(RZ_VAR_SEED_H) << 16) | VarGet(RZ_VAR_SEED_L);
        return result;
    #endif
}

// Sets the seed that will be used for the randomizer if doing so is possible.
bool8 SetRandomizerSeed(u32 newSeed)
{
    #if RZ_TRAINER_ID_IS_SEED == TRUE
        // It isn't possible to set the randomizer seed in this case.
        return FALSE;
    #else
        VarSet(RZ_VAR_SEED_L, (u16)newSeed);
        VarSet(RZ_VAR_SEED_H, (u16)(newSeed >> 16));
        return TRUE;
    #endif
}

static bool32 IsSpeciesPermitted(u16 species)
{
    // SPECIES_NONE is never valid.
    if (species == SPECIES_NONE)
        return FALSE;
    // Megas should not appear as randomization choices.
    if (species >= SPECIES_VENUSAUR_MEGA && species <= SPECIES_GROUDON_PRIMAL)
        return FALSE;
    // This is used to indicate a disabled species.
    if (gSpeciesInfo[species].baseHP == 0)
        return FALSE;

    return TRUE;
};

u32 GenerateSeedForRandomizer(void)
{
    u32 data;
    u32 vblankCounter = gMain.vblankCounter1;
    #if HQ_RANDOM == TRUE
        data = Random32();
    #else
        data = gRngValue;
        Random();
    #endif
    return data ^ vblankCounter;
}

u16 GetRandomizerOption(enum RandomizerOption option)
{
    switch(option) {
        case RZO_SPECIES_MODE:
            return VarGet(RZ_VAR_SPECIES_MODE);
        default: // Unknown option.
            return 0;
    }
}

/* Seeds an SFC32 random number generator state for the randomizer.

SFC32 can be seeded with up to 96 bits of data.
RandomizerRandSeed uses 32 of those for the global randomizer seed, which
represents a particular 'run' a player is on.
The reason code is used to ensure random numbers used for different purposes
with the same data1 and data2 do not produce the same results.
Finally, data1 and data2 are 48 bits of data that a caller can use for
any purpose they wish. Certain groups of functions will assign a purpose to
these: for example, RandomizeMon uses data2 for the original species number.
*/
struct Sfc32State RandomizerRandSeed(enum RandomizerReason reason, u32 data1, u16 data2)
{
    struct Sfc32State state;
    u32 i;
    state.a = GetRandomizerSeed();
    state.b = ((u32)reason << 16) | data2;
    state.c = data1;
    state.ctr = RANDOMIZER_STREAM;

    for (i = 0; i < 10; i++)
    {
        _SFC32_Next_Stream(&state, RANDOMIZER_STREAM);
    }

    return state;
}


// This uses a debiased multiplication technique sometimes known as
// Lemire's method. It usually does not need to do a division.
u16 RandomizerNextRange(struct Sfc32State* state, u16 range)
{
    u16 lower;
    u32 scaled;

    if (range < 2)
        return 0;

    scaled = (u32)RandomizerNext(state) * (u32)range;
    lower = (u16)scaled;

    if (lower < range)
    {
        u16 adjusted_range;
        adjusted_range = (u16)((UINT16_MAX + 1u) % range);
        while (lower < adjusted_range)
        {
            scaled = (u32)RandomizerNext(state) * (u32)range;
            lower = (u16)scaled;
        }
    }

    return scaled >> 16;
}

// Functions for producing single seeded random numbers.
u16 RandomizerRand(enum RandomizerReason reason, u32 data1, u16 data2)
{
    struct Sfc32State state;
    state = RandomizerRandSeed(reason, data1, data2);
    return RandomizerNext(&state);
}

u16 RandomizerRandRange(enum RandomizerReason reason, u32 data1, u16 data2, u16 range)
{
    struct Sfc32State state;
    state = RandomizerRandSeed(reason, data1, data2);
    return RandomizerNextRange(&state, range);
}

// Utility functions for the field item randomizer.
static inline bool8 IsItemTMHM(u16 itemId)
{
    return ItemId_GetPocket(itemId) == POCKET_TM_HM;
}

static inline bool8 IsItemHM(u16 itemId)
{
    return itemId >= ITEM_HM01 && IsItemTMHM(itemId);
}

static inline bool8 IsKeyItem(u16 itemId)
{
    return ItemId_GetPocket(itemId) == POCKET_KEY_ITEMS;
}

// Don't randomize HMs or key items, that can make the game unwinnable.
// ITEM_NONE also should not be randomized as it is invalid.
static inline bool8 ShouldRandomizeItem(u16 itemId)
{
    return !(IsItemHM(itemId) || IsKeyItem(itemId) || itemId == ITEM_NONE);
}

#include "data/randomizer/item_whitelist.h"

// Given a found item and its location in the game, returns a replacement for that item.
u16 RandomizeFoundItem(u16 itemId, u8 mapNum, u8 mapGroup, u8 localId)
{
    struct Sfc32State state;
    u16 result;
    u32 mapSeed;

    if (!ShouldRandomizeItem(itemId))
        return itemId;

    // Seed the generator using the original item and the object event that led up
    // to this call.
    mapSeed = ((u32)mapGroup) << 16;
    mapSeed |= ((u32)mapNum) << 8;
    mapSeed |= localId;

    state = RandomizerRandSeed(RZR_FIELD_ITEM, mapSeed, itemId);

    // Randomize TMs to TMs. Because HMs shouldn't be randomized, we can assume
    // this is a TM.
    if (IsItemTMHM(itemId))
        return RandomizerNextRange(&state, RZ_MAX_TM - ITEM_TM01 + 1) + ITEM_TM01;

    // Randomize everything else to everything else.
    do {
        result = sRandomizerItemWhitelist[RandomizerNextRange(&state, ITEM_WHITELIST_SIZE)];
    } while(!ShouldRandomizeItem(result) || IsItemTMHM(result));

    return result;

}

// Takes a SpecialVar as an argument to simplify handling separate scripts.
static inline void RandomizeFoundItemScript(u16 *scriptVar)
{
    if (RandomizerFeatureEnabled(RZ_FIELD_ITEMS))
    {
        // Pull the object event information from the current object event.
        u8 objEvent = gSelectedObjectEvent;
        *scriptVar = RandomizeFoundItem(
            *scriptVar,
            gObjectEvents[objEvent].mapGroup,
            gObjectEvents[objEvent].mapNum,
            gObjectEvents[objEvent].localId);
    }
}

// These functions are invoked by the scripts that handle found items and
// write the results of the randomization to the correct script variable.
void FindItemRandomize_NativeCall(struct ScriptContext *ctx)
{
    RandomizeFoundItemScript(&gSpecialVar_0x8000);
}

void FindHiddenItemRandomize_NativeCall(struct ScriptContext *ctx)
{
    RandomizeFoundItemScript(&gSpecialVar_0x8005);
}

// Both legendary and mythical Pokémon are included
static inline bool32 IsRandomizerLegendary(u16 species)
{
    return gSpeciesInfo[species].isLegendary
        || gSpeciesInfo[species].isMythical
        || gSpeciesInfo[species].isUltraBeast;
}

#define RZ_SPECIES_COUNT (RANDOMIZER_MAX_MON)

struct SpeciesGroupEntry
{
    u16 species;
    u16 group;
};

struct SpeciesTable
{
    u16 speciesIndex[RZ_SPECIES_COUNT];
    struct SpeciesGroupEntry speciesGroups[RZ_SPECIES_COUNT];
};

#define GROUP_INVALID   0xFFFF

static inline u16 GetSpeciesGroup(const struct SpeciesTable* table, u16 species)
{
    struct SpeciesGroupEntry groupEntry;
    if (species == SPECIES_NONE)
        return GROUP_INVALID;
    species -= 1;
    groupEntry = table->speciesGroups[table->speciesIndex[species]];

    #ifndef NDEBUG
        MgbaPrintf(MGBA_LOG_INFO, "GetSpeciesGroup: input %lu species %lu group %lu",
            (unsigned long)species+1, (unsigned long)groupEntry.species, (unsigned long)groupEntry.group);
    #endif

    return groupEntry.group;

}

static void GetGroupRange(u16 group, enum RandomizerSpeciesMode mode, u16 *resultMin, u16 *resultMax)
{
    if (group == GROUP_INVALID)
    {
        *resultMax = *resultMin = group;
        return;
    }

    if (mode == MON_RANDOM_BST)
    {
        // Choose a 10.24% range around the base BST.
        s32 base, minScaled, maxScaled;
        base = group * 1024;
        minScaled = (base - group * 100) / 1024;
        maxScaled = (base + group * 100) / 1024;
        *resultMin = (u16)max(minScaled, 0);
        *resultMax =(u16)min(maxScaled, 0xFFFE);
    }
    else
        *resultMax = *resultMin = group;
}

//
static void GetIndicesFromGroupRange(const struct SpeciesTable *table, u16 minGroup, u16 maxGroup, u16 *start, u16 *end)
{
    u16 index, leftBound, rightBound, maxRightBound;
    maxRightBound = RZ_SPECIES_COUNT;
    maxGroup = min(0xFFFEu, maxGroup);
    minGroup = min(0xFFFEu, minGroup);
    leftBound = 0;
    rightBound = RZ_SPECIES_COUNT;
    // Do leftmost binary search to find the lower limit.
    while (leftBound < rightBound)
    {
        u16 leftFoundGroup;
        index = (leftBound + rightBound) / 2;
        leftFoundGroup = table->speciesGroups[index].group;
        if (leftFoundGroup < minGroup)
            leftBound = index + 1;
        else
        {
            if (leftFoundGroup > maxGroup)
                maxRightBound = index;
            rightBound = index;
        }
    }
    *start = leftBound;

    rightBound = maxRightBound;

    // Do rightmost binary search to find the upper limit.
    while (leftBound < rightBound)
    {
        index = (leftBound + rightBound) / 2;
        if (table->speciesGroups[index].group > maxGroup)
            rightBound = index;
        else
            leftBound = index + 1;
    }
    *end = rightBound - 1;
}

#if RZ_SPECIES_TABLES_IN_RAM == TRUE

struct RamSpeciesTable
{
    enum RandomizerSpeciesMode mode;
    bool8 tableInitialized;
    struct SpeciesTable speciesTable;
};

EWRAM_DATA static struct RamSpeciesTable sRamSpeciesTable = {0};

static void FillSpeciesGroupsRandom(struct SpeciesGroupEntry* entries)
{
    u16 i;
    for (i = 1; i <= RANDOMIZER_MAX_MON; i++)
    {
        entries[i-1].species = i;
        if (IsSpeciesPermitted(i))
            entries[i-1].group = 0;
        else
            entries[i-1].group = GROUP_INVALID;
    }
}

static void FillSpeciesGroupsBST(struct SpeciesGroupEntry* entries)
{
    u16 i;
    for(i = 1; i <= RANDOMIZER_MAX_MON; i++)
    {
        struct SpeciesGroupEntry entry;
        const struct SpeciesInfo *curSpeciesInfo;
        u16 bst;

        entry.species = i;

        if (IsSpeciesPermitted(i))
        {
            curSpeciesInfo = &gSpeciesInfo[i];

            bst = curSpeciesInfo->baseAttack;
            bst += curSpeciesInfo->baseDefense;
            bst += curSpeciesInfo->baseSpAttack;
            bst += curSpeciesInfo->baseSpDefense;
            bst += curSpeciesInfo->baseHP;
            bst += curSpeciesInfo->baseSpeed;

            entry.group = bst;
        }
        else
            entry.group = GROUP_INVALID;

        entries[i-1] = entry;
    }
}

static void FillSpeciesGroupsLegendary(struct SpeciesGroupEntry* entries)
{
    u16 i;
    for(i = 1; i <= RANDOMIZER_MAX_MON; i++)
    {
        struct SpeciesGroupEntry entry;
        entry.species = i;
        if (!IsSpeciesPermitted(i))
            entry.group = GROUP_INVALID;
        else
            entry.group = IsRandomizerLegendary(i);
        entries[i-1] = entry;
    }
}

// This is a list of baby Pokémon that should not cause their evolution
// to count as an evolved pokemon.
static const u16 sPreevolutionBabyMons[] =
{
    SPECIES_PICHU,
    SPECIES_CLEFFA,
    SPECIES_IGGLYBUFF,
    SPECIES_TYROGUE,
    SPECIES_SMOOCHUM,
    SPECIES_ELEKID,
    SPECIES_MAGBY,
    SPECIES_AZURILL,
    SPECIES_WYNAUT,
    SPECIES_BUDEW,
    SPECIES_CHINGLING,
    SPECIES_BONSLY,
    SPECIES_MIME_JR,
    SPECIES_HAPPINY,
    SPECIES_MUNCHLAX,
    SPECIES_MANTYKE,
};

static void MarkEvolutions(struct SpeciesGroupEntry *entries, u16 species, u16 stage)
{
    const struct Evolution *evos;
    if (stage == RZ_MAX_EVO_STAGES)
        return;

    evos = GetSpeciesEvolutions(species);
    if (evos != NULL)
    {
        u32 i;
        for (i = 0; evos[i].method != 0xFFFF; i++)
        {
            if(entries[species-1].group <= stage)
                MarkEvolutions(entries, evos[i].targetSpecies, stage+1);
        }
    }
    entries[species-1].species = species;
    entries[species-1].group = stage;
}

static void FillSpeciesGroupsEvolution(struct SpeciesGroupEntry* entries)
{
    u16 i;
    static const u8 EVO_GROUP_LEGENDARY = 0x81;
    static const u8 EVO_GROUP_NO_EVO = RZ_MAX_EVO_STAGES+1;

    // Step 0: zero everything
    memset(entries, 0, sizeof(sRamSpeciesTable.speciesTable.speciesGroups));

    // Step 1: pre-visit the special babies
    for (i = 0; i < ARRAY_COUNT(sPreevolutionBabyMons); i++)
    {
        u16 babyMonIndex = sPreevolutionBabyMons[i]-1;
        entries[babyMonIndex].species = babyMonIndex + 1;
        if(IsSpeciesPermitted(babyMonIndex))
            entries[babyMonIndex].group = 0;
        else
            entries[babyMonIndex].group = GROUP_INVALID;
    }

    for(i = 1; i < RANDOMIZER_MAX_MON; i++)
    {
        u16 idx = i - 1;
        if (entries[idx].species == 0) // This entry hasn't been visited yet.
        {
            const struct Evolution *evos = GetSpeciesEvolutions(i);
            entries[idx].species = i;
            if (!IsSpeciesPermitted(i)) // This shouldn't show up in randomization.
                entries[idx].group = GROUP_INVALID;
            else if (IsRandomizerLegendary(i)) // Legendaries get their own group.
                entries[idx].group = EVO_GROUP_LEGENDARY;
            else if (evos == NULL || evos->method == 0xFFFF) //
                entries[idx].group = EVO_GROUP_NO_EVO;
            else // There are evolutions! Let's check it out.
                MarkEvolutions(entries, i, 0);
        }
    }
}

static inline u16 LeftChildIndex(u16 index)
{
    return 2*index + 1;
}

static void BuildRandomizerSpeciesTable(enum RandomizerSpeciesMode mode)
{
    u16 i, start, end;
    struct SpeciesGroupEntry temp;
    struct SpeciesGroupEntry *groupTable;

    sRamSpeciesTable.tableInitialized = TRUE;
    sRamSpeciesTable.mode = mode;
    groupTable = sRamSpeciesTable.speciesTable.speciesGroups;

    switch(mode)
    {
        case MON_RANDOM_LEGEND_AWARE:
            FillSpeciesGroupsLegendary(groupTable);
            break;
        case MON_RANDOM_BST:
            FillSpeciesGroupsBST(groupTable);
            break;
        case MON_EVOLUTION:
            FillSpeciesGroupsEvolution(groupTable);
            break;
        case MON_RANDOM:
        default:
            FillSpeciesGroupsRandom(groupTable);
    }

    // Heap sort the table.
    start = RZ_SPECIES_COUNT/2;
    end = RZ_SPECIES_COUNT;

    while (end > 1)
    {
        u16 root;
        if (start > 0)
            start = start - 1;
        else
        {
            end = end - 1;
            SWAP(groupTable[end], groupTable[0], temp);
        }
        root = start;
        while(LeftChildIndex(root) < end)
        {
            u16 child;
            child = LeftChildIndex(root);

            if (child+1 < end
                && groupTable[child].group < groupTable[child+1].group)
            {
                child = child + 1;
            }

            if (groupTable[root].group < groupTable[child].group)
            {
                SWAP(groupTable[root], groupTable[child], temp);
                root = child;
            }
            else
                break;
        }
    }


    // Build the species index. This is needed for getting a group from a species.
    for (i = 0; i < RZ_SPECIES_COUNT; i++)
    {
        u16 targetIndex = groupTable[i].species - 1;
        sRamSpeciesTable.speciesTable.speciesIndex[targetIndex] = i;
    }
}

static const struct SpeciesTable* GetSpeciesTable(enum RandomizerSpeciesMode mode)
{
    if (!sRamSpeciesTable.tableInitialized || mode != sRamSpeciesTable.mode )
        BuildRandomizerSpeciesTable(mode);

    return &sRamSpeciesTable.speciesTable;
}

void PreloadRandomizationTables(void)
{
    GetSpeciesTable(GetRandomizerOption(RZO_SPECIES_MODE));
}

#endif

// Don't call this for MON_RANDOM, it won't work properly.
static u16 RandomizeMonTableLookup(struct Sfc32State* state, enum RandomizerSpeciesMode mode, u16 species)
{
    u16 minGroup, maxGroup, originalGroup, resultIndex;
    u16 minIndex, maxIndex;
    const struct SpeciesTable *table;

    table = GetSpeciesTable(mode);
    originalGroup = GetSpeciesGroup(table, species);

    if (originalGroup == GROUP_INVALID)
        return species;

    GetGroupRange(originalGroup, mode, &minGroup, &maxGroup);
    GetIndicesFromGroupRange(table, minGroup, maxGroup, &minIndex, &maxIndex);
    resultIndex = RandomizerNextRange(state, maxIndex - minIndex + 1) + minIndex;
    return table->speciesGroups[resultIndex].species;
}

#undef RZ_SPECIES_COUNT

static u16 RandomizeMonFromSeed(struct Sfc32State *state, enum RandomizerSpeciesMode mode, u16 species)
{
    if (species == SPECIES_NONE)
        return species;

    if (mode >= MAX_MON_MODE)
        mode = MON_RANDOM;

    return RandomizeMonTableLookup(state, mode, species);

}

// Fills an array with count Pokémon, with no repeats.
void GetUniqueMonList(enum RandomizerReason reason, enum RandomizerSpeciesMode mode, u32 seed1, u16 seed2, u8 count, const u16 *originalSpecies, u16 *resultSpecies)
{
    u32 i, curMon;
    u32 seenMonBitVector[(RANDOMIZER_MAX_MON-1)/32+1] = {};
    struct Sfc32State state = RandomizerRandSeed(reason, seed1, seed2);

    for (i = 0; i < count; i++)
    {
        u16 curOriginal = originalSpecies[i];
        bool32 foundNextMon = FALSE;

        while (!foundNextMon)
        {
            u16 wordIndex, adjustedCurMon;
            u32 bitVectorWord;
            u8 bitIndex;

            if (!IsSpeciesPermitted(curOriginal))
            {
                // If there's non-permitted Pokémon in here, something is wrong.
                // Just pass them through without marking.
                foundNextMon = TRUE;
                curMon = curOriginal;
                continue;
            }

            // Generate a Pokémon. If it has already been generated, keep generating new ones
            // until one that hasn't been seen is picked.

            curMon = RandomizeMonFromSeed(&state, mode, curOriginal);

            // Compute the bit address of this mon.
            adjustedCurMon = curMon - 1;
            wordIndex = adjustedCurMon / 32;
            bitIndex = adjustedCurMon & 31;
            bitVectorWord = seenMonBitVector[wordIndex];

            if (bitVectorWord != 0)
            {
                // If the bit for this mon is set, it has already been generated.
                if (bitVectorWord & (1 << bitIndex))
                    continue;
            }

            bitVectorWord |= 1 << bitIndex;
            seenMonBitVector[wordIndex] = bitVectorWord;
            foundNextMon = TRUE;
        }
        resultSpecies[i] = curMon;
    }
}

u16 RandomizeMon(enum RandomizerReason reason, enum RandomizerSpeciesMode mode, u32 seed, u16 species)
{
    struct Sfc32State state;
    state = RandomizerRandSeed(reason, seed, species);
    return RandomizeMonFromSeed(&state, mode, species);
}

u16 RandomizeWildEncounter(u16 species, u8 mapNum, u8 mapGroup, enum WildArea area, u8 slot)
{
    if (RandomizerFeatureEnabled(RZ_WILD_MON))
    {
        // Randomization is done based on the map number, the WildArea, and the encounter slot.
        // This means a distinct species can appear in each encounter slot.
        u32 seed;
        seed = ((u32)mapGroup) << 24;
        seed |= ((u32)mapNum) << 16;
        seed |= ((u32)area) << 8;
        seed |= slot;

        return RandomizeMon(RZR_WILD_ENCOUNTER, GetRandomizerOption(RZO_SPECIES_MODE), seed, species);
    }

    return species;
}

// This is used in the Pokédex area map code.
bool8 IsRandomizationPossible(u16 originalSpecies, u16 targetSpecies)
{
    const enum RandomizerSpeciesMode mode = GetRandomizerOption(RZO_SPECIES_MODE);
    if (!IsSpeciesPermitted(targetSpecies) || !IsSpeciesPermitted(originalSpecies))
        return FALSE;

    if (mode != MON_RANDOM && mode < MAX_MON_MODE)
    {
        u16 minGroupOriginal, maxGroupOriginal, minGroupTarget, maxGroupTarget,
            originalGroup, targetGroup;
        const struct SpeciesTable* table;
        table = GetSpeciesTable(mode);
        originalGroup = GetSpeciesGroup(table, originalSpecies);
        targetGroup = GetSpeciesGroup(table, targetSpecies);
        GetGroupRange(originalGroup, mode, &minGroupOriginal, &maxGroupOriginal);
        GetGroupRange(targetGroup, mode, &minGroupTarget, &maxGroupTarget);

        // If the group ranges intersect, randomization is possible.
        return maxGroupOriginal >= minGroupTarget && minGroupOriginal <= maxGroupTarget;
    }

    return TRUE;
}

u16 RandomizeTrainerMon(u16 trainerId, u8 slot, u8 totalMons, u16 species)
{
    if (RandomizerFeatureEnabled(RZ_TRAINER_MON))
    {
        // The seed is based on the internal trainer number, the number of
        // Pokémon in that trainer's party, and which party position it is in.
        u32 seed;
        seed = (u32)trainerId << 16;
        seed |= (u32)totalMons << 8;
        seed |= slot;

        return RandomizeMon(RZR_TRAINER_PARTY, GetRandomizerOption(RZO_SPECIES_MODE), seed, species);
    }

    return species;
}

u16 RandomizeFixedEncounterMon(u16 species, u8 mapNum, u8 mapGroup, u8 localId)
{
    if (RandomizerFeatureEnabled(RZ_FIXED_MON))
    {
        u32 seed;
        seed = (u32)mapNum << 16;
        seed |= (u32)mapGroup << 8;
        seed |= localId;

        return RandomizeMon(RZR_FIXED_ENCOUNTER, GetRandomizerOption(RZO_SPECIES_MODE), seed, species);
    }

    return species;
}

EWRAM_DATA static u32 sLastStarterRandomizerSeed = 0;
EWRAM_DATA static u16 sRandomizedStarters[3] = {0};

u16 RandomizeStarter(u16 starterSlot, const u16* originalStarters)
{
    if (RandomizerFeatureEnabled(RZ_STARTERS))
    {
        if (sLastStarterRandomizerSeed != GetRandomizerSeed() || sRandomizedStarters[0] == SPECIES_NONE)
        {
            // The randomized starter table is stale or uninitialized. Fix that!

            // Hash the starter list so that which starters there are influences the seed.
            u32 starterHash = 5381;
            u32 i;
            for (i = 0; i < 3; i++)
            {
                u16 originalStarter = originalStarters[i];
                starterHash = ((starterHash << 5) + starterHash) ^ (u8)originalStarter;
                starterHash = ((starterHash << 5) + starterHash) ^ (u8)(originalStarter >> 8);
            }

            GetUniqueMonList(RZR_STARTER, GetRandomizerOption(RZO_SPECIES_MODE),
                starterHash, 0, 3, originalStarters, sRandomizedStarters);
        }
        return sRandomizedStarters[starterSlot];
    }

    return originalStarters[starterSlot];
}

#endif // RZ_ENABLE
