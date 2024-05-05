#include "global.h"
#include "math_fast.h"
#include "pokemon_rom_resource.h"

#define RUBY_GAME_CODE 'VXA'
#define SAPPHIRE_GAME_CODE 'PXA'
#define FIRERED_GAME_CODE 'RPB'
#define LEAFGREAN_GAME_CODE 'GPB'
#define EMERALD_GAME_cODE 'EPB'

#define ROM_GAME_CODE *(u32 *)0x80000AC
#define ROM_FIXED_VALUE *(u32 *)0x80000B2
#define ROM_SOFTWARE_VERSION *(u8 *)0x80000BC

const struct PMRomHeader *gRomHeader;
const struct PMRomResource *gRomResource;

static struct PMRomResource sRomResource;

static const struct PMRomHeader sRubyJPRomHeader =
    {
        2u,
        1u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81BCB60,
        (const struct CompressedSpriteSheet *)0x81BE000,
        (const struct CompressedSpritePalette *)0x81BEDC0,
        (const struct CompressedSpritePalette *)0x81BFB80,
        (const u8 *const *)0x8391A98,
        (const u8 *)0x8392178,
        (const struct SpritePalette *)0x8392330,
        (const u8 (*)[])0x81CA354,
        (const u8 (*)[])0x81CACFC,
        (const struct Decoration *)0x83C2A44,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        5u,
        10u,
        5u,
        7u,
        8u,
        6u,
        7u,
        4u,
        10u,
        18u,
        10u,
        10u,
        5u,
        1u,
        3u,
        7u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x81D09CC,
        (const u8 (*)[])0x81CBC44,
        (const u8 *const *)0x81CBEB4,
        (const struct Item *)0x839A648,
        (const struct BattleMove *)0x81CCEE0,
        (const struct CompressedSpriteSheet *)0x81DC630,
        (const struct CompressedSpritePalette *)0x81DC690,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        137962488u}; // weak

static const struct PMRomHeader sSapphireJPRomHeader =
    {
        1u,
        1u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81BCAF0,
        (const struct CompressedSpriteSheet *)0x81BDF90,
        (const struct CompressedSpritePalette *)0x81BED50,
        (const struct CompressedSpritePalette *)0x81BFB10,
        (const u8 *const *)0x8391A7C,
        (const u8 *)0x839215C,
        (const struct SpritePalette *)0x8392314,
        (const u8 (*)[])0x81CA2E4,
        (const u8 (*)[])0x81CAC8C,
        (const struct Decoration *)0x83C2A28,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        5u,
        10u,
        5u,
        7u,
        8u,
        6u,
        7u,
        4u,
        10u,
        18u,
        10u,
        10u,
        5u,
        1u,
        3u,
        7u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x81D095C,
        (const u8 (*)[])0x81CBBD4,
        (const u8 *const *)0x81CBE44,
        (const struct Item *)0x839A62C,
        (const struct BattleMove *)0x81CCE70,
        (const struct CompressedSpriteSheet *)0x81DC5C0,
        (const struct CompressedSpritePalette *)0x81DC620,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        137962460u}; // weak

static const struct PMRomHeader sRubyUSRomHeaderV0 =
    {
        2u,
        2u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81E8354,
        (const struct CompressedSpriteSheet *)0x81E97F4,
        (const struct CompressedSpritePalette *)0x81EA5B4,
        (const struct CompressedSpritePalette *)0x81EB374,
        (const u8 *const *)0x83BBD20,
        (const u8 *)0x83BC400,
        (const struct SpritePalette *)0x83BC5B8,
        (const u8 (*)[])0x81F716C,
        (const u8 (*)[])0x81F8320,
        (const struct Decoration *)0x83EB6C4,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x81FEC18,
        (const u8 (*)[])0x81FA248,
        (const u8 *const *)0x81FA110,
        (const struct Item *)0x83C5564,
        (const struct BattleMove *)0x81FB12C,
        (const struct CompressedSpriteSheet *)0x820A92C,
        (const struct CompressedSpritePalette *)0x820A98C,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138152408u}; // weak

