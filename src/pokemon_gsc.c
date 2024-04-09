#include "global.h"
#include "item.h"
#include "main.h"
#include "math_fast.h"
#include "pokemon.h"
#include "pokemon_gsc.h"
#include "string_util.h"
#include "string_util_gsc.h"
#include "random.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/region_map_sections.h"


#define POKEMON_NAME_LENGTH_JPN 5
#define PLAYER_NAME_LENGTH_JPN 5

#define MOVE_COUNT_GSC MOVE_BEAT_UP

#define READ_BIG_ENDIAN_16(buff) ((buff[0] << 8) | buff[1])

enum
{
    F_VERSION_SAPPHIRE = 1 << VERSION_SAPPHIRE,
    F_VERSION_RUBY = 1 << VERSION_RUBY,
    F_VERSION_EMERALD = 1 << VERSION_EMERALD,
    F_VERSION_FIRE_RED = 1 << VERSION_FIRE_RED,
    F_VERSION_LEAF_GREEN = 1 << VERSION_LEAF_GREEN,
    F_VERSION_GAMECUBE = 1 << VERSION_GAMECUBE
};

enum
{
    F_LANGUAGE_JAPANESE = 1 << LANGUAGE_JAPANESE,
    F_LANGUAGE_ENGLISH = 1 << LANGUAGE_ENGLISH,
    F_LANGUAGE_FRENCH = 1 << LANGUAGE_FRENCH,
    F_LANGUAGE_ITALIAN = 1 << LANGUAGE_ITALIAN,
    F_LANGUAGE_GERMAN = 1 << LANGUAGE_GERMAN,
    F_LANGUAGE_KOREAN = 1 << LANGUAGE_KOREAN,
    F_LANGUAGE_SPANISH = 1 << LANGUAGE_SPANISH,
    F_LANGUAGE_ALL = F_LANGUAGE_JAPANESE | F_LANGUAGE_ENGLISH | F_LANGUAGE_FRENCH | F_LANGUAGE_ITALIAN | F_LANGUAGE_GERMAN | F_LANGUAGE_SPANISH
};

enum
{
    MOVE_SOURCE_NONE,
    MOVE_SOURCE_FIXED,
    MOVE_SOURCE_LEVELUP,
    MOVE_SOURCE_BREEDING,
    MOVE_VALUE_REPEATED
};

struct EggLessPokemonExtTemplate {
    u32 otId;
    const u8 *nickName;
    const u8 *otName;
};

struct EggLessPokemonTemplate {
    u8 species;
    u8 metLevel;
    u8 metLocation;
    u8 metLanguage;
    u16 metGameVer;
    const struct EggLessPokemonExtTemplate *extTemplate;
};

static const u8 unownMetLocation[] = {
    MAPSEC_MONEAN_CHAMBER,
    MAPSEC_RIXY_CHAMBER,
    MAPSEC_LIPTOO_CHAMBER,
    MAPSEC_LIPTOO_CHAMBER,
    MAPSEC_WEEPTH_CHAMBER,
    MAPSEC_SCUFIB_CHAMBER,
    MAPSEC_SCUFIB_CHAMBER,
    MAPSEC_LIPTOO_CHAMBER,
    MAPSEC_WEEPTH_CHAMBER,
    MAPSEC_DILFORD_CHAMBER,
    MAPSEC_SCUFIB_CHAMBER,
    MAPSEC_DILFORD_CHAMBER,
    MAPSEC_RIXY_CHAMBER,
    MAPSEC_WEEPTH_CHAMBER,
    MAPSEC_LIPTOO_CHAMBER,
    MAPSEC_DILFORD_CHAMBER,
    MAPSEC_DILFORD_CHAMBER,
    MAPSEC_DILFORD_CHAMBER,
    MAPSEC_WEEPTH_CHAMBER,
    MAPSEC_SCUFIB_CHAMBER,
    MAPSEC_LIPTOO_CHAMBER,
    MAPSEC_RIXY_CHAMBER,
    MAPSEC_RIXY_CHAMBER,
    MAPSEC_RIXY_CHAMBER,
    MAPSEC_SCUFIB_CHAMBER,
    MAPSEC_VIAPOIS_CHAMBER,
};

static const struct EggLessPokemonTemplate monTemplateDITTO = {
    .species = SPECIES_DITTO,
    .metLevel = 23,
    .metLocation = MAPSEC_ROUTE_14,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_FIRE_RED | F_VERSION_LEAF_GREEN
};

static const struct EggLessPokemonTemplate monTemplateARTICUNO = {
    .species = SPECIES_ARTICUNO,
    .metLevel = 50,
    .metLocation = MAPSEC_SEAFOAM_ISLANDS,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_FIRE_RED | F_VERSION_LEAF_GREEN
};

static const struct EggLessPokemonTemplate monTemplateZAPDOS = {
    .species = SPECIES_ZAPDOS,
    .metLevel = 50,
    .metLocation = MAPSEC_POWER_PLANT,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_FIRE_RED | F_VERSION_LEAF_GREEN
};

static const struct EggLessPokemonTemplate monTemplateMOLTRES = {
    .species = SPECIES_MOLTRES,
    .metLevel = 50,
    .metLocation = MAPSEC_MT_EMBER,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_FIRE_RED | F_VERSION_LEAF_GREEN
};

static const struct EggLessPokemonTemplate monTemplateMEWTWO = {
    .species = SPECIES_MEWTWO,
    .metLevel = 70,
    .metLocation = MAPSEC_CERULEAN_CAVE,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_FIRE_RED | F_VERSION_LEAF_GREEN
};

