#include "define_gsc.h"
#include "global.h"
#include "gpu_regs.h"
#include "link_poke_mover.h"
#include "main.h"
#include "malloc.h"

#define CRYSTAL_MOBILE_CTRL 0x15
#define GBZ80_OPCODE_JUMP 0xC3
#define PAYLOAD_ENTRYPOINT_INTL 0xD288
#define PAYLOAD_ENTRYPOINT_JP 0xD2B9

#define SERIAL_MASTER 1
#define SERIAL_SLAVE 2

#define SERIAL_MAX_BUFFERS 4
#define SERIAL_PREAMBLE_LENGTH 6
#define SERIAL_RN_PREAMBLE_LENGTH 7
#define SERIAL_PATCH_PREAMBLE_LENGTH 3
#define SERIAL_RNS_LENGTH 10
#define SERIAL_MAIL_PREAMBLE_BYTE 0x20
#define SERIAL_MAIL_PREAMBLE_LENGTH 5

#define MAIL_STRUCT_LENGTH 0x2f
#define MAIL_STRUCT_LENGTH_JP 0x2a

#define PARTY_LENGTH 6
#define PARTYMON_STRUCT_LENGTH 0x30
#define SERIAL_PATCH_DATA_SIZE 0xFC
#define SERIAL_PREAMBLE_BYTE 0xFD
#define SERIAL_NO_DATA_BYTE 0xFE
#define SERIAL_PATCH_LIST_PART_TERMINATOR 0xFF
#define SERIAL_PATCH_REPLACEMENT_BYTE 0xFF

#define SERIAL_PATCH_LIST_LENGTH 200

struct LinkDataParams {
    u16 linkDataLength;
    u16 mailDataLength;
    u8 playerIdOffset;
    u8 mailEntryLength;
    u16 shellcodeEntryPoint;
    const u8 *shellcode;
};

struct LinkGSCTrade
{
    u8 state;
    u8 currentBufferIndex;
    u16 currenrBufferOffset;    
    u8 *buffers[SERIAL_MAX_BUFFERS];
    u16 bufferLengths[SERIAL_MAX_BUFFERS];
    NotifyStatusChangedCallback callback;
    u8 subState;
    u8 bufferCmd;
    u8 latestData;
    u8 timerState;
};

static struct LinkGSCTrade *sLinkGSCTrade;

extern const u8 gShellCode_Trade_GSC_JPN_Start[];
extern const u8 gShellCode_Trade_GSC_JPN_End[];
extern const u8 gShellCode_Trade_GSC_INTL_Start[];
extern const u8 gShellCode_Trade_GSC_INTL_End[];
extern const u8 gPayload_GSC_transfer_tool_Start[];
extern const u8 gPayload_GSC_transfer_tool_End[];

const struct LinkDataParams paramsJpn = {
    .linkDataLength = SERIAL_PREAMBLE_LENGTH + (NAME_LENGTH_GSC_JP + 1) + sizeof(u8) + PARTY_LENGTH + sizeof(u8) + sizeof(u16) + PARTYMON_STRUCT_LENGTH * PARTY_LENGTH + PARTY_LENGTH * (NAME_LENGTH_GSC_JP + 1) * 2 + 7,
    .playerIdOffset = SERIAL_PREAMBLE_LENGTH + (NAME_LENGTH_GSC_JP + 1) + sizeof(u8) + PARTY_LENGTH + sizeof(u8),
    .mailEntryLength = MAIL_STRUCT_LENGTH_JP,
    .mailDataLength = 0x122,
    .shellcodeEntryPoint = PAYLOAD_ENTRYPOINT_JP,
    .shellcode = gShellCode_Trade_GSC_JPN_Start
};

