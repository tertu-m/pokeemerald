#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

#include "config/randomizer.h"
#if RZ_ENABLE == TRUE

#define RANDOMIZER_MAX_MON  (FORMS_START)

#include "global.h"
#include "random.h"
#include "script.h"
#include "wild_encounter.h"
//#include "constants/moves.h"

#define RANDOMIZER_STREAM 17

enum RandomizerFeature
{
    RZ_WILD_MON,
    RZ_TRAINER_MON,
    RZ_FIELD_ITEMS,
    RZ_BASE_STATS,
    RZ_MON_TYPES,
    RZ_LEARNSET,
    RZ_FIXED_MON,
    RZ_STARTERS,
};


enum RandomizerReason
{
    RZR_WILD_ENCOUNTER,
    RZR_FIXED_ENCOUNTER,
    RZR_TRAINER_PARTY,
    RZR_BASE_STATS,
    RZR_SPECIES_TYPE,
    RZR_LEARNSET,
    RZR_FIELD_ITEM,
    RZR_STARTER,
};

enum RandomizerOption {
    RZO_SPECIES_MODE,
};

enum RandomizerSpeciesMode {
    MON_RANDOM,
    MON_RANDOM_LEGEND_AWARE,
    #if RZ_SPECIES_BASIC_SUPPORT == FALSE
        MON_RANDOM_BST,
        // Other modes here.
        MAX_MON_MODE
    #endif
};


u32 GetRandomizerSeed(void);
bool8 RandomizerFeatureEnabled(enum RandomizerFeature feature);

struct Sfc32State RandomizerRandSeed(enum RandomizerReason reason, u32 data1, u16 data2);

static inline u16 RandomizerNext(struct Sfc32State* state)
{
    return _SFC32_Next_Stream(state, RANDOMIZER_STREAM) >> 16;
}
u16 RandomizerNextRange(struct Sfc32State* state, u16 range);

u16 RandomizerRand(enum RandomizerReason reason, u32 data1, u16 data2);
u16 RandomizerRandRange(enum RandomizerReason reason, u32 data1, u16 data2, u16 range);

static inline u8 RandomizeMonType(u16 species, u8 typeNum)
{
    return (u8)RandomizerRandRange(RZR_SPECIES_TYPE, species, typeNum, NUMBER_OF_MON_TYPES);
}

u16 RandomizeFoundItem(u16 itemId, u8 mapNum, u8 mapGroup, u8 localId);
void FindItemRandomize_NativeCall(struct ScriptContext *ctx);
void FindHiddenItemRandomize_NativeCall(struct ScriptContext *ctx);

void RandomizerCountLegendarySpecies(void);
u16 RandomizeMon(enum RandomizerReason reason, enum RandomizerSpeciesMode mode, u32 seed, u16 species);

u16 RandomizeWildEncounter(u16 species, u8 mapNum, u8 mapGroup, enum WildArea area, u8 slot);
bool8 IsRandomizationPossible(u16 tableSpecies, u16 matchSpecies);

u16 RandomizeTrainerMon(u16 trainerId, u8 slot, u8 totalMons, u16 species);
u16 RandomizeFixedEncounterMon(u16 species, u8 mapNum, u8 mapGroup, u8 localId);
u16 RandomizeStarter(u16 starterSlot, const u16* originalStarters);

#endif // RZ_ENABLE

#endif // GUARD_RANDOMIZER_H
