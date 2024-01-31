#ifndef GUARD_CONFIG_RANDOMIZER_H
#define GUARD_CONFIG_RANDOMIZER_H
#include "item.h"

// Global control. If FALSE, no randomizer functionality will be enabled.
// If this is TRUE, that doesn't necessarily mean that a particular part of the randomizer
// will be enabled.
#define RZ_ENABLE                   TRUE

#if RZ_ENABLE == TRUE

// If TRUE, the trainer ID (including secret ID) will be the randomizer seed.
#define RZ_TRAINER_ID_IS_SEED       TRUE

// If TRUE, dynamically generated randomization tables stored in EWRAM are used.
// This consumes 6 bytes for each species present.
#define RZ_SPECIES_TABLES_IN_RAM    TRUE

#if RZ_SPECIES_TABLES_IN_RAM == TRUE

// If the longest evolutionary chain (excluding babies) is longer than this,
// the dynamic evolutionary stage randomization table will be generated
// incorrectly.
#define RZ_MAX_EVO_STAGES   5

#endif // RZ_SPECIES_TABLES_IN_RAM

#define RZ_MAX_TM           ITEM_TM50

// Vars and features

// These features allow you to force enable or disable individual randomization
// features.
// If undefined, the feature will be enabled if one of the flags below is set.
// If defined and set to TRUE, the feature will always be enabled.
// If defined and set to FALSE, the feature will always be disabled.
//#define RZ_WILD_MON_FORCE         TRUE
//#define RZ_FIELD_ITEMS_FORCE      TRUE
//#define RZ_TRAINER_MON_FORCE      TRUE
//#define RZ_FIXED_MON_FORCE        TRUE
//#define RZ_STARTERS_FORCE    TRUE

// These flags control whether a particular randomization feature is active.
// They are ignored and disabled if the _FORCE flags above are set.
#ifndef RZ_WILD_MON_FORCE
#define RZ_FLAG_WILD_MON            FLAG_UNUSED_0x020
#endif

#ifndef RZ_FIELD_ITEMS_FORCE
#define RZ_FLAG_FIELD_ITEMS         FLAG_UNUSED_0x021
#endif

#ifndef RZ_TRAINER_MON_FORCE
#define RZ_FLAG_TRAINER_MON         FLAG_UNUSED_0x022
#endif

#ifndef RZ_FIXED_MON_FORCE
#define RZ_FLAG_FIXED_MON           FLAG_UNUSED_0x023
#endif

#ifndef RZ_STARTERS_FORCE
#define RZ_FLAG_STARTERS            FLAG_UNUSED_0x024
#endif

#define RZ_VAR_SPECIES_MODE         VAR_UNUSED_0x404E

#if RZ_TRAINER_ID_IS_SEED == FALSE
#define RZ_VAR_SEED_L               VAR_UNUSED_0x40FA
#define RZ_VAR_SEED_H               VAR_UNUSED_0x40FB
#endif

#endif // RZ_ENABLE

#endif // GUARD_CONFIG_RANDOMIZER_H