const struct LinkDataParams paramsIntl = {
    .linkDataLength = SERIAL_PREAMBLE_LENGTH + (NAME_LENGTH_GSC_INTL + 1) + sizeof(u8) + PARTY_LENGTH + sizeof(u8) + sizeof(u16) + PARTYMON_STRUCT_LENGTH * PARTY_LENGTH + PARTY_LENGTH * (NAME_LENGTH_GSC_INTL + 1) * 2 + 3,
    .playerIdOffset = SERIAL_PREAMBLE_LENGTH + (NAME_LENGTH_GSC_INTL + 1) + sizeof(u8) + PARTY_LENGTH + sizeof(u8),
    .mailEntryLength = MAIL_STRUCT_LENGTH,
    .mailDataLength = 0x186,
    .shellcodeEntryPoint = PAYLOAD_ENTRYPOINT_INTL,
    .shellcode = gShellCode_Trade_GSC_INTL_Start
};

static void CreateLinkData(u8 * linkData, const struct LinkDataParams * params) {
    u8 *nameBuff = linkData + SERIAL_PREAMBLE_LENGTH;
    u8 *payload = linkData + params->playerIdOffset + sizeof(u16);
    memset(linkData, 0x50, params->linkDataLength);
    memset(linkData, SERIAL_PREAMBLE_BYTE, SERIAL_PREAMBLE_LENGTH);
    nameBuff[0] = CRYSTAL_MOBILE_CTRL;
    nameBuff[1] = 0x00;
    nameBuff[2] = GBZ80_OPCODE_JUMP; // jp
    nameBuff[3] = params->shellcodeEntryPoint & 0xFF;
    nameBuff[4] = params->shellcodeEntryPoint >> 8;
    memcpy(payload, params->shellcode, gShellCode_Trade_GSC_JPN_End - gShellCode_Trade_GSC_JPN_Start);
}

static void CreatePatchList(u8 * patchList, u8 * playerData, const struct LinkDataParams * params) {
    u8 patchAreaLengths[2] = {SERIAL_PATCH_DATA_SIZE, sizeof(u16) + PARTYMON_STRUCT_LENGTH * PARTY_LENGTH - SERIAL_PATCH_DATA_SIZE};
    s32 i, j;
    memset(patchList, 0, SERIAL_PATCH_LIST_LENGTH);
    memset(patchList, SERIAL_PREAMBLE_BYTE, SERIAL_PATCH_PREAMBLE_LENGTH);
    patchList += SERIAL_RNS_LENGTH; //????? I can’t understand too
    playerData += params->playerIdOffset;
    for (i = 0; i < sizeof(patchAreaLengths); i++) {
        for (j = 0; j < patchAreaLengths[i]; j++) {
            if (playerData[j] == SERIAL_NO_DATA_BYTE) {
                playerData[j] = SERIAL_PATCH_REPLACEMENT_BYTE;
                *patchList++ = j + 1;
            }
        }
        playerData += SERIAL_PATCH_DATA_SIZE;
        *patchList++ = SERIAL_PATCH_LIST_PART_TERMINATOR;
    }
}

static void CreateMailData(u8 * mailData, const struct LinkDataParams * params) {
    memset(mailData, 0, params->mailDataLength);
    memset(mailData, SERIAL_MAIL_PREAMBLE_BYTE, SERIAL_MAIL_PREAMBLE_LENGTH);
    mailData += SERIAL_MAIL_PREAMBLE_LENGTH;
    mailData += params->mailEntryLength * PARTY_LENGTH;
    *mailData = 0xFF;
}

static void IntrTimer3_LinkGSCTrade(void)
{
    REG_TM3CNT_H &= ~TIMER_ENABLE;
    if (!sLinkGSCTrade->timerState) {
        REG_TM3CNT_L = -(8 * 0x1000000 / 64 / 1000);
        REG_SIOCNT |= SIO_ENABLE;
        REG_TM3CNT_H |= TIMER_ENABLE;
        sLinkGSCTrade->timerState = !sLinkGSCTrade->timerState;
    }
    else {
        REG_SIOCNT &= ~SIO_ENABLE;
        REG_SIODATA8 = sLinkGSCTrade->latestData;
        REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
        REG_TM3CNT_H |= TIMER_ENABLE;
        sLinkGSCTrade->timerState = !sLinkGSCTrade->timerState;
    }
}

