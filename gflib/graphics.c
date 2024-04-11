#include "global.h"
#include "data/graphics/pokemon.h"

const u8 gMonIcon_QuestionMark[] = INCBIN_U8("graphics/pokemon/question_mark/icon.4bpp");

const u16 gMonIconPalettes[][16] =
{
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_0.gbapal"),
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_1.gbapal"),
    INCBIN_U16("graphics/pokemon/icon_palettes/icon_palette_2.gbapal"),
};

const u16 gMessageBox_Pal[] = INCBIN_U16("graphics/text_window/message_box.gbapal");
const u8 gMessageBox_Gfx[] = INCBIN_U8("graphics/text_window/message_box.4bpp");

const u32 gWallpaperTiles_Horizontal[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/horizontal/tiles.4bpp.lz");
const u32 gWallpaperTilemap_Horizontal[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/horizontal/tilemap.bin.lz");

const u16 gWallpaperPalettes_Ribbon[][16] =
{
    INCBIN_U16("graphics/pokemon_storage/wallpapers/ribbon/frame.gbapal"),
    INCBIN_U16("graphics/pokemon_storage/wallpapers/ribbon/bg.gbapal"),
};

const u32 gWallpaperTiles_Ribbon[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/ribbon/tiles.4bpp.lz");
const u32 gWallpaperTilemap_Ribbon[] = INCBIN_U32("graphics/pokemon_storage/wallpapers/ribbon/tilemap.bin.lz");

const u8 gMonIcon_Egg[] = INCBIN_U8("graphics/pokemon/egg/icon.4bpp");

const u16 gWallpaperPalettes_Horizontal[][16] =
{
    INCBIN_U16("graphics/pokemon_storage/wallpapers/friends_frame2.gbapal"),
    INCBIN_U16("graphics/pokemon_storage/wallpapers/horizontal/bg.gbapal"),
};