static const struct EggLessPokemonTemplate monTemplateMEW = {
    .species = SPECIES_MEW,
    .metLevel = 30,
    .metLocation = MAPSEC_FARAWAY_ISLAND,
    .metLanguage = F_LANGUAGE_JAPANESE,
    .metGameVer = F_VERSION_EMERALD
};

static const struct EggLessPokemonTemplate monTemplateUNOWN = {
    .species = SPECIES_UNOWN,
    .metLevel = 25,
    .metLocation = MAPSEC_TANOBY_CHAMBERS,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_FIRE_RED | F_VERSION_LEAF_GREEN
};

static const struct EggLessPokemonTemplate monTemplateRAIKOU = {
    .species = SPECIES_RAIKOU,
    .metLevel = 40,
    .metLocation = 113,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_GAMECUBE
};

static const struct EggLessPokemonTemplate monTemplateENTEI = {
    .species = SPECIES_ENTEI,
    .metLevel = 40,
    .metLocation = 76,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_GAMECUBE
};

static const struct EggLessPokemonTemplate monTemplateSUICUNE = {
    .species = SPECIES_SUICUNE,
    .metLevel = 40,
    .metLocation = 55,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_GAMECUBE
};

static const struct EggLessPokemonTemplate monTemplateLUGIA = {
    .species = SPECIES_LUGIA,
    .metLevel = 70,
    .metLocation = MAPSEC_NAVEL_ROCK,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_EMERALD
};

static const struct EggLessPokemonTemplate monTemplateHO_OH = {
    .species = SPECIES_HO_OH,
    .metLevel = 70,
    .metLocation = MAPSEC_NAVEL_ROCK,
    .metLanguage = F_LANGUAGE_ALL,
    .metGameVer = F_VERSION_EMERALD
};

static const u8 CELEBI_NAME_JPN[] = _("セレビィ");
static const u8 AGETO_NAME_JPN[] = _("アゲト");

static const struct EggLessPokemonExtTemplate monTemplateCELEBIExt = {
    .otId = 31121,
    .nickName = CELEBI_NAME_JPN,
    .otName = AGETO_NAME_JPN
};

static const struct EggLessPokemonTemplate monTemplateCELEBI = {
    .species = SPECIES_CELEBI,
    .metLevel = 10,
    .metLocation = METLOC_FATEFUL_ENCOUNTER,
    .metLanguage = F_LANGUAGE_JAPANESE,
    .metGameVer = F_VERSION_RUBY | F_VERSION_GAMECUBE,
    .extTemplate = &monTemplateCELEBIExt
};

static const struct EggLessPokemonTemplate * const sEggLessPokemonTemplates[] = {
    &monTemplateDITTO,
    &monTemplateARTICUNO,
    &monTemplateZAPDOS,
    &monTemplateMOLTRES,
    &monTemplateMEWTWO,
    &monTemplateMEW,
    &monTemplateUNOWN,
    &monTemplateRAIKOU,
    &monTemplateENTEI,
    &monTemplateSUICUNE,
    &monTemplateLUGIA,
    &monTemplateHO_OH,
    &monTemplateCELEBI
};

extern const u8 gBase_fixed_moves_table[];
extern const u8 gBreeding_moves_table[];
extern const u8 gBreeding_shared_table[];
extern const u8 gEvo_reverse_table[];
extern const u8 gExternal_fixed_moves_table[];
extern const u8 gLevel_up_moves_table[];
extern const u8 gMinimum_levels[];