static const struct PMRomHeader sRubyUSRomHeaderV1 =
    {
        2u,
        2u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81E836C,
        (const struct CompressedSpriteSheet *)0x81E980C,
        (const struct CompressedSpritePalette *)0x81EA5CC,
        (const struct CompressedSpritePalette *)0x81EB38C,
        (const u8 *const *)0x83BBD3C,
        (const u8 *)0x83BC41C,
        (const struct SpritePalette *)0x83BC5D4,
        (const u8 (*)[])0x81F7184,
        (const u8 (*)[])0x81F8338,
        (const struct Decoration *)0x83EB6E0,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x81FEC30,
        (const u8 (*)[])0x81FA260,
        (const u8 *const *)0x81FA128,
        (const struct Item *)0x83C5580,
        (const struct BattleMove *)0x81FB144,
        (const struct CompressedSpriteSheet *)0x820A944,
        (const struct CompressedSpritePalette *)0x820A9A4,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138152436u}; // weak

static const struct PMRomHeader sSapphireUSRomHeaderV0 =
    {
        1u,
        2u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81E82E4,
        (const struct CompressedSpriteSheet *)0x81E9784,
        (const struct CompressedSpritePalette *)0x81EA544,
        (const struct CompressedSpritePalette *)0x81EB304,
        (const u8 *const *)0x83BBD78,
        (const u8 *)0x83BC458,
        (const struct SpritePalette *)0x83BC610,
        (const u8 (*)[])0x81F70FC,
        (const u8 (*)[])0x81F82B0,
        (const struct Decoration *)0x83EB71C,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x81FEBA8,
        (const u8 (*)[])0x81FA1D8,
        (const u8 *const *)0x81FA0A0,
        (const struct Item *)0x83C55BC,
        (const struct BattleMove *)0x81FB0BC,
        (const struct CompressedSpriteSheet *)0x820A8BC,
        (const struct CompressedSpritePalette *)0x820A91C,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138152496u}; // weak

static const struct PMRomHeader sSapphireUSRomHeaderV1 =
    {
        1u,
        2u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81E82FC,
        (const struct CompressedSpriteSheet *)0x81E979C,
        (const struct CompressedSpritePalette *)0x81EA55C,
        (const struct CompressedSpritePalette *)0x81EB31C,
        (const u8 *const *)0x83BBD98,
        (const u8 *)0x83BC478,
        (const struct SpritePalette *)0x83BC630,
        (const u8 (*)[])0x81F7114,
        (const u8 (*)[])0x81F82C8,
        (const struct Decoration *)0x83EB73C,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x81FEBC0,
        (const u8 (*)[])0x81FA1F0,
        (const u8 *const *)0x81FA0B8,
        (const struct Item *)0x83C55DC,
        (const struct BattleMove *)0x81FB0D4,
        (const struct CompressedSpriteSheet *)0x820A8D4,
        (const struct CompressedSpritePalette *)0x820A934,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138152528u}; // weak

static const struct PMRomHeader sRubyDERomHeader =
    {
        2u,
        5u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81F52D0,
        (const struct CompressedSpriteSheet *)0x81F6770,
        (const struct CompressedSpritePalette *)0x81F7530,
        (const struct CompressedSpritePalette *)0x81F82F0,
        (const u8 *const *)0x83C7C30,
        (const u8 *)0x83C8310,
        (const struct SpritePalette *)0x83C84C8,
        (const u8 (*)[])0x82040E8,
        (const u8 (*)[])0x820529C,
        (const struct Decoration *)0x83F7BF0,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x820BBE8,
        (const u8 (*)[])0x8207218,
        (const u8 *const *)0x82070E0,
        (const struct Item *)0x83D13DC,
        (const struct BattleMove *)0x82080FC,
        (const struct CompressedSpriteSheet *)0x82178FC,
        (const struct CompressedSpritePalette *)0x821795C,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138201464u}; // weak

static const struct PMRomHeader sSapphireDERomHeader =
    {
        1u,
        5u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81F5264,
        (const struct CompressedSpriteSheet *)0x81F6704,
        (const struct CompressedSpritePalette *)0x81F74C4,
        (const struct CompressedSpritePalette *)0x81F8284,
        (const u8 *const *)0x83C7B9C,
        (const u8 *)0x83C827C,
        (const struct SpritePalette *)0x83C8434,
        (const u8 (*)[])0x820407C,
        (const u8 (*)[])0x8205230,
        (const struct Decoration *)0x83F7B5C,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x820BB7C,
        (const u8 (*)[])0x82071AC,
        (const u8 *const *)0x8207074,
        (const struct Item *)0x83D1348,
        (const struct BattleMove *)0x8208090,
        (const struct CompressedSpriteSheet *)0x8217890,
        (const struct CompressedSpritePalette *)0x82178F0,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138201316u}; // weak

