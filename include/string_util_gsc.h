#ifndef GUARD_STRING_UTIL_GSC
#define GUARD_STRING_UTIL_GSC

#define NAME_TYPE_POKEMON 0
#define NAME_TYPE_TRAINER 1

bool8 ContainsCrystalCHSChars(const u8 *str);
bool8 ContainsGSCHSChars(const u8 *str);
u32 DecodeGSCString(u8 *dest, size_t destLen, const u8 *src, size_t srcLen, u8 gameVer, u8 gameLang);
u16 IdentifyInvalidNameChars(u8 *str, size_t maxLen, u8 strLang, const u8 *origStr, u8 origGameVer, u8 origGameLang);
void HighlightInvalidNameChars(const u8 *src, u8 *dest, u16 nameFlags, u8 strLang);
void PaddingName(u8 *name, u8 nameLanguage, u8 nameType);
#endif