static s32 BinarySearch(const u8 arr[], s32 n, u8 target) {
    s32 left = 0;
    s32 right = n - 1;
    s32 mid;
    while (left <= right) {
        mid = left + (right - left) / 2;
        if (arr[mid] == target)
            return mid;
        else if (arr[mid] < target)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return -1;
}

static u32 QueryGSCBoxMonData(struct BoxPokemonGSC * mon, s32 field)
{
    switch (field)
    {
        case MON_DATA_SPECIES:
            return mon->species;
        case MON_DATA_HELD_ITEM:
            return mon->heldItem;
        case MON_DATA_MOVE1:
            return mon->moves[0];
        case MON_DATA_MOVE2:
            return mon->moves[1];
        case MON_DATA_MOVE3:
            return mon->moves[2];
        case MON_DATA_MOVE4:
            return mon->moves[3];
        case MON_DATA_OT_ID:
            return READ_BIG_ENDIAN_16(mon->otId);
        case MON_DATA_EXP:
            return (((mon->experience[0] << 16) | (mon->experience[1] << 8) | mon->experience[2]));
        case MON_DATA_HP_EV:
            return READ_BIG_ENDIAN_16(mon->hpEV);
        case MON_DATA_ATK_EV:
            return READ_BIG_ENDIAN_16(mon->attackEV);
        case MON_DATA_DEF_EV:
            return READ_BIG_ENDIAN_16(mon->defenseEV);
        case MON_DATA_SPEED_EV:
            return READ_BIG_ENDIAN_16(mon->speedEV);
        case MON_DATA_SPATK_EV:
        case MON_DATA_SPDEF_EV:
            return READ_BIG_ENDIAN_16(mon->specialEV);
        case MON_DATA_HP_IV:
            return ((QueryGSCBoxMonData(mon, MON_DATA_ATK_IV) & 1) << 3) |
                   ((QueryGSCBoxMonData(mon, MON_DATA_DEF_IV) & 1) << 2) |
                   ((QueryGSCBoxMonData(mon, MON_DATA_SPEED_IV) & 1) << 1) |
                   (QueryGSCBoxMonData(mon, MON_DATA_SPATK_IV) & 1);
        case MON_DATA_ATK_IV:
            return mon->statIVs[0] >> 4;
        case MON_DATA_DEF_IV:
            return mon->statIVs[0] & 0xF;
        case MON_DATA_SPEED_IV:
            return mon->statIVs[1] >> 4;
        case MON_DATA_SPATK_IV:
        case MON_DATA_SPDEF_IV:
            return mon->statIVs[1] & 0xF;
        case MON_DATA_PP1:
            return mon->pp[0] & 0x3F;
        case MON_DATA_PP2:
            return mon->pp[1] & 0x3F;
        case MON_DATA_PP3:
            return mon->pp[2] & 0x3F;
        case MON_DATA_PP4:
            return mon->pp[3] & 0x3F;
        case MON_DATA_PP_BONUSES:
            return ((mon->pp[0] >> 6) & 0x3) |
             (((mon->pp[1] >> 6) & 0x3) << 2) |
             (((mon->pp[2] >> 6) & 0x3) << 4) |
             (((mon->pp[3] >> 6) & 0x3) << 6);
        case MON_DATA_FRIENDSHIP:
            return mon->friendship;
        case MON_DATA_POKERUS:
            return mon->pokerus;
        case MON_DATA_LEVEL:
            return mon->level;
    }
}

static u8 GetGSCBoxMonGender(struct BoxPokemonGSC * mon) {
    u8 species = QueryGSCBoxMonData(mon, MON_DATA_SPECIES);
    u8 vals;
    switch (gSpeciesInfo[species].genderRatio)
    {
        case MON_MALE:
        case MON_FEMALE:
        case MON_GENDERLESS:
            return gSpeciesInfo[species].genderRatio;
    }
    vals = (QueryGSCBoxMonData(mon, MON_DATA_ATK_IV) << 4) | QueryGSCBoxMonData(mon, MON_DATA_SPEED_IV);
    if (vals > gSpeciesInfo[species].genderRatio) 
        return MON_MALE;
    else 
        return MON_FEMALE;
}

static bool8 IsGSCBoxMonShiny(struct BoxPokemonGSC * mon) {
    u8 atkIV, defIV, speIV, spcIV;
    atkIV = QueryGSCBoxMonData(mon, MON_DATA_ATK_IV);
    defIV = QueryGSCBoxMonData(mon, MON_DATA_DEF_IV);
    speIV = QueryGSCBoxMonData(mon, MON_DATA_SPEED_IV);
    spcIV = QueryGSCBoxMonData(mon, MON_DATA_SPATK_IV);
    return ((atkIV & 2) != 0) && defIV == 10 && speIV == 10 && spcIV == 10;
}

static u8 GetGSCBoxMonUnownLetter(struct BoxPokemonGSC * mon) {
    u8 atkIV, defIV, speIV, spcIV, vals;
    atkIV = QueryGSCBoxMonData(mon, MON_DATA_ATK_IV);
    defIV = QueryGSCBoxMonData(mon, MON_DATA_DEF_IV);
    speIV = QueryGSCBoxMonData(mon, MON_DATA_SPEED_IV);
    spcIV = QueryGSCBoxMonData(mon, MON_DATA_SPATK_IV);
    vals = (((atkIV >> 1) & 3) << 6) |
           (((defIV >> 1) & 3) << 4) | 
           (((speIV >> 1) & 3) << 2) | 
           ((spcIV >> 1) & 3);
    return vals / 10;
}

static u8 GetGSCBoxMonNature(struct BoxPokemonGSC * mon) {
    u32 exp = QueryGSCBoxMonData(mon, MON_DATA_EXP);
    return Mod25Fast(exp);
}

static u8 GetHiddenPowerType(struct BoxPokemonGSC * mon) {
    u8 atkIV, defIV;
    atkIV = QueryGSCBoxMonData(mon, MON_DATA_ATK_IV);
    defIV = QueryGSCBoxMonData(mon, MON_DATA_DEF_IV);
    return ((atkIV & 3) << 2) | (defIV & 0x3);
}

static void ConvertDVtoIV(struct BoxPokemonGSC * mon, u8 * result) {
    u8 ivLsb = CalcIVLsb(GetHiddenPowerType(mon));
    result[0] = (QueryGSCBoxMonData(mon, MON_DATA_HP_IV) << 1) | (ivLsb & 1);
    result[1] = (QueryGSCBoxMonData(mon, MON_DATA_ATK_IV) << 1) | ((ivLsb >> 1) & 1);
    result[2] = (QueryGSCBoxMonData(mon, MON_DATA_DEF_IV) << 1) | ((ivLsb >> 2) & 1);
    result[3] = (QueryGSCBoxMonData(mon, MON_DATA_SPEED_IV) << 1) | ((ivLsb >> 3) & 1);
    result[4] = (QueryGSCBoxMonData(mon, MON_DATA_SPATK_IV) << 1) | ((ivLsb >> 4) & 1);
    result[5] = (QueryGSCBoxMonData(mon, MON_DATA_SPDEF_IV) << 1) | ((ivLsb >> 5) & 1);
}

static bool8 LegalityCheck(u8 species, u8 * moves, u8 level, struct LegalityCheckResult * legalityCheckResult) {
    bool8 result, maskMatched;
    u8 moveCheckResults[MAX_MON_MOVES] = {MOVE_SOURCE_NONE};
    u32 i, j, num;
    s32 index, indices[MAX_MON_MOVES];
    u8 speciesAlt, move, len;
    const u8 *movePtr;
    const u8 moveCombineMasks[] = {0xF, 0x7, 0xB, 0xD, 0xE, 0x3, 0x5, 0x9, 0x6, 0xA, 0xC};
    u16 breedingMasks[16], mask;
    result = TRUE;

    legalityCheckResult->minimumLevel = gMinimum_levels[species];
    if (level < legalityCheckResult->minimumLevel) {
        level = legalityCheckResult->minimumLevel;
        result = FALSE;
    }
    else if (level > MAX_LEVEL) {
        level = MAX_LEVEL;
        result = FALSE;
    }

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (moves[i] == MOVE_NONE) continue;
        for (j = i + 1; j < MAX_MON_MOVES; j++) {
            if (moves[i] == moves[j])
                moveCheckResults[j] = MOVE_VALUE_REPEATED;
        }
    }

    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = moves[i];
        if (move == MOVE_NONE || moveCheckResults[i] != MOVE_SOURCE_NONE)
            continue;
        if (species == SPECIES_SMEARGLE) {
            moveCheckResults[i] = MOVE_SOURCE_FIXED;
            continue;
        }
        speciesAlt = species;
        while (speciesAlt != SPECIES_NONE) {
            if (gBase_fixed_moves_table[speciesAlt] != 0xFF) {
                movePtr = &gBase_fixed_moves_table[0x100 + gBase_fixed_moves_table[speciesAlt] * 0x20];
                if ((movePtr[move / 8] & (1 << (move % 8))) != 0) {
                    moveCheckResults[i] = MOVE_SOURCE_FIXED;
                    break;
                }
            }
            if (gExternal_fixed_moves_table[speciesAlt] != 0xFF) {
                movePtr = &gExternal_fixed_moves_table[0x100 + gExternal_fixed_moves_table[speciesAlt] * 4];
                len = *movePtr++;
                index = BinarySearch(movePtr, len, move);
                if (index >= 0) {
                    moveCheckResults[i] = MOVE_SOURCE_FIXED;
                    break;
                }
            }
            if (gLevel_up_moves_table[speciesAlt] != 0xFF) {
                movePtr = &gLevel_up_moves_table[0x100 + gLevel_up_moves_table[speciesAlt] * 2];
                while (*movePtr != 0xFF) {
                    if (move == movePtr[0] && level >= movePtr[1]) {
                        moveCheckResults[i] = MOVE_SOURCE_LEVELUP;
                        break;
                    }
                    movePtr += 2;
                }
                if (*movePtr != 0xFF)
                    break;
            }
            if (gBreeding_moves_table[speciesAlt] != 0xFF) {
                movePtr = &gBreeding_moves_table[0x100 + gBreeding_moves_table[speciesAlt] * 2];
                len = *movePtr++;
                index = BinarySearch(movePtr, len, move);
                if (index >= 0) {
                    moveCheckResults[i] = MOVE_SOURCE_BREEDING;
                    break;
                }
            }
            speciesAlt = gEvo_reverse_table[speciesAlt];
        }
    }

    num = 0;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (moveCheckResults[i] == MOVE_SOURCE_BREEDING)
            num++;
    }

    if (num >= 2) {
        movePtr = &gBreeding_moves_table[0x100 + gBreeding_moves_table[species] * 2];
        len = *movePtr++;
        for (i = 0; i < MAX_MON_MOVES; i++) {
            if (moveCheckResults[i] == MOVE_SOURCE_BREEDING) {
                indices[i] = BinarySearch(movePtr, len, moves[i]);
            }
            else
                indices[i] = -1;
        }
        movePtr = &gBreeding_shared_table[0x100 + gBreeding_shared_table[species]];
        len = *movePtr++;
        for (i = 0; i < len; i++) {
            if (*movePtr >= 0x80) {
                // encoding as leb-128, only Kangaskhan uses it, though.
                breedingMasks[i] = ((movePtr[0] & 0x7F) << 7) | movePtr[1];
                movePtr += 2;
            }
            else
                breedingMasks[i] = *movePtr++;
        }
        if (num == 4)
            i = 0;
        else if (num == 3)
            i = 1;
        else if (num == 2)
            i = 5;
        maskMatched = FALSE;
        for (; i < sizeof(moveCombineMasks); i++) {
            for (j = 0, mask = 0; j < MAX_MON_MOVES; j++) {
                if ((moveCombineMasks[i] & (1 << j)) != 0) {
                    if (moveCheckResults[j] == MOVE_SOURCE_BREEDING)
                        mask = mask | (1 << indices[j]);
                    else {
                        mask = 0;
                        break;
                    }
                }
            }
            if (mask != 0) {
                for (j = 0; j < len; j++) {
                    if ((breedingMasks[j] & mask) == mask) {
                        maskMatched = TRUE;
                        break;
                    }
                }
                if (maskMatched)
                    break;
            }
        }
        if (maskMatched) {
            for (j = 0; j < MAX_MON_MOVES; j++) {
                if ((moveCombineMasks[i] & (1 << j)) == 0) {
                    if (moveCheckResults[j] == MOVE_SOURCE_BREEDING)
                        moveCheckResults[j] = MOVE_SOURCE_NONE;
                }
            }
        } 
        else {
            for (i = 0; i < MAX_MON_MOVES; i++)
                if (moveCheckResults[i] == MOVE_SOURCE_BREEDING)
                    break;
            for (j = 0; j < MAX_MON_MOVES; j++) {
                if (j != i && moveCheckResults[j] == MOVE_SOURCE_BREEDING)
                    moveCheckResults[j] = MOVE_SOURCE_NONE;
            }
        }
    }

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (moves[i] == MOVE_NONE) continue;
        result = result && moveCheckResults[i] != MOVE_SOURCE_NONE && moveCheckResults[i] != MOVE_VALUE_REPEATED;
    }

    legalityCheckResult->moveLegalFlags = 0;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (moves[i] == MOVE_NONE || 
            (moveCheckResults[i] != MOVE_SOURCE_NONE && 
            moveCheckResults[i] != MOVE_VALUE_REPEATED))
            legalityCheckResult->moveLegalFlags = legalityCheckResult->moveLegalFlags | (1 << i);
    }

    return result;
}

