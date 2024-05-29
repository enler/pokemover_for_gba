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
#include "text_ext.h"

#define _Static_assert(expr, msg)

extern u8 gHeapEnd[];
extern u8 __vram_font_orig__[];
extern u8 __vram_font_start__[];
extern u8 __vram_font_end__[];

static void VBlankIntr(void);
static void HBlankIntr(void);
static void VCountIntr(void);
static void SerialIntr(void);
static void IntrDummy(void);

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
    RegisterRamReset(~RESET_EWRAM & 0xFF);
    CpuCopy16(__vram_font_orig__, __vram_font_start__, __vram_font_end__ - __vram_font_start__);
    CpuFill16(0, gHeap, gHeapEnd - gHeap);
    InitHeap(gHeap, gHeapEnd - gHeap);

    REG_WAITCNT = WAITCNT_PREFETCH_ENABLE | WAITCNT_WS0_S_1 | WAITCNT_WS0_N_3;
    InitGpuRegManager();
    ResetSpriteData();
    InitKeys();

    ClearDma3Requests();

    REG_IME = 1;
    EnableInterrupts(INTR_FLAG_VBLANK);

    InitBppConvTable();

    ShowPokeMoverScreen(0);

    for (;;)
    {
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
    gMain.vblankCounter1++;
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
    SoftReset(0);
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

void ReadKeys(void)
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

    if (JOY_NEW(gMain.watchedKeysMask))
        gMain.watchedKeysPressed = TRUE;
}