static const struct PMRomHeader sRubyFRRomHeader =
    {
        2u,
        3u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81F075C,
        (const struct CompressedSpriteSheet *)0x81F1BFC,
        (const struct CompressedSpritePalette *)0x81F29BC,
        (const struct CompressedSpritePalette *)0x81F377C,
        (const u8 *const *)0x83C3704,
        (const u8 *)0x83C3DE4,
        (const struct SpritePalette *)0x83C3F9C,
        (const u8 (*)[])0x81FF574,
        (const u8 (*)[])0x8200728,
        (const struct Decoration *)0x83F33F4,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x8207064,
        (const u8 (*)[])0x8202694,
        (const u8 *const *)0x820255C,
        (const struct Item *)0x83CCFC4,
        (const struct BattleMove *)0x8203578,
        (const struct CompressedSpriteSheet *)0x8212D78,
        (const struct CompressedSpritePalette *)0x8212DD8,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138183732u}; // weak

static const struct PMRomHeader sSapphireFRRomHeader =
    {
        1u,
        3u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81F06EC,
        (const struct CompressedSpriteSheet *)0x81F1B8C,
        (const struct CompressedSpritePalette *)0x81F294C,
        (const struct CompressedSpritePalette *)0x81F370C,
        (const u8 *const *)0x83C3234,
        (const u8 *)0x83C3914,
        (const struct SpritePalette *)0x83C3ACC,
        (const u8 (*)[])0x81FF504,
        (const u8 (*)[])0x82006B8,
        (const struct Decoration *)0x83F2F24,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x8206FF4,
        (const u8 (*)[])0x8202624,
        (const u8 *const *)0x82024EC,
        (const struct Item *)0x83CCAF4,
        (const struct BattleMove *)0x8203508,
        (const struct CompressedSpriteSheet *)0x8212D08,
        (const struct CompressedSpritePalette *)0x8212D68,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138182500u}; // weak

static const struct PMRomHeader sRubyESRomHeader =
    {
        2u,
        7u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81ED074,
        (const struct CompressedSpriteSheet *)0x81EE514,
        (const struct CompressedSpritePalette *)0x81EF2D4,
        (const struct CompressedSpritePalette *)0x81F0094,
        (const u8 *const *)0x83BFD84,
        (const u8 *)0x83C0464,
        (const struct SpritePalette *)0x83C061C,
        (const u8 (*)[])0x81FBE8C,
        (const u8 (*)[])0x81FD040,
        (const struct Decoration *)0x83EF50C,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x8203994,
        (const u8 (*)[])0x81FEFC4,
        (const u8 *const *)0x81FEE8C,
        (const struct Item *)0x83C8FFC,
        (const struct BattleMove *)0x81FFEA8,
        (const struct CompressedSpriteSheet *)0x820F6A8,
        (const struct CompressedSpritePalette *)0x820F708,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138167556u}; // weak

static const struct PMRomHeader sSapphireESRomHeader =
    {
        1u,
        7u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81ED004,
        (const struct CompressedSpriteSheet *)0x81EE4A4,
        (const struct CompressedSpritePalette *)0x81EF264,
        (const struct CompressedSpritePalette *)0x81F0024,
        (const u8 *const *)0x83BFAC0,
        (const u8 *)0x83C01A0,
        (const struct SpritePalette *)0x83C0358,
        (const u8 (*)[])0x81FBE1C,
        (const u8 (*)[])0x81FCFD0,
        (const struct Decoration *)0x83EF248,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x8203924,
        (const u8 (*)[])0x81FEF54,
        (const u8 *const *)0x81FEE1C,
        (const struct Item *)0x83C8D38,
        (const struct BattleMove *)0x81FFE38,
        (const struct CompressedSpriteSheet *)0x820F638,
        (const struct CompressedSpritePalette *)0x820F698,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138166848u}; // weak