static void DetectGameVerAndLanguage(const u8 *nickName, const u8 *otName, u8 *gameVer, u8 *language) {
    u8 origGameVer, origLanguage, namePos;
    if (*language == LANGUAGE_JAPANESE)
        return;
    while (otName[namePos] != 0x50) namePos++;
    if (namePos > 7) {
        *gameVer = VERSION_GOLD;
        *language = LANGUAGE_KOREAN;
        return;
    }
    origGameVer = *gameVer;
    origLanguage = *language;
    if (origGameVer == VERSION_CRYSTAL && origLanguage == LANGUAGE_CHINESE) {
        if (!ContainsCrystalCHSChars(nickName)) {
            if (ContainsGSCHSChars(nickName)) {
                *gameVer = VERSION_GOLD;
                *language = LANGUAGE_KOREAN;
                return;
            }
        }
        if (!ContainsCrystalCHSChars(otName)) {
            if (ContainsGSCHSChars(otName)) {
                *gameVer = VERSION_GOLD;
                *language = LANGUAGE_KOREAN;
                return;
            }
        }
        return;
    }

    if ((origGameVer == VERSION_GOLD || origGameVer == VERSION_SILVER) && origLanguage == LANGUAGE_KOREAN) {
        if (!ContainsGSCHSChars(nickName)) {
            if (ContainsCrystalCHSChars(nickName)) {
                *gameVer = VERSION_CRYSTAL;
                *language = LANGUAGE_CHINESE;
                return;
            }
        }
        if (!ContainsGSCHSChars(otName)) {
            if (ContainsCrystalCHSChars(otName)) {
                *gameVer = VERSION_CRYSTAL;
                *language = LANGUAGE_CHINESE;
                return;
            }
        }
        return;
    }

    if (ContainsCrystalCHSChars(nickName) || ContainsCrystalCHSChars(otName)) {
        *gameVer = VERSION_CRYSTAL;
        *language = LANGUAGE_CHINESE;
        return;
    }

    if (ContainsGSCHSChars(nickName) || ContainsGSCHSChars(otName)) {
        *gameVer = VERSION_GOLD;
        *language = LANGUAGE_KOREAN;
        return;
    }

    namePos = 0;
    while (nickName[namePos] != 0x50) {
        if (nickName[namePos] >= 0xC0 && nickName[namePos] <= 0xC5) {
            *language = LANGUAGE_GERMAN;
            return;
        }
        namePos++;
    }

    namePos = 0;
    while (otName[namePos] != 0x50) {
        if (otName[namePos] >= 0xC0 && otName[namePos] <= 0xC5) {
            *language = LANGUAGE_GERMAN;
            return;
        }
        namePos++;
    }
}

