#include "global.h"
#include "define_gsc.h"
#include "gpu_regs.h"
#include "io_reg.h"
#include "link_poke_mover.h"
#include "main.h"
#include "malloc.h"

static void IntrSerial_LinkPokeMover(void);
static void IntrTimer3_LinkPokeMover(void);

enum {
    MODE_NONE,
    MODE_RECV,
    MODE_SEND,
    MODE_SYNC
};

struct LinkPokeMover {
    u8 mode;
    u8 command;
    u8 status;
    u8 state;
    u8 buffer[MAX_BUFFER_SIZE];
    u16 length;
    u16 offset;
    NotifyStatusChangedCallback callback;
};

static struct LinkPokeMover *sLinkPokeMover;

void SetupLinkPokeMover(NotifyStatusChangedCallback callback)
{
    if (sLinkPokeMover == NULL)
        sLinkPokeMover = (struct LinkPokeMover *)AllocZeroed(sizeof(struct LinkPokeMover));
    sLinkPokeMover->callback = callback;
    gIntrTable[1] = IntrSerial_LinkPokeMover;
    gIntrTable[2] = IntrTimer3_LinkPokeMover;
    IntrEnable(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
}

u8 CalcCRC8(const u8* data, int len) {
    u8 crc = 0xFF;
    int i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];

        for (j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            }
            else {
                crc <<= 1;
            }
        }
    }

    return crc;
}


static void IntrTimer3_LinkPokeMover(void)
{
    REG_TM3CNT_H &= ~TIMER_ENABLE;
    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_SIOCNT |= SIO_ENABLE;
}

static void IntrSerial_LinkPokeMover(void)
{
    u8 data = REG_SIODATA8 & 0xFF;
    if (sLinkPokeMover == NULL)
        return;
    if (sLinkPokeMover->mode == MODE_RECV)
    {
        switch (sLinkPokeMover->state)
        {
        case 0:
            if (data == 0xFD)
            {
                sLinkPokeMover->state = 1;
                sLinkPokeMover->status = TRANSFER_BUSY;
                if (sLinkPokeMover->callback)
                    sLinkPokeMover->callback(sLinkPokeMover->status, FALSE);
            }
            break;
        case 1:
            if (data == 0xFF)
                sLinkPokeMover->state = 2;
            else if (data == 0xFE)
                sLinkPokeMover->state = 3;
            else if (sLinkPokeMover->offset < sizeof(sLinkPokeMover->buffer))
            {
                sLinkPokeMover->buffer[sLinkPokeMover->offset++] = data;
                if (sLinkPokeMover->callback)
                    sLinkPokeMover->callback(sLinkPokeMover->status, TRUE);
            }
            break;
        case 2:
            if (sLinkPokeMover->offset < sizeof(sLinkPokeMover->buffer))
            {
                sLinkPokeMover->buffer[sLinkPokeMover->offset++] = data;
                if (sLinkPokeMover->callback)
                    sLinkPokeMover->callback(sLinkPokeMover->status, TRUE);
            }
            sLinkPokeMover->state = 1;
            break;
        case 3:
            if (data == CalcCRC8(sLinkPokeMover->buffer, sLinkPokeMover->offset))
                sLinkPokeMover->status = TRANSFER_COMPLETE;
            else
                sLinkPokeMover->status = TRANSFER_FAILED;
            if (sLinkPokeMover->callback)
                sLinkPokeMover->callback(sLinkPokeMover->status, FALSE);
            return;
        default:
            break;
        }
    } 
    else if (sLinkPokeMover->mode == MODE_SEND)
    {
        switch (sLinkPokeMover->state) {
            case 0:
                sLinkPokeMover->state++;
                sLinkPokeMover->status = TRANSFER_BUSY;
                if (sLinkPokeMover->callback)
                    sLinkPokeMover->callback(sLinkPokeMover->status, FALSE);
            case 1:
                if (sLinkPokeMover->command == data)
                {
                    if (sLinkPokeMover->offset < sLinkPokeMover->length) {
                        sLinkPokeMover->command = 0x80 | (sLinkPokeMover->buffer[sLinkPokeMover->offset++] & 0x1F);
                        if (sLinkPokeMover->callback)
                            sLinkPokeMover->callback(sLinkPokeMover->status, TRUE);
                    }
                    else {
                        sLinkPokeMover->command = 0xC0;
                        sLinkPokeMover->state++;
                    }
                }
                break;
            case 2:
                if (data == 0xC1) {
                    sLinkPokeMover->state++;
                    sLinkPokeMover->status = TRANSFER_WAIT_SYNC;
                    if (sLinkPokeMover->callback)
                        sLinkPokeMover->callback(sLinkPokeMover->status, FALSE);
                }
                break;
            case 3:
                if (data != 0xC1) {
                    sLinkPokeMover->status = TRANSFER_FAILED;
                }
                if (sLinkPokeMover->callback)
                    sLinkPokeMover->callback(sLinkPokeMover->status, TRUE);
                break;
        }
    }
    else if (sLinkPokeMover->mode == MODE_SYNC) {
        switch (sLinkPokeMover->state) {
            case 3:
                sLinkPokeMover->command = 0xE0;
                sLinkPokeMover->state++;
                break;
            case 4:
                if (data == 0xE1) {
                    sLinkPokeMover->state++;
                    sLinkPokeMover->status = TRANSFER_SYNC_COMPLETE;
                    if (sLinkPokeMover->callback)
                        sLinkPokeMover->callback(sLinkPokeMover->status, FALSE);
                    return;
                }
                break;
        }
    }

    REG_SIODATA8 = sLinkPokeMover->command;
    REG_TM3CNT_H |= TIMER_ENABLE;
}

