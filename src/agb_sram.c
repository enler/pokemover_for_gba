#include "agb_sram.h"
#include "gba/flash_internal.h"

#define SECTOR_SIZE 4096

const char gAgbSramLibVer[] = "NINTENDOSRAM_V113";

static void ReadSram_Core(const u8 *src, u8 *dest, u32 size)
{
    while (--size != -1)
        *dest++ = *src++;
}

void ReadSram(const u8 *src, u8 *dest, u32 size)
{
    const u16 *s;
    vu16 *d;
    vu8 readSramFast_Work[64];
    u16 size_;
    
    REG_WAITCNT = (REG_WAITCNT & ~3) | 3;
    s = (void *)((uintptr_t)ReadSram_Core);
    s = (void *)((uintptr_t)s & ~1);
    d = (vu16*)readSramFast_Work;
    size_ = ((uintptr_t)ReadSram - (uintptr_t)ReadSram_Core) / 2;
    while (size_ != 0)
    {
        *d++ = *s++;
        --size_;
    }
    ((void (*)(const u8 *, u8 *, u32))readSramFast_Work + 1)(src, dest, size);
}

void WriteSram(const u8 *src, u8 *dest, u32 size)
{
    REG_WAITCNT = (REG_WAITCNT & ~3) | 3;
    while (--size != -1)
        *dest++ = *src++;
}

static u32 VerifySram_Core(const u8 *src, u8 *dest, u32 size)
{
    while (--size != -1)
        if (*dest++ != *src++)
            return (u32)(dest - 1);
    return 0;
}

u32 VerifySram(const u8 *src, u8 *dest, u32 size)
{
    const u16 *s;
    vu16 *d;
    vu8 verifySramFast_Work[64];
    u16 size_;
    
    REG_WAITCNT = (REG_WAITCNT & ~3) | 3;
    s = (void *)((uintptr_t)VerifySram_Core);
    s = (void *)((uintptr_t)s & ~1);
    d = (vu16*)verifySramFast_Work;
    size_ = ((uintptr_t)VerifySram - (uintptr_t)VerifySram_Core) / 2;
    while (size_ != 0)
    {
        *d++ = *s++;
        --size_;
    }
    return ((u32 (*)(const u8 *, u8 *, u32))verifySramFast_Work + 1)(src, dest, size);
}

u32 WriteSramEx(const u8 *src, u8 *dest, u32 size)
{
    u8 i;
    u32 errorAddr;

    // try writing and verifying the data 3 times
    for (i = 0; i < SRAM_RETRY_MAX; ++i)
    {
        WriteSram(src, dest, size);
        errorAddr = VerifySram(src, dest, size);
        if (errorAddr == 0)
            break;
    }
    return errorAddr;
}

void SwitchSramBank(u8 bankNum) {
    *(vu8 *)0x9000000 = bankNum;
}

u16 EraseSramSectorImpl(u16 sectorNum) {
    SwitchSramBank(sectorNum / SECTORS_PER_BANK);
    sectorNum = sectorNum % SECTORS_PER_BANK;
    REG_WAITCNT = (REG_WAITCNT & ~3) | 3;
    u8 *dest = (u8*)SRAM_ADR + sectorNum * SECTOR_SIZE;
    s32 size = SECTOR_SIZE;
    while (--size != -1)
        *dest++ = 0xFF;
    return 0;
}

void ReadSramImpl(u16 sectorNum, u32 offset, u8 *dest, u32 size) {
    SwitchSramBank(sectorNum / SECTORS_PER_BANK);
    sectorNum = sectorNum % SECTORS_PER_BANK;
    ReadSram((const u8*)SRAM_ADR + sectorNum * SECTOR_SIZE + offset, dest, size);
}


u32 WriteSramSectorAndVerifyImpl(u16 sectorNum, u8 *src) {
    SwitchSramBank(sectorNum / SECTORS_PER_BANK);
    sectorNum = sectorNum % SECTORS_PER_BANK;
    return WriteSramEx(src, (u8*)SRAM_ADR + sectorNum * SECTOR_SIZE, SECTOR_SIZE);
}

u16 WriteSramByte(u16 sectorNum, u32 offset, u8 data) {
    REG_WAITCNT = (REG_WAITCNT & ~WAITCNT_SRAM_MASK) | WAITCNT_SRAM_8;
    SwitchSramBank(sectorNum / SECTORS_PER_BANK);
    sectorNum = sectorNum % SECTORS_PER_BANK;
    WriteSram(&data, (u8 *)SRAM_ADR + sectorNum * SECTOR_SIZE + offset, sizeof(data));
    return 0;
}

bool8 IdentifySram() {
    vu8 *sramTest = (vu8 *)(SRAM);
    REG_WAITCNT = (REG_WAITCNT & ~WAITCNT_SRAM_MASK) | WAITCNT_SRAM_8;
    SwitchSramBank(0);
    vu8 val = *sramTest;
    *sramTest = ~val;
    if (val != *sramTest) {
        ReadFlash = ReadSramImpl;
        EraseFlashSector = EraseSramSectorImpl;
        ProgramFlashSectorAndVerify = WriteSramSectorAndVerifyImpl;
        ProgramFlashByte = WriteSramByte;
        *sramTest = val;
        return TRUE;
    }
    else
        return FALSE;
}