int FindIndexIfGSCBoxMonIsEggless(struct BoxPokemonGSC *boxMonGSC) {
    u8 species = QueryGSCBoxMonData(boxMonGSC, MON_DATA_SPECIES);
    int i;
    for (i = 0; i < ARRAY_COUNT(sEggLessPokemonTemplates); i++) {
        if (sEggLessPokemonTemplates[i]->species == species) {
            return i;
        }
    }
    return -1;
}

bool8 ConvertEgglessBoxMonFromGSC(struct BoxPokemon *boxMon, struct BoxPokemonGSC *boxMonGSC, u8 * origNickName, u8 * origOtName, u8 otGender, u8 gameVer, u8 language, struct LegalityCheckResult * legalityCheckResult) {
    u8 level, abilityNum, moves[MAX_MON_MOVES], *movePtr, ppBonuses, ppBonusBuff[MAX_MON_MOVES], *ppBonusPtr, 
       gender, friendship, pokerus, metGameVer, iv, nameLanguage,
       nickName[POKEMON_NAME_LENGTH + 1], otName[PLAYER_NAME_LENGTH + 1];
    u16 move;
    u32 pid;
    s32 index = FindIndexIfGSCBoxMonIsEggless(boxMonGSC);
    bool32 isModernFatefulEncounter = TRUE;
    const struct EggLessPokemonTemplate *template;
    struct PidGenerationParam param;
    int i;
    u32 calcPidBuff[320 / sizeof(u32)];
    u32 (*CalcPID)(struct PidGenerationParam *) = (u32 (*)(struct PidGenerationParam *))calcPidBuff;
    static const u8 zeroValue = 0;
    static const u8 fixedBall = ITEM_POKE_BALL;
    static const u8 nationalRibbon = 1;
    if (index < 0)
        return FALSE;
    template = sEggLessPokemonTemplates[index];
    if ((template->metGameVer & F_VERSION_GAMECUBE)) 
        memcpy(calcPidBuff, CalcPersonalityIdGameCube, sizeof(calcPidBuff));
    else if (template->species == SPECIES_UNOWN) 
        memcpy(calcPidBuff, CalcPersonalityIdGBAUnown, sizeof(calcPidBuff));
    else
        memcpy(calcPidBuff, CalcPersonalityIdGBAUsual, sizeof(calcPidBuff));
    param.rngValue = gRngValue;
    param.isShiny = template->species == SPECIES_CELEBI ? FALSE : IsGSCBoxMonShiny(boxMonGSC);
    param.otIdLo = template->extTemplate != NULL ? template->extTemplate->otId : QueryGSCBoxMonData(boxMonGSC, MON_DATA_OT_ID);
    param.otIdHi = 0;
    param.nature = GetGSCBoxMonNature(boxMonGSC);
    param.genderThreshold = gSpeciesInfo[template->species].genderRatio;
    gender = GetGSCBoxMonGender(boxMonGSC);
    param.isMaleOrGenderLess = gender == MON_MALE || gender == MON_GENDERLESS;
    param.unownLetter = GetGSCBoxMonUnownLetter(boxMonGSC);
    friendship = QueryGSCBoxMonData(boxMonGSC, MON_DATA_FRIENDSHIP);
    pokerus = QueryGSCBoxMonData(boxMonGSC, MON_DATA_POKERUS);
    ppBonuses = QueryGSCBoxMonData(boxMonGSC, MON_DATA_PP_BONUSES);
    // pid
    pid = CalcPID(&param);
    gRngValue = param.rngValue;
    level = QueryGSCBoxMonData(boxMonGSC, MON_DATA_LEVEL);
    // level
    if (level > MAX_LEVEL)
        level = MAX_LEVEL;
    else if (level < MIN_LEVEL)
        level = MIN_LEVEL;
    CreateBoxMon(boxMon, template->species, level, USE_RANDOM_IVS, TRUE, pid, OT_ID_PRESET, (param.otIdHi << 16) | param.otIdLo);

    // pokeball
    SetBoxMonData(boxMon, MON_DATA_POKEBALL, &fixedBall);

    // gender
    SetBoxMonData(boxMon, MON_DATA_OT_GENDER, &otGender);

    DetectGameVerAndLanguage(origNickName, origOtName, &gameVer, &language);
    nameLanguage = language;
    if (language == LANGUAGE_KOREAN)
        nameLanguage = LANGUAGE_CHINESE;
    if (template->metLanguage == F_LANGUAGE_JAPANESE)
        nameLanguage = LANGUAGE_JAPANESE;
    // nick name
    if (template->extTemplate == NULL || (template->extTemplate != NULL && template->extTemplate->nickName == NULL)) {
        DecodeGSCString(nickName, sizeof(nickName), origNickName, (language == LANGUAGE_JAPANESE ? NAME_LENGTH_GSC_JP : NAME_LENGTH_GSC_INTL) + 1, gameVer, language);
        legalityCheckResult->nameInvalidFlags = IdentifyInvalidNameChars(nickName, nameLanguage == LANGUAGE_JAPANESE ? POKEMON_NAME_LENGTH_JPN + 1 : sizeof(nickName), nameLanguage, origNickName, gameVer, language);
    }
    else {
        StringCopy(nickName, template->extTemplate->nickName);
    }
    PaddingName(nickName, nameLanguage, NAME_TYPE_POKEMON);
    SetBoxMonData(boxMon, MON_DATA_NICKNAME, nickName);

    // ot name 
    if (template->extTemplate == NULL || (template->extTemplate != NULL && template->extTemplate->otName == NULL)) {
        DecodeGSCString(otName, sizeof(otName), origOtName, (language == LANGUAGE_JAPANESE ? NAME_LENGTH_GSC_JP : NAME_LENGTH_GSC_INTL) + 1, gameVer, language);
        IdentifyInvalidNameChars(otName, nameLanguage == LANGUAGE_JAPANESE ? PLAYER_NAME_LENGTH_JPN + 1 : sizeof(otName), nameLanguage, origOtName, gameVer, language);
    }
    else {
        StringCopy(otName, template->extTemplate->otName);
    }
    PaddingName(otName, nameLanguage, NAME_TYPE_TRAINER);
    SetBoxMonData(boxMon, MON_DATA_OT_NAME, otName);

    // met data
    SetBoxMonData(boxMon, MON_DATA_MET_LEVEL, &template->metLevel);
    if (template->species == SPECIES_UNOWN) 
        SetBoxMonData(boxMon, MON_DATA_MET_LOCATION, &unownMetLocation[GET_UNOWN_LETTER(pid)]);
    else
        SetBoxMonData(boxMon, MON_DATA_MET_LOCATION, &template->metLocation);
    metGameVer = gGameVersion;
    if (template->metGameVer == F_VERSION_GAMECUBE) {
        metGameVer = VERSION_GAMECUBE;
        SetBoxMonData(boxMon, MON_DATA_MET_GAME, &metGameVer);
        SetBoxMonData(boxMon, MON_DATA_NATIONAL_RIBBON, &nationalRibbon);
    }
    else if ((template->metGameVer & (1 << gGameVersion)) == 0) {
        while ((template->metGameVer & (1 << metGameVer)) == 0) {
            metGameVer = Random() & 7;
        }
        SetBoxMonData(boxMon, MON_DATA_MET_GAME, &metGameVer);
    }

    if (template->metLanguage == F_LANGUAGE_JAPANESE) {
        language = LANGUAGE_JAPANESE;
        SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &language);
    }
    else {
        if (language == LANGUAGE_CHINESE || language == LANGUAGE_KOREAN)
            language = LANGUAGE_ENGLISH;
        SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &language);
    }

    // skill
    memset(moves, MOVE_NONE, sizeof(moves));
    memset(ppBonusBuff, 0, sizeof(ppBonusBuff));
    movePtr = moves;
    ppBonusPtr = ppBonusBuff;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = QueryGSCBoxMonData(boxMonGSC, MON_DATA_MOVE1 + i);
        if (move > MOVE_NONE && move <= MOVE_COUNT_GSC) {
            *movePtr++ = move;
            *ppBonusPtr++ = (ppBonuses >> (i * 2)) & 0x3;
        }
    }

    ppBonuses = 0;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = moves[i];
        SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &move);
        SetBoxMonData(boxMon, MON_DATA_PP1 + i, &zeroValue);
        ppBonuses = ppBonuses | (ppBonusBuff[i] << (i * 2));
    }

    SetBoxMonData(boxMon, MON_DATA_PP_BONUSES, &ppBonuses);
    BoxMonRestorePP(boxMon);

    // IVs
    iv = param.iv[0] & 0x1F;
    SetBoxMonData(boxMon, MON_DATA_HP_IV, &iv);
    iv = (param.iv[0] >> 5) & 0x1F;
    SetBoxMonData(boxMon, MON_DATA_ATK_IV, &iv);
    iv = (param.iv[0] >> 10) & 0x1F;
    SetBoxMonData(boxMon, MON_DATA_DEF_IV, &iv);
    iv = param.iv[1] & 0x1F;
    SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &iv);
    iv = (param.iv[1] >> 5) & 0x1F;
    SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &iv);
    iv = (param.iv[1] >> 10) & 0x1F;
    SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &iv);

    // friendship
    SetBoxMonData(boxMon, MON_DATA_FRIENDSHIP, &friendship);

    // pokerus
    SetBoxMonData(boxMon, MON_DATA_POKERUS, &pokerus);

    if (template->metGameVer & F_VERSION_GAMECUBE) {
        if (gSpeciesInfo[template->species].abilities[1] != ABILITY_NONE) {
            SetBoxMonData(boxMon, MON_DATA_ABILITY_NUM, &param.abilityNum);
        }
    }

    // MEW LUGIA HO-OH
    if (template->species == SPECIES_MEW || template->species == SPECIES_LUGIA || template->species == SPECIES_HO_OH) {
        SetBoxMonData(boxMon, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &isModernFatefulEncounter);
    }

    return LegalityCheck(template->species, moves, level, legalityCheckResult);
}

