#include "global.h"
#define _Static_assert(expr, msg)

#define ROM_START 0x8000000

extern u8 __begin__[];
extern u8 __end__[];

void PrepareBoot() {
    u32 return_address = (u32)__builtin_return_address(0);
    void (*EntryPoint)() = (void (*)())__begin__;
    if (return_address >= ROM_START) {
        CpuCopy16((void*)ROM_START, __begin__, __end__ - __begin__);
        EntryPoint();
    }
}