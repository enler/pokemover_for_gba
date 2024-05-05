#include "global.h"
#include "load_save.h"
#include "malloc.h"
#include "pokemon_rom_resource.h"
#include "save.h"

u8 *gPokedexSeen1;
u8 *gPokedexSeen2;
struct Pokedex *gPokedex;

bool8 DetectSave() {
    u8 status;
    if (!gSaveBlock2Ptr)
        gSaveBlock2Ptr = (struct SaveBlock2 *)Alloc(gRomHeader->saveBlock2Size);
    if (!gSaveBlock1Ptr)
        gSaveBlock1Ptr = (struct SaveBlock1 *)Alloc(gRomHeader->saveBlock1Size);
    if (!gPokemonStoragePtr)
        gPokemonStoragePtr = (struct PokemonStorage *)Alloc(sizeof(struct PokemonStorage));

    CheckForFlashMemory();

    if (!gFlashMemoryPresent)
        return FALSE;
    
    Save_ResetSaveCounters();
    status = LoadGameSave(SAVE_NORMAL);
    if (status == SAVE_STATUS_EMPTY || status == SAVE_STATUS_CORRUPT || status == SAVE_STATUS_ERROR)
        return FALSE;
    gPokedexSeen1 = (u8*)gSaveBlock1Ptr + gRomHeader->seen1Offset;
    gPokedexSeen2 = (u8*)gSaveBlock1Ptr + gRomHeader->seen2Offset;
    gPokedex = (struct Pokedex *)((u8 *)gSaveBlock2Ptr + gRomHeader->pokedexOffset);
    return TRUE;
}