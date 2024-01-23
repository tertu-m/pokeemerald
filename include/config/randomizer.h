#ifndef GUARD_CONFIG_RANDOMIZER_H
#define GUARD_CONFIG_RANDOMIZER_H
#include "item.h"

// Global control. If FALSE, no randomizer functionality will be enabled.
#define RZ_ENABLE                   TRUE

#if RZ_ENABLE == TRUE

// If TRUE, the trainer ID (including secret ID) will be the randomizer seed.
#define RZ_TRAINER_ID_IS_SEED       TRUE

// If TRUE, the only options for species randomization are completely random and "legend aware."
#define RZ_SPECIES_BASIC_SUPPORT    FALSE

#if RZ_SPECIES_BASIC_SUPPORT == FALSE
#define RZ_SPECIES_TABLES_IN_RAM    TRUE
#endif

#define RZ_MAX_TM   ITEM_TM50

// Vars and features

#define RZ_FLAG_WILD_MON            FLAG_UNUSED_0x020
#define RZ_FLAG_FIELD_ITEMS         FLAG_UNUSED_0x021
#define RZ_FLAG_TRAINER_MON         FLAG_UNUSED_0x022
#define RZ_FLAG_FIXED_MON           FLAG_UNUSED_0x023
#define RZ_FLAG_STARTERS            FLAG_UNUSED_0x024

#define RZ_VAR_SPECIES_MODE         VAR_UNUSED_0x404E

#if RZ_TRAINER_ID_IS_SEED == FALSE
#define RZ_VAR_SEED_L               VAR_UNUSED_0x40FA
#define RZ_VAR_SEED_H               VAR_UNUSED_0x40FB
#endif

#endif // RZ_ENABLE

#endif // GUARD_CONFIG_RANDOMIZER_H
