#include "global.h"
#include "math_fast.h"
#include "random.h"

#define GAMECUBE_RANDOMIZE(val) (val * 0x343FD + 0x269EC3)

static const u8 UnonwLetterTable[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 0, 1, 2, 3,
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 0, 1, 2, 3,
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 0, 1, 2, 3};

u32 Mod25Fast(u32 val) {
    return val % 25;
}

// used for Pokémon Crystal Chinese encoding conversion 
void ConvertFromBase226ToBase247Customized(u8 * hiPtr, u8 * loPtr) {
    u8 hi = *hiPtr, lo = *loPtr;
    u16 seq = hi * 226 + lo;
    if (seq >= 3760)
        seq -= 5;
    hi = seq / 247;
    lo = seq % 247;
    *hiPtr = hi;
    *loPtr = lo;
}

// you should copy the following functions to stack (IWRAM), before you call them.

u32 CalcPersonalityIdGBAUsual(struct PidGenerationParam * param) {
    register u32 rngValue = param->rngValue;
    register u16 pidHi, pidLo;
    register u32 pid;
    register bool8 isMatched = FALSE;
    register u32 otIdXORed = param->otIdHi ^ param->otIdLo;
    register u32 nature = param->nature;
    register bool8 isShiny = param->isShiny;
    register s32 genderThreshold = param->genderThreshold;
    register bool8 isMaleOrGenderLess = param->isMaleOrGenderLess;
    if (param->genderThreshold == MON_FEMALE)
        genderThreshold = 0x100;
    if (param->genderThreshold == MON_GENDERLESS)
        genderThreshold = -1;

    rngValue = ISO_RANDOMIZE1(rngValue);
    pidHi = rngValue >> 16;
    while (!isMatched) {
        pidLo = pidHi;
        rngValue = ISO_RANDOMIZE1(rngValue);
        pidHi = rngValue >> 16;
        pid = (pidHi << 16) | pidLo;
        isMatched = ((otIdXORed ^ pidLo ^ pidHi) < 8) == isShiny &&
                    pid % 25 == nature &&
                    ((s32)(pid & 0xFF) >= genderThreshold) == isMaleOrGenderLess;
    }
    rngValue = ISO_RANDOMIZE1(rngValue);
    param->iv[0] = rngValue >> 16;
    rngValue = ISO_RANDOMIZE1(rngValue);
    param->iv[1] = rngValue >> 16;
    param->rngValue = rngValue;
    return pid;
}

u32 CalcPersonalityIdGBAUnown(struct PidGenerationParam * param) {
    register u32 rngValue = param->rngValue;
    register u16 pidHi, pidLo;
    register u32 pid;
    register bool8 isMatched = FALSE;
    register u32 otIdXORed = param->otIdHi ^ param->otIdLo;
    register u32 shinyValue;
    register u32 nature = param->nature;
    register bool8 isShiny = param->isShiny;
    register u32 unownLetter = param->unownLetter;

    rngValue = ISO_RANDOMIZE1(rngValue);
    pidLo = rngValue >> 16;
    while (!isMatched) {
        pidHi = pidLo;
        rngValue = ISO_RANDOMIZE1(rngValue);
        pidLo = rngValue >> 16;
        pid = (pidHi << 16) | pidLo;
        shinyValue = otIdXORed ^ pidHi ^ pidLo;
        isMatched = UnonwLetterTable[(((pid & 0x03000000) >> 18) |
                                      ((pid & 0x00030000) >> 12) |
                                      ((pid & 0x00000300) >> 6) |
                                      ((pid & 0x00000003) >> 0))] == unownLetter &&
                    pid % 25 == nature && 
                    (isShiny || shinyValue >= 8);
    }
    if (isShiny)
        param->otIdHi = shinyValue & ~7;
    rngValue = ISO_RANDOMIZE1(rngValue);
    param->iv[0] = rngValue >> 16;
    rngValue = ISO_RANDOMIZE1(rngValue);
    param->iv[1] = rngValue >> 16;
    param->rngValue = rngValue;
    return pid;
}

u32 CalcPersonalityIdGameCube(struct PidGenerationParam * param) {
    register u32 rngValue = param->rngValue;
    register u32 pid;
    register bool8 isMatched = FALSE;
    register u32 otIdXORed = param->otIdHi ^ param->otIdLo;
    register u32 nature = param->nature;
    register bool8 isShiny = param->isShiny;

    u16 pids[5];
    s32 i;
    for (i = 1; i < ARRAY_COUNT(pids); i++) {
        rngValue = GAMECUBE_RANDOMIZE(rngValue);
        pids[i] = rngValue >> 16;
    }

    while (!isMatched)
    {
        pids[0] = pids[1];
        pids[1] = pids[2];
        pids[2] = pids[3];
        pids[3] = pids[4];
        rngValue = GAMECUBE_RANDOMIZE(rngValue);
        pids[4] = rngValue >> 16;
        pid = (pids[3] << 16) | pids[4];
        isMatched = ((otIdXORed ^ pids[3] ^ pids[4]) < 8) == isShiny &&
                    pid % 25 == nature;
    }

    param->iv[0] = pids[0];
    param->iv[1] = pids[1];
    param->abilityNum = pids[2] & 1;
    param->rngValue = rngValue;
    return pid;
}

u8 CalcIVLsb(u8 hiddenPowerType) {
    u8 result;
    if (hiddenPowerType == 15)
        return 63;
    do
    {
        result = Random() & 0x3F;
    } while (hiddenPowerType != result * 15 / 63);
    return result;
}