void ExecBufferCommand(u8 cmd) {
    sLinkPokeMover->mode = MODE_RECV;
    sLinkPokeMover->command = cmd;
    sLinkPokeMover->offset = 0;
    sLinkPokeMover->state = 0;
    sLinkPokeMover->status = TRANSFER_IDLE;

    REG_RCNT = 0;
    REG_SIOCNT = SIO_INTR_ENABLE | SIO_8BIT_MODE | 1;
    REG_SIODATA8 = cmd;

    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_TM3CNT_H = TIMER_INTR_ENABLE | TIMER_64CLK;
    REG_TM3CNT_H |= TIMER_ENABLE;
}

void ExecEraseCommand(u8 cmd, u8 * buffer, u16 length) {
    int i;
    sLinkPokeMover->mode = MODE_SEND;
    sLinkPokeMover->command = cmd;
    sLinkPokeMover->offset = 0;
    sLinkPokeMover->state = 0;
    sLinkPokeMover->status = TRANSFER_IDLE;
    if (length != 0)
        sLinkPokeMover->length = length;

    if (buffer != NULL && length != 0)
        for (i = 0; i < length; i++)
            sLinkPokeMover->buffer[i] = buffer[i];

    REG_RCNT = 0;
    REG_SIOCNT = SIO_INTR_ENABLE | SIO_8BIT_MODE | 1;
    REG_SIODATA8 = cmd;

    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_TM3CNT_H = TIMER_INTR_ENABLE | TIMER_64CLK;
    REG_TM3CNT_H |= TIMER_ENABLE;
}

void FinishSyncErase() {
    sLinkPokeMover->mode = MODE_SYNC;
}

u8 *GetLinkPokeMoverBuffer() {
    return sLinkPokeMover->buffer;
}

void ExitLinkPokeMover() {
    u16 imeTemp;
    DisableInterrupts(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    imeTemp = REG_IME;
    REG_IME = 0;
    REG_IF = INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL;
    REG_TM3CNT_H = 0;
    REG_SIOCNT = REG_SIOCNT & ~SIO_INTR_ENABLE;
    if (sLinkPokeMover)
        FREE_AND_SET_NULL(sLinkPokeMover);
    REG_IME = imeTemp;
}