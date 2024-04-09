#ifndef GUARD_POKEMON_GSC_H
#define GUARD_POKEMON_GSC_H

#include "define_gsc.h"

struct BoxPokemonGSC
{
    u8 species;
    u8 heldItem;
    u8 moves[MAX_MON_MOVES];
    u8 otId[2];
    u8 experience[3];
    u8 hpEV[2];
    u8 attackEV[2];
    u8 defenseEV[2];
    u8 speedEV[2];
    u8 specialEV[2];
    u8 statIVs[2];
    u8 pp[MAX_MON_MOVES];
    u8 friendship;
    u8 pokerus;
    u8 caughtData[2];
    u8 level;
} __attribute__((packed));;

struct BoxGSCJP {
    u8 count;
    u8 monSpecies[BOX_MON_TOTAL_GSC_JP + 1];
    struct BoxPokemonGSC monData[BOX_MON_TOTAL_GSC_JP];
    u8 monOTNames[BOX_MON_TOTAL_GSC_JP][NAME_LENGTH_GSC_JP + 1];
    u8 monNames[BOX_MON_TOTAL_GSC_JP][NAME_LENGTH_GSC_JP + 1];
} __attribute__((packed));

struct BoxGSCIntl {
    u8 count;
    u8 monSpecies[BOX_MON_TOTAL_GSC_INTL + 1];
    struct BoxPokemonGSC monData[BOX_MON_TOTAL_GSC_INTL];
    u8 monOTNames[BOX_MON_TOTAL_GSC_INTL][NAME_LENGTH_GSC_INTL + 1];
    u8 monNames[BOX_MON_TOTAL_GSC_INTL][NAME_LENGTH_GSC_INTL + 1];
} __attribute__((packed));

struct LegalityCheckResult {
    u16 boxMonIndex : 5;
    u16 minimumLevel : 7;
    u16 moveLegalFlags : 4;
    u16 nameInvalidFlags : 16;
} __attribute__((packed));

int FindIndexIfGSCBoxMonIsEggless(struct BoxPokemonGSC *boxMonGSC);
bool8 ConvertEgglessBoxMonFromGSC(struct BoxPokemon *boxMon, struct BoxPokemonGSC *boxMonGSC, u8 *origNickName, u8 *origOtName, u8 otGender, u8 gameVer, u8 language, struct LegalityCheckResult * legalityCheckResult);
bool8 ConvertBoxMonFromGSC(struct BoxPokemon *boxMon, struct BoxPokemonGSC *boxMonGSC, u8 *origNickName, u8 *origOtName, u8 otGender, u8 gameVer, u8 language, struct LegalityCheckResult * legalityCheckResult);
void MakeGSCBoxMonLegal(struct BoxPokemon *gscBoxMon, struct LegalityCheckResult *legalityCheckResult);

#endif