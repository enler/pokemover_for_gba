#include "global.h"

void LoadPalette(const void *src, u16 offset, u16 size) {
    CpuCopy16(src, (u16 *)PLTT + offset, size);
}