static const struct PMRomHeader sRubyITRomHeader =
    {
        2u,
        4u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         114u,
         117u,
         98u,
         121u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81E9FF0,
        (const struct CompressedSpriteSheet *)0x81EB490,
        (const struct CompressedSpritePalette *)0x81EC250,
        (const struct CompressedSpritePalette *)0x81ED010,
        (const u8 *const *)0x83BC974,
        (const u8 *)0x83BD054,
        (const struct SpritePalette *)0x83BD20C,
        (const u8 (*)[])0x81F8E08,
        (const u8 (*)[])0x81F9FBC,
        (const struct Decoration *)0x83EC714,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x82008F0,
        (const u8 (*)[])0x81FBF20,
        (const u8 *const *)0x81FBDE8,
        (const struct Item *)0x83C5FF8,
        (const struct BattleMove *)0x81FCE04,
        (const struct CompressedSpriteSheet *)0x820C604,
        (const struct CompressedSpritePalette *)0x820C664,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138155404u}; // weak

static const struct PMRomHeader sSapphireITRomHeader =
    {
        1u,
        4u,
        {112u,
         111u,
         107u,
         101u,
         109u,
         111u,
         110u,
         32u,
         115u,
         97u,
         112u,
         112u,
         104u,
         105u,
         114u,
         101u,
         32u,
         118u,
         101u,
         114u,
         115u,
         105u,
         111u,
         110u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u,
         0u},
        (const struct CompressedSpriteSheet *)0x81E9F80,
        (const struct CompressedSpriteSheet *)0x81EB420,
        (const struct CompressedSpritePalette *)0x81EC1E0,
        (const struct CompressedSpritePalette *)0x81ECFA0,
        (const u8 *const *)0x83BC618,
        (const u8 *)0x83BCCF8,
        (const struct SpritePalette *)0x83BCEB0,
        (const u8 (*)[])0x81F8D98,
        (const u8 (*)[])0x81F9F4C,
        (const struct Decoration *)0x83EC3B8,
        4640u,
        4928u,
        24u,
        2360u,
        14988u,
        70u,
        2102u,
        2124u,
        386u,
        7u,
        10u,
        10u,
        10u,
        12u,
        12u,
        6u,
        12u,
        6u,
        16u,
        18u,
        12u,
        15u,
        11u,
        1u,
        8u,
        12u,
        2192u,
        15040u,
        564u,
        568u,
        9u,
        10u,
        0u,
        8u,
        1366u,
        1367u,
        12591u,
        12571u,
        0u,
        (const struct SpeciesInfo *)0x8200880,
        (const u8 (*)[])0x81FBEB0,
        (const u8 *const *)0x81FBD78,
        (const struct Item *)0x83C5C9C,
        (const struct BattleMove *)0x81FCD94,
        (const struct CompressedSpriteSheet *)0x820C594,
        (const struct CompressedSpritePalette *)0x820C5F4,
        0u,
        2052u,
        2107u,
        20u,
        20u,
        16u,
        64u,
        46u,
        50u,
        1176u,
        12560u,
        12640u,
        1328u,
        138154544u}; // weak

// used for rs frlg
ALIGNED(4) static const u8 sWallpaperPatternForest[] = {
    0x10, 0xA0, 0x06, 0x00, 0x32, 0x00, 0x00, 0xF0, 0x01, 0xD0, 0x01, 0xEE, 0xEE, 0x30, 0x01, 0xD0,
    0x00, 0xDD, 0xDD, 0xDD, 0x11, 0x11, 0x11, 0x11, 0x52, 0x00, 0x55, 0x25, 0x22, 0x65, 0x66, 0x56
};

//used for emerald
ALIGNED(4) static const u8 sWallpaperPatternEmeForest[32] = {
    0x10, 0xE0, 0x07, 0x00, 0x32, 0x00, 0x00, 0xF0, 0x01, 0xD0, 0x01, 0xEE, 0xEE, 0x30, 0x01, 0xDD, 
    0x00, 0xDD, 0xDD, 0xDD, 0x99, 0x99, 0x99, 0xAA, 0x99, 0x00, 0x99, 0xA9, 0xA9, 0x99, 0x99, 0x9A
};

ALIGNED(4) static const u8 sWaldaWallpaperPatternZigzagoon[32] = {
    0x10, 0x80, 0x08, 0x00, 0x32, 0x00, 0x00, 0xF0, 0x01, 0xD0, 0x01, 0xEE, 0xEE, 0x30, 0x01, 0xDD, 
    0x07, 0xDD, 0xDD, 0xDD, 0x11, 0x11, 0xB0, 0x01, 0xF0, 0x1F, 0xF0, 0x1F, 0xFF, 0xF0, 0x1F, 0xF0
};

