#ifndef GUARD_MATH_FAST_H
#define GUARD_MATH_FAST_H

struct PidGenerationParam {
    u32 rngValue;
    u8 nature;
    u8 genderThreshold;
    u8 unownLetter;
    bool8 isShiny;
    u16 otIdLo;
    u16 otIdHi;
    u16 iv[2];
    u8 abilityNum;
    bool8 isMaleOrGenderLess;
};

u32 Mod25Fast(u32 val);
void ConvertFromBase226ToBase247Customized(u8 * hiPtr, u8 * loPtr);
u32 CalcPersonalityIdGBAUsual(struct PidGenerationParam *param);
u32 CalcPersonalityIdGBAUnown(struct PidGenerationParam *param);
u32 CalcPersonalityIdGameCube(struct PidGenerationParam *param);
u8 CalcIVLsb(u8 hiddenPowerType);

#endif