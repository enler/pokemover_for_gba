#include "global.h"
#include "agb_flash.h"
#include "dma3.h"
#include "gpu_regs.h"
#include "load_save.h"
#include "malloc.h"
#include "main.h"
#include "poke_mover_screen.h"
#include "save.h"
#include "text.h"

extern u8 gHeapEnd[];

extern u8 __data_orig__[];
extern u8 __data_start__[];
extern u8 __data_end__[];


static void VBlankIntr(void);
static void HBlankIntr(void);
static void VCountIntr(void);
static void SerialIntr(void);
static void IntrDummy(void);
static void ReadKeys(void);

const u8 gGameVersion = GAME_VERSION;

const u8 gGameLanguage = GAME_LANGUAGE; // English

u32 gBattleTypeFlags = 0;

IntrFunc gIntrTable[] =
{
    IntrDummy, // V-count interrupt
    IntrDummy, // Serial interrupt
    IntrDummy, // Timer 3 interrupt
    IntrDummy, // H-blank interrupt
    VBlankIntr, // V-blank interrupt
    IntrDummy,  // Timer 0 interrupt
    IntrDummy,  // Timer 1 interrupt
    IntrDummy,  // Timer 2 interrupt
    IntrDummy,  // DMA 0 interrupt
    IntrDummy,  // DMA 1 interrupt
    IntrDummy,  // DMA 2 interrupt
    IntrDummy,  // DMA 3 interrupt
    IntrDummy,  // Key interrupt
    IntrDummy,  // Game Pak interrupt
};

struct Main gMain;
u16 gKeyRepeatContinueDelay;
u16 gKeyRepeatStartDelay;

void AgbMain()
{
    REG_IME = 0;
    RegisterRamReset(RESET_ALL);
    memcpy(__data_start__, __data_orig__, __data_end__ - __data_start__);
    InitHeap(gHeap, gHeapEnd - gHeap);

    REG_WAITCNT = WAITCNT_PREFETCH_ENABLE | WAITCNT_WS0_S_1 | WAITCNT_WS0_N_3;
    InitGpuRegManager();
    ResetSpriteData();
    InitKeys();

    SetDefaultFontsPointer();
    ClearDma3Requests();

    gSaveBlock1Ptr = Alloc(sizeof(struct SaveBlock1));
    gSaveBlock2Ptr = Alloc(sizeof(struct SaveBlock2));
    gPokemonStoragePtr = Alloc(sizeof(struct PokemonStorage));

    REG_IME = 1;
    EnableInterrupts(INTR_FLAG_VBLANK);
    
    CheckForFlashMemory();
    Save_ResetSaveCounters();
    LoadGameSave(SAVE_NORMAL);

    ShowPokeMoverScreen(0);

    for (;;) {
        ReadKeys();
        if (gMain.callback2)
            gMain.callback2();
        VBlankIntrWait();
    }
}

static void VBlankIntr(void)
{
    if (gMain.vblankCallback)
        gMain.vblankCallback();
    CopyBufferedValuesToGpuRegs();
    ProcessDma3Requests();
    INTR_CHECK |= INTR_FLAG_VBLANK;
}

static void IntrDummy(void)
{
    
}

void SetVBlankCallback(IntrCallback callback)
{
    gMain.vblankCallback = callback;
}

void SetHBlankCallback(IntrCallback callback)
{
    gMain.hblankCallback = callback;
}

void SetMainCallback2(MainCallback callback)
{
    gMain.callback2 = callback;
    gMain.state = 0;
}

void InitFlashTimer(void)
{
    SetFlashTimerIntr(2, gIntrTable + 0x7);
}

void DoSoftReset(void)
{
    REG_IME = 0;
    DmaStop(1);
    DmaStop(2);
    DmaStop(3);
    SoftReset(RESET_ALL);
}

void InitKeys(void)
{
    gKeyRepeatContinueDelay = 5;
    gKeyRepeatStartDelay = 40;

    gMain.heldKeys = 0;
    gMain.newKeys = 0;
    gMain.newAndRepeatedKeys = 0;
    gMain.heldKeysRaw = 0;
    gMain.newKeysRaw = 0;
}

static void ReadKeys(void)
{
    u16 keyInput = REG_KEYINPUT ^ KEYS_MASK;
    gMain.newKeysRaw = keyInput & ~gMain.heldKeysRaw;
    gMain.newKeys = gMain.newKeysRaw;
    gMain.newAndRepeatedKeys = gMain.newKeysRaw;

    // BUG: Key repeat won't work when pressing L using L=A button mode
    // because it compares the raw key input with the remapped held keys.
    // Note that newAndRepeatedKeys is never remapped either.

    if (keyInput != 0 && gMain.heldKeys == keyInput)
    {
        gMain.keyRepeatCounter--;

        if (gMain.keyRepeatCounter == 0)
        {
            gMain.newAndRepeatedKeys = keyInput;
            gMain.keyRepeatCounter = gKeyRepeatContinueDelay;
        }
    }
    else
    {
        // If there is no input or the input has changed, reset the counter.
        gMain.keyRepeatCounter = gKeyRepeatStartDelay;
    }

    gMain.heldKeysRaw = keyInput;
    gMain.heldKeys = gMain.heldKeysRaw;

    // Remap L to A if the L=A option is enabled.
    if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
    {
        if (JOY_NEW(L_BUTTON))
            gMain.newKeys |= A_BUTTON;

        if (JOY_HELD(L_BUTTON))
            gMain.heldKeys |= A_BUTTON;
    }

    if (JOY_NEW(gMain.watchedKeysMask))
        gMain.watchedKeysPressed = TRUE;
}