ALIGNED(4) static const u8 sWallpaperIconPatternAqua[32] = {
    0x10, 0x80, 0x00, 0x00, 0x28, 0x11, 0x11, 0x20, 0x01, 0x21, 0x30, 0x03, 0x21, 0x22, 0x11, 0x13,
    0x11, 0x22, 0x22, 0x20, 0x06, 0x21, 0x12, 0x30, 0x03, 0x00, 0x01, 0xDF, 0x10, 0x05, 0x10, 0x03
};

ALIGNED(4) static const u8 sOverworld_GetMapHeaderByGroupAndIdPattern[20] = {
    0x00, 0x04, 0x09, 0x04, 0x03, 0x4A, 0x80, 0x0B, 0x80, 0x18, 0x00, 0x68, 0x89, 0x0B, 0x09, 0x18, 
    0x08, 0x68, 0x70, 0x47
};


static const struct PMRomHeader * gRomHeaders [] = {
    &sRubyJPRomHeader,
    &sSapphireJPRomHeader,
	&sRubyUSRomHeaderV0, 
	&sSapphireUSRomHeaderV0, 
	&sRubyUSRomHeaderV1, 
	&sSapphireUSRomHeaderV1, 
	&sRubyDERomHeader, 
	&sSapphireDERomHeader, 
	&sRubyFRRomHeader, 
	&sSapphireFRRomHeader, 
	&sRubyESRomHeader, 
	&sSapphireESRomHeader, 
	&sRubyITRomHeader, 
	&sSapphireITRomHeader
};

const u32 *GetWallpaperTiles(u32 id, bool8 isWaldaWallpaper) {
    bool8 isEmerald = gRomHeader->version == VERSION_EMERALD;
    if (isEmerald && isWaldaWallpaper)
        return (const u32*)sRomResource.waldaWallpaperTable[id * 3];
    else
        return (const u32*)sRomResource.wallpaperTable[id * 3];
}
const u32 *GetWallpaperTilemap(u32 id, bool8 isWaldaWallpaper) {
    bool8 isRubySapp = gRomHeader->version == VERSION_RUBY || gRomHeader->version == VERSION_SAPPHIRE;
    bool8 isEmerald = gRomHeader->version == VERSION_EMERALD;

    if (isEmerald && isWaldaWallpaper)
        return (const u32*)sRomResource.waldaWallpaperTable[id * 3 + 1];
    else
        return (const u32*)sRomResource.wallpaperTable[id * 3 + 1 + isRubySapp * 1];
}

const u16 *GetWallpaperPalette(u32 id, bool8 isWaldaWallpaper) {
    bool8 isRubySapp = gRomHeader->version == VERSION_RUBY || gRomHeader->version == VERSION_SAPPHIRE;
    bool8 isEmerald = gRomHeader->version == VERSION_EMERALD;

    if (isEmerald && isWaldaWallpaper)
        return (const u16*)sRomResource.waldaWallpaperTable[id * 3 + 2];
    else
        return (const u16*)sRomResource.wallpaperTable[id * 3 + 2 + isRubySapp * 1];
}

