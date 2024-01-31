#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

#include "config/randomizer.h"
#if RZ_ENABLE == TRUE

#define RANDOMIZER_MAX_MON  (SPECIES_DECIDUEYE_HISUIAN)

#include "global.h"
#include "random.h"
#include "script.h"
#include "wild_encounter.h"
//#include "constants/moves.h"

#define RANDOMIZER_STREAM 17

enum RandomizerFeature
{
    // Wild encounter randomization.
    RZ_WILD_MON,
    // Trainer party randomization.
    RZ_TRAINER_MON,
    // Item ball and hidden item randomization.
    RZ_FIELD_ITEMS,
    // Randomization of species base stats. Not yet implemented.
    RZ_BASE_STATS,
    // Randomization of species types. Not yet implemented.
    RZ_MON_TYPES,
    // Randomization of species learnset. Not yet implemented.
    RZ_LEARNSET,
    // Randomization of fixed encounters.
    RZ_FIXED_MON,
    // Randomization of starter Pokémon species.
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
    // Controls how a species is randomized.
    RZO_SPECIES_MODE,
};

enum RandomizerSpeciesMode {
    // Any valid species can be randomized to any other valid species.
    MON_RANDOM,
    MON_RANDOM_LEGEND_AWARE,
    MON_RANDOM_BST,
    MON_EVOLUTION,
    // Other modes here.

    // A dummy mode to end the list.
    MAX_MON_MODE
};


u32 GetRandomizerSeed(void);
bool8 RandomizerFeatureEnabled(enum RandomizerFeature feature);

struct Sfc32State RandomizerRandSeed(enum RandomizerReason reason, u32 data1, u16 data2);

// Returns the next number from a randomizer seed.
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

u16 RandomizeMon(enum RandomizerReason reason, enum RandomizerSpeciesMode mode, u32 seed, u16 species);

u16 RandomizeWildEncounter(u16 species, u8 mapNum, u8 mapGroup, enum WildArea area, u8 slot);

// Returns TRUE if it is possible for the species tableSpecies to randomize into the species matchSpecies.
// This does not mean that it actually did, though.
bool8 IsRandomizationPossible(u16 tableSpecies, u16 matchSpecies);

u16 RandomizeTrainerMon(u16 trainerId, u8 slot, u8 totalMons, u16 species);

u16 RandomizeFixedEncounterMon(u16 species, u8 mapNum, u8 mapGroup, u8 localId);

// Given a starter slot and the list of original starters, returns the starter in that slot.
u16 RandomizeStarter(u16 starterSlot, const u16* originalStarters);

#if RZ_SPECIES_TABLES_IN_RAM == TRUE
// Once the randomizer is set up, in RAM mode, you can call this to preload the tables.
// This avoids a short hitch the first time the randomizer is used.
// It's also a good idea to call this function if the randomizer mode changes.
void PreloadRandomizationTables(void);
#endif

#endif // RZ_ENABLE

#endif // GUARD_RANDOMIZER_H