bool8 ConvertBoxMonFromGSC(struct BoxPokemon *boxMon, struct BoxPokemonGSC *boxMonGSC, u8 * origNickName, u8 * origOtName, u8 otGender, u8 gameVer, u8 language, struct LegalityCheckResult * legalityCheckResult)
{
    u8 species, level, moves[MAX_MON_MOVES], *movePtr, 
    ppBonuses, ppBonusBuff[MAX_MON_MOVES], *ppBonusPtr, gender, friendship, pokerus, nameLanguage, 
    nickName[POKEMON_NAME_LENGTH + 1], otName[PLAYER_NAME_LENGTH + 1], ivs[6];
    u16 move;
    u32 pid;
    s32 i;
    struct PidGenerationParam param;
    u32 calcPidBuff[320 / sizeof(u32)];
    u32 (*CalcPID)(struct PidGenerationParam *) = (u32 (*)(struct PidGenerationParam *))calcPidBuff;
    static const u8 zeroValue = 0;
    static const u8 fixedBall = ITEM_POKE_BALL;
    memcpy(calcPidBuff, CalcPersonalityIdGBAUsual, sizeof(calcPidBuff));
    species = QueryGSCBoxMonData(boxMonGSC, MON_DATA_SPECIES),
    level = QueryGSCBoxMonData(boxMonGSC, MON_DATA_LEVEL);
    friendship = QueryGSCBoxMonData(boxMonGSC, MON_DATA_FRIENDSHIP);
    pokerus = QueryGSCBoxMonData(boxMonGSC, MON_DATA_POKERUS);
    ppBonuses = QueryGSCBoxMonData(boxMonGSC, MON_DATA_PP_BONUSES);
    param.rngValue = gRngValue;
    param.genderThreshold = gSpeciesInfo[species].genderRatio;
    param.nature = GetGSCBoxMonNature(boxMonGSC);
    param.isShiny = IsGSCBoxMonShiny(boxMonGSC);
    gender = GetGSCBoxMonGender(boxMonGSC);
    param.isMaleOrGenderLess = gender == MON_MALE || gender == MON_GENDERLESS;
    param.otIdLo = QueryGSCBoxMonData(boxMonGSC, MON_DATA_OT_ID);
    param.otIdHi = 0;
    pid = CalcPID(&param);
    gRngValue = param.rngValue;
    if (level > MAX_LEVEL)
        level = MAX_LEVEL;
    else if (level < MIN_LEVEL)
        level = MIN_LEVEL;
    DetectGameVerAndLanguage(origNickName, origOtName, &gameVer, &language);
    nameLanguage = language;
    if (nameLanguage == LANGUAGE_KOREAN)
        nameLanguage = LANGUAGE_CHINESE;

    CreateBoxMon(boxMon, species, level, USE_RANDOM_IVS, TRUE, pid, OT_ID_PRESET, (param.otIdHi << 16) | param.otIdLo);

    SetBoxMonData(boxMon, MON_DATA_POKEBALL, &fixedBall);

    SetBoxMonData(boxMon, MON_DATA_OT_GENDER, &otGender);
    DecodeGSCString(nickName, sizeof(nickName), origNickName, (language == LANGUAGE_JAPANESE ? NAME_LENGTH_GSC_JP : NAME_LENGTH_GSC_INTL) + 1, gameVer, language);
    legalityCheckResult->nameInvalidFlags = IdentifyInvalidNameChars(nickName, (nameLanguage == LANGUAGE_JAPANESE ? POKEMON_NAME_LENGTH_JPN + 1 : sizeof(nickName)), nameLanguage, origNickName, gameVer, language);
    PaddingName(nickName, nameLanguage, NAME_TYPE_POKEMON);
    SetBoxMonData(boxMon, MON_DATA_NICKNAME, nickName);
    DecodeGSCString(otName, sizeof(otName), origOtName, (language == LANGUAGE_JAPANESE ? NAME_LENGTH_GSC_JP : NAME_LENGTH_GSC_INTL) + 1, gameVer, language);
    IdentifyInvalidNameChars(otName, (nameLanguage == LANGUAGE_JAPANESE ? PLAYER_NAME_LENGTH_JPN + 1 : sizeof(otName)), nameLanguage, origOtName, gameVer, language);
    PaddingName(otName, nameLanguage, NAME_TYPE_TRAINER);
    SetBoxMonData(boxMon, MON_DATA_OT_NAME, otName);
    SetBoxMonData(boxMon, MON_DATA_MET_LEVEL, &zeroValue);
    if (language == LANGUAGE_CHINESE || language == LANGUAGE_KOREAN)
        language = LANGUAGE_ENGLISH;
    SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &language);

    memset(moves, MOVE_NONE, sizeof(moves));
    memset(ppBonusBuff, 0, sizeof(ppBonusBuff));
    movePtr = moves;
    ppBonusPtr = ppBonusBuff;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = QueryGSCBoxMonData(boxMonGSC, MON_DATA_MOVE1 + i);
        if (move > MOVE_NONE && move <= MOVE_COUNT_GSC) {
            *movePtr++ = (u8)move;
            *ppBonusPtr++ = (ppBonuses >> (i * 2)) & 0x3;
        }
    }

    ppBonuses = 0;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = moves[i];
        SetBoxMonData(boxMon, MON_DATA_MOVE1 + i, &move);
        SetBoxMonData(boxMon, MON_DATA_PP1 + i, &zeroValue);
        ppBonuses = ppBonuses | (ppBonusBuff[i] << (i * 2));
    }

    SetBoxMonData(boxMon, MON_DATA_PP_BONUSES, &ppBonuses);
    BoxMonRestorePP(boxMon);

    SetBoxMonData(boxMon, MON_DATA_FRIENDSHIP, &friendship);
    SetBoxMonData(boxMon, MON_DATA_POKERUS, &pokerus);

    ConvertDVtoIV(boxMonGSC, ivs);
    for (i = 0; i < sizeof(ivs); i++)
        SetBoxMonData(boxMon, MON_DATA_HP_IV + i, &ivs[i]);

    return LegalityCheck(species, moves, level, legalityCheckResult);
}

