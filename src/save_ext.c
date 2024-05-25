#include "global.h"
#include "agb_sram.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "pokemon_rom_resource.h"
#include "save.h"
#include "string_util.h"
#include "text_ext.h"
#include "window.h"

u8 *gPokedexSeen1;
u8 *gPokedexSeen2;
struct Pokedex *gPokedex;
u16 *gSaveVars;
u8 *gSaveFlags;

static const u8 gTextSaveFailed[] = _("写入记录时发生错误，\n请按下A键重启。\p");

u8 CheckSaveLanguage() {
    struct Pokemon *partyPokemon = (struct Pokemon *)((u8 *)gSaveBlock1Ptr + gRomHeader->partyOffset);
    u32 trainerId;
    u8 *playerName = (u8 *)((u8 *)gSaveBlock2Ptr + gRomHeader->playerNameOffset);
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u32 otId;
    u16 species;
    memcpy(&trainerId, ((u8 *)gSaveBlock2Ptr + gRomHeader->trainerIdOffset), sizeof(u32));
    for (int i = 0; i < PARTY_SIZE; i++) {
        GetBoxMonData(&partyPokemon[i].box, MON_DATA_OT_NAME, otName);
        otId = GetBoxMonData(&partyPokemon[i].box, MON_DATA_OT_ID);
        species = GetBoxMonData(&partyPokemon[i].box, MON_DATA_SPECIES_OR_EGG);
        if (species == SPECIES_NONE)
            break;
        if (species == SPECIES_EGG)
            continue;
        if (otId == trainerId && StringCompare(otName, playerName) == 0)
            return GetBoxMonData(&partyPokemon[i].box, MON_DATA_LANGUAGE);
    }

    if (playerName[6] == 0 && playerName[7] == 0)
        return LANGUAGE_JAPANESE;

    return gRomHeader->language;
}

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