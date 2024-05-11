#include "global.h"
#include "agb_sram.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "pokemon_rom_resource.h"
#include "save.h"
#include "text_ext.h"
#include "window.h"

u8 *gPokedexSeen1;
u8 *gPokedexSeen2;
struct Pokedex *gPokedex;
u16 *gSaveVars;
u8 *gSaveFlags;

static const u8 gTextSaveFailed[] = _("写入记录时发生错误，\n请按下A键重启。\p");

bool8 DetectSave() {
    u8 status;
    if (!gSaveBlock2Ptr)
        gSaveBlock2Ptr = (struct SaveBlock2 *)Alloc(gRomHeader->saveBlock2Size);
    if (!gSaveBlock1Ptr)
        gSaveBlock1Ptr = (struct SaveBlock1 *)Alloc(gRomHeader->saveBlock1Size);
    if (!gPokemonStoragePtr)
        gPokemonStoragePtr = (struct PokemonStorage *)Alloc(sizeof(struct PokemonStorage));
    if (!IdentifySram())
    {
        CheckForFlashMemory();
        if (!gFlashMemoryPresent)
            return FALSE;
    }
    else
        gFlashMemoryPresent = TRUE;

    Save_ResetSaveCounters();
    status = LoadGameSave(SAVE_NORMAL);
    if (status == SAVE_STATUS_EMPTY || status == SAVE_STATUS_CORRUPT || status == SAVE_STATUS_ERROR)
        return FALSE;
    gPokedexSeen1 = (u8*)gSaveBlock1Ptr + gRomHeader->seen1Offset;
    gPokedexSeen2 = (u8*)gSaveBlock1Ptr + gRomHeader->seen2Offset;
    gPokedex = (struct Pokedex *)((u8 *)gSaveBlock2Ptr + gRomHeader->pokedexOffset);
    gSaveVars = (u16 *)((u32)gSaveBlock1Ptr + gRomHeader->varsOffset);
    gSaveFlags = (u8 *)gSaveBlock1Ptr + gRomHeader->flagsOffset;
    return TRUE;
}

void DoSaveFailedScreen(u8 saveType) {
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    DrawMessage(gTextSaveFailed, 2);
    while (!HandleMessage()) {
        VBlankIntrWait();
        ReadKeys();
    }
    DoSoftReset();
}