void MakeGSCBoxMonLegal(struct BoxPokemon *gscBoxMon, struct LegalityCheckResult *legalityCheckResult) {
    bool8 hasNoMove;
    u16 moves[MAX_MON_MOVES] = {MOVE_NONE};
    u8 level, ppBonuses, ppBonusBuff[MAX_MON_MOVES], *ppBonusPtr;
    s32 i;
    u16 species, *movePtr;
    static const u8 zeroValue = 0;
    species = GetBoxMonData(gscBoxMon, MON_DATA_SPECIES, NULL);
    ppBonuses = GetBoxMonData(gscBoxMon, MON_DATA_PP_BONUSES, NULL);
    level = GetLevelFromBoxMonExp(gscBoxMon);

    if (level < legalityCheckResult->minimumLevel) {
        level = legalityCheckResult->minimumLevel;
        SetBoxMonData(gscBoxMon, MON_DATA_EXP, &gExperienceTables[gSpeciesInfo[species].growthRate][level]);
    }

    memset(ppBonusBuff, 0, sizeof(ppBonusBuff));
    movePtr = moves;
    ppBonusPtr = ppBonusBuff;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        if ((legalityCheckResult->moveLegalFlags & (1 << i)) != 0) {
            *movePtr++ = GetBoxMonData(gscBoxMon, MON_DATA_MOVE1 + i, NULL);
            *ppBonusPtr++ = (ppBonuses >> (i * 2)) & 0x3;
        }
    }

    hasNoMove = TRUE;
    ppBonuses = 0;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        hasNoMove = hasNoMove && moves[i] == MOVE_NONE;
        SetBoxMonData(gscBoxMon, MON_DATA_MOVE1 + i, &moves[i]);
        SetBoxMonData(gscBoxMon, MON_DATA_PP1 + i, &zeroValue);
        ppBonuses = ppBonuses | (ppBonusBuff[i] << (i * 2));
    }

    if (hasNoMove) 
        GiveBoxMonInitialMoveset(gscBoxMon);
    else 
        SetBoxMonData(gscBoxMon, MON_DATA_PP_BONUSES, &ppBonuses);

    BoxMonRestorePP(gscBoxMon);
}