static void IntrSerial_LinkGSCTrade(void) {
    static const u8 handshakeBytes[] = {0x0, 0x61, 0x0, 0xD1, 0x0};
    static const u8 enteringTradeViewLeadBytes[] = {0x75, 0x0, 0x76, 0x0};
    u8 data;
    REG_TM3CNT_H &= ~TIMER_ENABLE;

    data = REG_SIODATA8 & 0xFF;

    switch (sLinkGSCTrade->state)
    {
    case STATE_IDLE:
        data = SERIAL_NO_DATA_BYTE;
        break;
    case STATE_EXCHANGE_SERIAL_CLOCK:
        if (data == SERIAL_MASTER || data == SERIAL_SLAVE)
        {
            sLinkGSCTrade->currenrBufferOffset = 0;
            data = handshakeBytes[sLinkGSCTrade->currenrBufferOffset];
            sLinkGSCTrade->state++;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        else
            data = SERIAL_MASTER;
        break;
    case STATE_ENTERING_TRADE_ROOM_STANDBY:
        if (data == handshakeBytes[sLinkGSCTrade->currenrBufferOffset]) {
            sLinkGSCTrade->state++;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        data = handshakeBytes[sLinkGSCTrade->currenrBufferOffset];
        break;
    case STATE_ENTERING_TRADE_ROOM:
        if ((sLinkGSCTrade->currenrBufferOffset < sizeof(handshakeBytes) - 1 && data == SERIAL_NO_DATA_BYTE) || data == handshakeBytes[sLinkGSCTrade->currenrBufferOffset])
        {
            data = handshakeBytes[sLinkGSCTrade->currenrBufferOffset];
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
        }
        else
        {
            sLinkGSCTrade->currenrBufferOffset++;
            if (sLinkGSCTrade->currenrBufferOffset >= sizeof(handshakeBytes)) {
                sLinkGSCTrade->state = STATE_IDLE;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
                return;
            }
            else {
                sLinkGSCTrade->state = STATE_ENTERING_TRADE_ROOM_STANDBY;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
                data = handshakeBytes[sLinkGSCTrade->currenrBufferOffset];
            }
        }
        break;
    case STATE_ENTERING_TRADE_VIEW_STANDBY:
        if (data == enteringTradeViewLeadBytes[sLinkGSCTrade->currenrBufferOffset])
        {
            sLinkGSCTrade->state++;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        data = enteringTradeViewLeadBytes[sLinkGSCTrade->currenrBufferOffset];
        break;
    case STATE_ENTERING_TRADE_VIEW:
        if (data == SERIAL_NO_DATA_BYTE || data == enteringTradeViewLeadBytes[sLinkGSCTrade->currenrBufferOffset]) {
            data = enteringTradeViewLeadBytes[sLinkGSCTrade->currenrBufferOffset];
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
        }
        else
        {
            sLinkGSCTrade->currenrBufferOffset++;
            if (sLinkGSCTrade->currenrBufferOffset >= sizeof(enteringTradeViewLeadBytes)) {
                sLinkGSCTrade->state++;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
                sLinkGSCTrade->currenrBufferOffset = 0;
                sLinkGSCTrade->currentBufferIndex = 0;
                data = SERIAL_NO_DATA_BYTE;
            }
            else {
                sLinkGSCTrade->state = STATE_ENTERING_TRADE_VIEW_STANDBY;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
                data = enteringTradeViewLeadBytes[sLinkGSCTrade->currenrBufferOffset];
            }
        }
        break;
    case STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY:
    STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY_CASE:
        data = sLinkGSCTrade->buffers[sLinkGSCTrade->currentBufferIndex][sLinkGSCTrade->currenrBufferOffset];
        if (sLinkGSCTrade->currentBufferIndex == 3)
            sLinkGSCTrade->state += 2;
        else
            sLinkGSCTrade->state++;
        if (sLinkGSCTrade->callback)
            sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        break;
    case STATE_EXCHANGE_DATA_LEAD_BYTE:
        if (data == SERIAL_PREAMBLE_BYTE)
            sLinkGSCTrade->state++;
        else
        {
            data = sLinkGSCTrade->buffers[sLinkGSCTrade->currentBufferIndex][sLinkGSCTrade->currenrBufferOffset];
            break;
        }
    case STATE_EXCHANGE_DATA:
        if (sLinkGSCTrade->currenrBufferOffset < sLinkGSCTrade->bufferLengths[sLinkGSCTrade->currentBufferIndex])
            data = sLinkGSCTrade->buffers[sLinkGSCTrade->currentBufferIndex][sLinkGSCTrade->currenrBufferOffset++];
        else
        {
            sLinkGSCTrade->currentBufferIndex++;
            if (sLinkGSCTrade->currentBufferIndex >= SERIAL_MAX_BUFFERS)
            {
                sLinkGSCTrade->state++;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
                data = 0;
            }
            else
            {
                sLinkGSCTrade->state = STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY;
                sLinkGSCTrade->currenrBufferOffset = 0;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
                goto STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY_CASE;
            }
        }
        if (sLinkGSCTrade->callback)
            sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
        break;
    case STATE_SENDING_PAYLOAD_STANDBY:
        if ((data & 0xF0) == 0xC0) {
            sLinkGSCTrade->state++;
            sLinkGSCTrade->bufferCmd = data;
            sLinkGSCTrade->subState = 0;
            sLinkGSCTrade->currenrBufferOffset = (data & 0x0F) * 0x100;
        }
        else if (data == 0xD0) {
            sLinkGSCTrade->state = STATE_SENDING_PAYLOAD_COMPLETELY;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
            break;
        }
        else {
            data = 0;
            break;
        }
    case STATE_SENDING_PAYLOAD:
        if (data != sLinkGSCTrade->bufferCmd) {
            data = 0;
            sLinkGSCTrade->state = STATE_SENDING_PAYLOAD_STANDBY;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        else {
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
            if (sLinkGSCTrade->subState < 3) {
                if ((sLinkGSCTrade->currenrBufferOffset - (data & 0x0F) * 0x100) == 0x100)
                    sLinkGSCTrade->subState = 3;
            }
            if (sLinkGSCTrade->subState == 0) {
                sLinkGSCTrade->subState++;
                data = 0xFD;
            }
            else if (sLinkGSCTrade->subState == 1) {
                data = gPayload_GSC_transfer_tool_Start[sLinkGSCTrade->currenrBufferOffset];
                if (data == 0xFE || data == 0xFF) {
                    sLinkGSCTrade->subState++;
                    data = 0xFF;
                }
                else
                    sLinkGSCTrade->currenrBufferOffset++;
            }
            else if (sLinkGSCTrade->subState == 2) {
                sLinkGSCTrade->subState = 1;
                data = gPayload_GSC_transfer_tool_Start[sLinkGSCTrade->currenrBufferOffset++];
            }
            else if (sLinkGSCTrade->subState == 3) {
                sLinkGSCTrade->subState++;
                data = 0xFE;
            }
            else if (sLinkGSCTrade->subState == 4) {
                data = CalcCRC8(&gPayload_GSC_transfer_tool_Start[(sLinkGSCTrade->bufferCmd & 0x0F) * 0x100], 0x100);
                sLinkGSCTrade->state = STATE_SENDING_PAYLOAD_STANDBY;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
            }
        }
        break;
    case STATE_SENDING_PAYLOAD_COMPLETELY:
        return;
    }

    sLinkGSCTrade->latestData = data;
    REG_SIODATA8 = data;
    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_TM3CNT_H |= TIMER_ENABLE;
    sLinkGSCTrade->timerState = 0;
}

void TryHandshakeWithGSC() {
    sLinkGSCTrade->state = STATE_EXCHANGE_SERIAL_CLOCK;

    REG_RCNT = 0;
    REG_SIOCNT = SIO_INTR_ENABLE | SIO_8BIT_MODE | 1;
    REG_SIODATA8 = SERIAL_MASTER;
    sLinkGSCTrade->latestData = SERIAL_MASTER;

    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_TM3CNT_H = TIMER_INTR_ENABLE | TIMER_64CLK;
    REG_TM3CNT_H |= TIMER_ENABLE;

    sLinkGSCTrade->timerState = 0;
}

void TryEnteringGSCTradeView() {
    sLinkGSCTrade->state = STATE_ENTERING_TRADE_VIEW_STANDBY;
    sLinkGSCTrade->currenrBufferOffset = 0;

    REG_RCNT = 0;
    REG_SIOCNT = SIO_INTR_ENABLE | SIO_8BIT_MODE | 1;
    REG_SIODATA8 = 0;
    sLinkGSCTrade->latestData = 0;

    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_TM3CNT_H = TIMER_INTR_ENABLE | TIMER_64CLK;
    REG_TM3CNT_H |= TIMER_ENABLE;

    sLinkGSCTrade->timerState = 0;
}

void ExitLinkGSCTrade() {
    u16 imeTemp;
    s32 i;
    DisableInterrupts(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    imeTemp = REG_IME;
    REG_IME = 0;
    REG_IF = INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL;
    REG_TM3CNT_H = 0;
    REG_SIOCNT = REG_SIOCNT & ~SIO_INTR_ENABLE;
    if (sLinkGSCTrade) {
        for (i = 0; i < SERIAL_MAX_BUFFERS; i++)
            if (sLinkGSCTrade->buffers[i])
                FREE_AND_SET_NULL(sLinkGSCTrade->buffers[i]);
        FREE_AND_SET_NULL(sLinkGSCTrade);
    }
    gIntrTable[1] = NULL;
    gIntrTable[2] = NULL;

    REG_IME = imeTemp;
}

void SetupLinkGSCTrade(bool8 isJPN, NotifyStatusChangedCallback callback) {
    const struct LinkDataParams *params = isJPN ? &paramsJpn : &paramsIntl;
    s32 i;
    if (sLinkGSCTrade)
        return;
    sLinkGSCTrade = (struct LinkGSCTrade*)AllocZeroed(sizeof(struct LinkGSCTrade));
    sLinkGSCTrade->callback = callback;
    sLinkGSCTrade->bufferLengths[0] = SERIAL_RN_PREAMBLE_LENGTH + SERIAL_RNS_LENGTH;
    sLinkGSCTrade->bufferLengths[1] = params->linkDataLength;
    sLinkGSCTrade->bufferLengths[2] = SERIAL_PATCH_LIST_LENGTH;
    sLinkGSCTrade->bufferLengths[3] = params->mailDataLength;

    for (i = 0; i < SERIAL_MAX_BUFFERS; i++) 
        sLinkGSCTrade->buffers[i] = (u8 *)AllocZeroed(sLinkGSCTrade->bufferLengths[i]);

    for (i = 0; i < SERIAL_RN_PREAMBLE_LENGTH; i++) 
        sLinkGSCTrade->buffers[0][i] = SERIAL_PREAMBLE_BYTE;

    for (i = SERIAL_RN_PREAMBLE_LENGTH; i < SERIAL_RN_PREAMBLE_LENGTH + SERIAL_RNS_LENGTH; i++)
        sLinkGSCTrade->buffers[0][i] = (u8)i;

    CreateLinkData(sLinkGSCTrade->buffers[1], params);
    CreatePatchList(sLinkGSCTrade->buffers[2], sLinkGSCTrade->buffers[1], params);
    CreateMailData(sLinkGSCTrade->buffers[3], params);

    gIntrTable[1] = IntrSerial_LinkGSCTrade;
    gIntrTable[2] = IntrTimer3_LinkGSCTrade;
    IntrEnable(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
}