#ifndef GUARD_POKEMON_ROM_RESOURCE_H
#define GUARD_POKEMON_ROM_RESOURCE_H

struct PMRomHeader
{
  u32 version;
  u32 language;
  u8 gameName[32];
  const struct CompressedSpriteSheet *monFrontPics;
  const struct CompressedSpriteSheet *monBackPics;
  const struct CompressedSpritePalette *monNormalPalettes;
  const struct CompressedSpritePalette *monShinyPalettes;
  const u8 *const *monIcons;
  const u8 *monIconPaletteIds;
  const struct SpritePalette *monIconPalettes;
  const u8 (*monSpeciesNames)[];
  const u8 (*moveNames)[];
  const struct Decoration *decorations;
  u32 flagsOffset;
  u32 varsOffset;
  u32 pokedexOffset;
  u32 seen1Offset;
  u32 seen2Offset;
  u32 pokedexVar;
  u32 pokedexFlag;
  u32 mysteryEventFlag;
  u32 pokedexCount;
  u8 person_name_size;
  u8 tr_name_size;
  u8 mons_name_size;
  u8 mons_disp_size;
  u8 waza_name_size;
  u8 item_name_size;
  u8 seed_name_size;
  u8 speabi_name_size;
  u8 zokusei_name_size;
  u8 map_name_width;
  u8 mapname_max;
  u8 trtype_name_size;
  u8 goods_name_size;
  u8 zukan_type_size;
  u8 eom_size;
  u8 btl_tr_name_size;
  u8 kaiwa_work_size;
  u32 saveBlock2Size;
  u32 saveBlock1Size;
  u32 partyCountOffset;
  u32 partyOffset;
  u32 warpFlagsOffset;
  u32 trainerIdOffset;
  u32 playerNameOffset;
  u32 playerGenderOffset;
  u32 frontierStatusOffset;
  u32 frontierStatusOffset2;
  u32 externalEventFlagsOffset;
  u32 externalEventDataOffset;
  u32 expand_soft_disable_flag;
  const struct SpeciesInfo *baseStats;
  const u8 (*abilityNames)[];
  const u8 *const *abilityDescriptions;
  const struct Item *items;
  const struct BattleMove *moves;
  const struct CompressedSpriteSheet *ballGfx;
  const struct CompressedSpritePalette *ballPalettes;
  u32 gcnLinkFlagsOffset;
  u32 gameClearFlag;
  u32 ribbonFlag;
  u8 bagCountItems;
  u8 bagCountKeyItems;
  u8 bagCountPokeballs;
  u8 bagCountTMHMs;
  u8 bagCountBerries;
  u8 pcItemsCount;
  u32 pcItemsOffset;
  u32 giftRibbonsOffset;
  u32 enigmaBerryOffset;
  u32 mapViewOffset;
  u32 ram_seed_data_size;
};

struct PMRomResource {
    const u16 *const *levelUpLearnsets;
    const u32 (*experienceTables)[MAX_LEVEL + 1];
    void **wallpaperTable;
    void **waldaWallpaperTable;
    const u32 *const *waldaWallpaperIcons;
    struct MapHeader const *const (*Overworld_GetMapHeaderByGroupAndId)(u16, u16);
};

extern const struct PMRomHeader *gRomHeader;
extern const struct PMRomResource *gRomResource;

const u32 *GetWallpaperTiles(u32 id, bool8 isWaldaWallpaper);
const u32 *GetWallpaperTilemap(u32 id, bool8 isWaldaWallpaper);
const u16 *GetWallpaperPalette(u32 id, bool8 isWaldaWallpaper);

#endif