bool8 SearchResource() {
    const void *patternsPtrs[4];
    void *results[4], *patterns[4];
    vu8 searchPatternsBuffer[0x180];
    u32 begin, end;
    void (*funcSearchPatterns)(u32, u32, const void *, u32, void *[], u32) = (void (*)(u32, u32, const void *, u32, void *[], u32))searchPatternsBuffer;
    memcpy(searchPatternsBuffer, SearchPatterns, sizeof(searchPatternsBuffer));

    end = (u32)gRomHeader->monIcons;
    begin = end - 0x20000;
    if (gRomHeader->version == VERSION_EMERALD) {
        patternsPtrs[0] = sWallpaperPatternEmeForest;
        patternsPtrs[1] = sWaldaWallpaperPatternZigzagoon;
        patternsPtrs[2] = sWallpaperIconPatternAqua;
        patternsPtrs[3] = NULL;
        memset(results, 0, sizeof(results));
        funcSearchPatterns(begin, end, patternsPtrs, 3, results, 32 / 4);
        if (results[0] && results[1] && results[2]) {
            memcpy(patterns, results, sizeof(patterns));
            memset(results, 0, sizeof(results));
            patternsPtrs[0] = &patterns[0];
            patternsPtrs[1] = &patterns[1];
            patternsPtrs[2] = &patterns[2];
            patternsPtrs[3] = NULL;
            funcSearchPatterns(begin, end, patternsPtrs, 3, results, 1);
            if (results[0] && results[1] && results[2]) {
                sRomResource.wallpaperTable = (void **)results[0];
                sRomResource.waldaWallpaperTable = (void **)results[1];
                sRomResource.waldaWallpaperIcons = (const u32 *const *)results[2];   
            }
            else
                return FALSE;
        }
        else
            return FALSE;
    }
    else {
        patternsPtrs[0] = sWallpaperPatternForest;
        patternsPtrs[1] = NULL;
        memset(results, 0, sizeof(results));
        funcSearchPatterns(begin, end, patternsPtrs, 1, results, 32 / 4);
        if (results[0]) {
            patterns[0] = results[0];
            memset(results, 0, sizeof(results));
            patternsPtrs[0] = &patterns[0];
            patternsPtrs[1] = NULL;
            funcSearchPatterns(begin, end, patternsPtrs, 1, results, 1);
            if (results[0])
                sRomResource.wallpaperTable = (void **)results[0];
            else
                return FALSE;
        }
        else
            return FALSE;
    }

    patternsPtrs[0] = sOverworld_GetMapHeaderByGroupAndIdPattern;
    patternsPtrs[1] = NULL;
    memset(results, 0, sizeof(results));
    funcSearchPatterns(0x8000000, 0x8100000, patternsPtrs, 1, results, sizeof(sOverworld_GetMapHeaderByGroupAndIdPattern) / sizeof(u32));
    if (results[0])
        sRomResource.Overworld_GetMapHeaderByGroupAndId = (struct MapHeader const *const (*)(u16, u16))results[0];
    else
        return FALSE;

    sRomResource.experienceTables = (const u32(*)[MAX_LEVEL + 1])((u32)gRomHeader->baseStats - sizeof(*sRomResource.experienceTables) * 8);

    patterns[0] = (void*)(gRomHeader->baseStats + NUM_SPECIES);
    begin = (u32)patterns[0];
    patternsPtrs[0] = &patterns[0];
    patternsPtrs[1] = NULL;
    memset(results, 0, sizeof(results));

    funcSearchPatterns(begin, begin + 0x20000, patternsPtrs, 1, results, 1);
    if (results[0]) 
        sRomResource.levelUpLearnsets = (const u16 *const *)results[0];
    else
        return FALSE;

    gRomResource = &sRomResource;

    return TRUE;
}

void SetRomHeaderForRS() {
    const u8 langChars[] = {'J', 'E', 'E', 'D', 'F', 'S', 'I'};
    u32 gameCode = ROM_GAME_CODE & 0xFFFFFF;
    u8 langChar = ROM_GAME_CODE >> 24;
    u8 langIdx = 0;
    for ( ; langIdx < ARRAY_COUNT(langChars); langIdx++)
    {
        if (langChars[langIdx] == langChar) {
            break;
        }
    }
    if (langIdx < ARRAY_COUNT(langChars)) {
        if (langIdx == 1) {
            if (ROM_SOFTWARE_VERSION != 0)
                langIdx += 1;
        }
        switch(gameCode) {
            case RUBY_GAME_CODE:
                gRomHeader = gRomHeaders[langIdx * 2];
                break;
            case SAPPHIRE_GAME_CODE:
                gRomHeader = gRomHeaders[langIdx * 2 + 1];
                break;
        }
    }
}

bool8 DetectCart() {
    u32 gameCode = ROM_GAME_CODE & 0xFFFFFF;
    switch(gameCode) 
    {
        case RUBY_GAME_CODE:
        case SAPPHIRE_GAME_CODE:
            SetRomHeaderForRS();
            if (gRomHeader == NULL)
                return FALSE;
            break;
        case FIRERED_GAME_CODE:
        case LEAFGREAN_GAME_CODE:
        case EMERALD_GAME_cODE:
            gRomHeader = (const struct PMRomHeader *)0x8000100;
            break;
        default:
            return FALSE;
    }

    return SearchResource();
}