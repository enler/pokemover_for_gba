#include "global.h"
#include "m4a.h"
#include "gba/multiboot.h"
#include "gba/multiboot_handler.h"

int timed_sio_normal_master(int data, int vCountWait) {
    u8 curr_vcount, target_vcount;

    
    REG_SIODATA32 = data;
        
    
    // - Wait at least 36 us between sends (this is a bit more, but it works)
    curr_vcount = REG_VCOUNT;
    target_vcount = curr_vcount + vCountWait;
    if(target_vcount >= 0xE4)
        target_vcount -= 0xE4;
    while (target_vcount != REG_VCOUNT);
    
    // - Set Start flag.
    REG_SIOCNT |= SIO_START;
    // - Wait for IRQ (or for Start bit to become zero).
    while (REG_SIOCNT & SIO_START);

    // - Process received data.
    return REG_SIODATA32;
}

int multiboot_normal_send(int data) {
    // Only this part of REG_SIODATA32 is used during setup.
    // The rest is handled by SWI $25
    if (REG_VCOUNT + 2 >= DISPLAY_HEIGHT) VBlankIntrWait();
    return (timed_sio_normal_master(data, 2) >> 0x10);
}

void wait_fun(int read_value, int wait) {
    int frames;
    
    //REG_IME = 1;
    //for (frames = 0; frames < 64; ++frames) {
    if(wait)
        VBlankIntrWait();
    //}

    
    //REG_IME = 0;
}

NAKED
static int MultiBootNormal32(struct MultiBootParam * mp)
{
    asm_unified("\
	movs r1, #0\n\
	svc #37\n\
	bx lr\n");
}

enum MULTIBOOT_RESULTS multiboot_normal (const u8* data, const u8* end) {
    int response, mbResult;
    u8 clientMask = 0;
    int attempts, sends, halves;
    u8 answer, handshake;
    const u16 *header = (const u16*)data;
    const u8 palette = 0x81;
    const int paletteCmd = 0x6300 | palette;
    struct MultiBootParam mp;

    REG_RCNT = 0;
    REG_SIOCNT = SIO_32BIT_MODE | 1;

    for(attempts = 0; attempts < 32; attempts++) {
        for (sends = 0; sends < 16; sends++) {
            response = multiboot_normal_send(0x6200);

            if ((response & 0xfff0) == 0x7200)
                clientMask |= (response & 0xf);
        }

        if (clientMask)
            break;
        else
            wait_fun(response, 1);
    }

    if (!clientMask) {
        return MB_NO_INIT_SYNC;
    }

    wait_fun(response, 0);

    clientMask &= 0xF;
    response = multiboot_normal_send(0x6100 | clientMask);
    if (response != (0x7200 | clientMask))
        return MB_WRONG_ANSWER;

    for (halves = 0; halves < 0x60; ++halves) {

        response = multiboot_normal_send(header[halves]);

        if (response != ((0x60 - halves) << 8 | clientMask))
            return MB_HEADER_ISSUE;
        
        wait_fun(response, 0);
    }

    response = multiboot_normal_send(0x6200);
    if (response != (clientMask))
        return MB_WRONG_ANSWER;

    response = multiboot_normal_send(0x6200 | clientMask);
    if (response != (0x7200 | clientMask))
        return MB_WRONG_ANSWER;

    while ((response & 0xFF00) != 0x7300) {
        response = multiboot_normal_send(paletteCmd);
    
        wait_fun(response, 0);
    }

    answer = response&0xFF;
    handshake = 0x11 + 0xFF + 0xFF + answer;

    response = multiboot_normal_send(0x6400 | handshake);
    if ((response & 0xFF00) != 0x7300)
        return MB_WRONG_ANSWER;

    wait_fun(response, 0);
    
    mp.handshake_data = handshake;
    mp.client_data[0] = answer;
    mp.client_data[1] = 0xFF;
    mp.client_data[2] = 0xFF;
    mp.palette_data = palette;
    mp.client_bit = clientMask;
    mp.boot_srcp = data + 0xC0;
    mp.boot_endp = end;
    m4aMPlayAllStop();
    m4aSoundVSyncOff();
    mbResult = MultiBootNormal32(&mp);
    m4aSoundVSyncOn();
    m4aMPlayAllContinue();

    return mbResult != 0 ? MB_SWI_FAILURE : MB_SUCCESS;
}