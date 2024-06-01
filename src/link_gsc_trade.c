#include "define_gsc.h"
#include "global.h"
#include "gpu_regs.h"
#include "link_poke_mover.h"
#include "main.h"
#include "malloc.h"

#define CRYSTAL_MOBILE_CTRL 0x15
#define GBZ80_OPCODE_JUMP 0xC3
#define SHELLCODE_ENTRYPOINT_CRYSTAL_INTL 0xD288
#define SHELLCODE_ENTRYPOINT_CRYSTAL_JPN 0xD2B9
#define SHELLCODE_ENTRYPOINT_GS_INTL 0xDDFE
#define SHELLCODE_ENTRYPOINT_GS_JPN 0xDD7F
#define SHELLCODE_ENTRYPOINT_GS_KOR 0xDEDE

#define SERIAL_MASTER 1
#define SERIAL_SLAVE 2

#define SERIAL_MAX_BUFFERS 4
#define SERIAL_PREAMBLE_LENGTH 6
#define SERIAL_RN_PREAMBLE_LENGTH 7
#define SERIAL_PATCH_PREAMBLE_LENGTH 3
#define SERIAL_RNS_LENGTH 10
#define SERIAL_MAIL_PREAMBLE_BYTE 0x20
#define SERIAL_MAIL_PREAMBLE_BYTE_GS_KOR 0x52
#define SERIAL_MAIL_PREAMBLE_LENGTH 5

#define MAIL_STRUCT_LENGTH 0x2f
#define MAIL_STRUCT_LENGTH_JPN 0x2a
#define MAIL_STRUCT_LENGTH_KOR 0x4f

#define PARTY_LENGTH 6
#define PARTYMON_STRUCT_LENGTH 0x30
#define SERIAL_PATCH_DATA_SIZE 0xFC
#define SERIAL_PREAMBLE_BYTE 0xFD
#define SERIAL_NO_DATA_BYTE 0xFE
#define SERIAL_PATCH_LIST_PART_TERMINATOR 0xFF
#define SERIAL_PATCH_REPLACEMENT_BYTE 0xFF

#define SERIAL_PATCH_LIST_LENGTH 200

struct LinkGSCPlayerDataJPN {
    u8 preambleBytes[SERIAL_PREAMBLE_LENGTH];
    u8 playerName[NAME_LENGTH_GSC_JP + 1];
    u8 partyConut;
    u8 partySpecies[PARTY_LENGTH];
    u8 partyTerminator;
    u16 trainerId;
    u8 partyMon[PARTY_LENGTH][PARTYMON_STRUCT_LENGTH];
    u8 partyMonNames[PARTY_LENGTH][NAME_LENGTH_GSC_JP + 1];
    u8 partyMonOtNames[PARTY_LENGTH][NAME_LENGTH_GSC_JP + 1];
    u8 padding[7];
} __attribute__((packed));

struct LinkGSCPlayerDataINTL {
    u8 preambleBytes[SERIAL_PREAMBLE_LENGTH];
    u8 playerName[NAME_LENGTH_GSC_INTL + 1];
    u8 partyConut;
    u8 partySpecies[PARTY_LENGTH];
    u8 partyTerminator;
    u16 trainerId;
    u8 partyMon[PARTY_LENGTH][PARTYMON_STRUCT_LENGTH];
    u8 partyMonNames[PARTY_LENGTH][NAME_LENGTH_GSC_INTL + 1];
    u8 partyMonOtNames[PARTY_LENGTH][NAME_LENGTH_GSC_INTL + 1];
    u8 padding[3];
} __attribute__((packed));

struct ShellCodeParams {
    u16 shellcodeEntryPoint;
    u16 shellCodeLength;
    const u8 *shellcode;
    u8 lineChar;
    u8 nextLineChar;
    u8 nextLineCharsNum;
    u8 entrypointNum;
};

struct LinkDataParams {
    u16 linkDataLength;
    u16 mailDataLength;
    u8 playerIdOffset;
    u8 mailEntryLength;
};

struct LinkGSCTrade
{
    u8 state;
    u8 currentBufferIndex;
    u16 currentBufferOffset;
    u8 *buffers[SERIAL_MAX_BUFFERS];
    u16 bufferLengths[SERIAL_MAX_BUFFERS];
    NotifyStatusChangedCallback callback;
    u8 subState;
    u8 bufferCmd;
    u8 latestData;
    u8 timerState;
    u8 *payload;
};

static struct LinkGSCTrade *sLinkGSCTrade;

const u8 gShellCode_Trade_GS_INTL[] = INCBIN_U8("data/shellcode_trade_gs_intl.bin");
const u8 gShellCode_Trade_GS_JPN[] = INCBIN_U8("data/shellcode_trade_gs_jpn.bin");
const u8 gShellCode_Trade_GS_KOR[] = INCBIN_U8("data/shellcode_trade_gs_kor.bin");
const u8 gShellCode_Trade_Crystal_JPN[] = INCBIN_U8("data/shellcode_trade_gsc_jpn.bin");
const u8 gShellCode_Trade_Crystal_INTL[] = INCBIN_U8("data/shellcode_trade_gsc_intl.bin");
const u32 gPayload_GSC_transfer_tool[] = INCBIN_U32("data/payload_GSC_transfer_tool.bin.lz");


static const struct LinkDataParams sLinkDataParamsGSCJPN = {
    .linkDataLength = sizeof(struct LinkGSCPlayerDataJPN),
    .playerIdOffset = offsetof(struct LinkGSCPlayerDataJPN, trainerId),
    .mailEntryLength = MAIL_STRUCT_LENGTH_JPN,
    .mailDataLength = 0x122
};

static const struct LinkDataParams sLinkDataParamsGSCINTL = {
    .linkDataLength = sizeof(struct LinkGSCPlayerDataINTL),
    .playerIdOffset = offsetof(struct LinkGSCPlayerDataINTL, trainerId),
    .mailEntryLength = MAIL_STRUCT_LENGTH,
    .mailDataLength = 0x186
};

static const struct LinkDataParams sLinkDataParamsGSKOR = {
    .linkDataLength = sizeof(struct LinkGSCPlayerDataINTL),
    .playerIdOffset = offsetof(struct LinkGSCPlayerDataINTL, trainerId),
    .mailEntryLength = MAIL_STRUCT_LENGTH_KOR,
    .mailDataLength = 0x234
};

static const struct ShellCodeParams sShellCodeParamsGSJPN = {
    .shellcodeEntryPoint = SHELLCODE_ENTRYPOINT_GS_JPN,
    .shellCodeLength = sizeof(gShellCode_Trade_GS_JPN),
    .shellcode = gShellCode_Trade_GS_JPN,
    .lineChar = 0x4F,
    .nextLineChar = 0x4E,
    .nextLineCharsNum = 0x9F,
    .entrypointNum = 0x10
};

static const struct ShellCodeParams sShellCodeParamsGSINTL = {
    .shellcodeEntryPoint = SHELLCODE_ENTRYPOINT_GS_INTL,
    .shellCodeLength = sizeof(gShellCode_Trade_GS_INTL),
    .shellcode = gShellCode_Trade_GS_INTL,
    .lineChar = 0x4F,
    .nextLineChar = 0x4E,
    .nextLineCharsNum = 0xA4,
    .entrypointNum = 0x8
};

static const struct ShellCodeParams sShellCodeParamsGSKOR = {
    .shellcodeEntryPoint = SHELLCODE_ENTRYPOINT_GS_KOR,
    .shellCodeLength = sizeof(gShellCode_Trade_GS_KOR),
    .shellcode = gShellCode_Trade_GS_KOR,
    .lineChar = 0x5A,
    .nextLineChar = 0x59,
    .nextLineCharsNum = 0x3D,
    .entrypointNum = 0x2D
};

static const struct ShellCodeParams sShellCodeParamsCrystalJPN = {
    .shellcodeEntryPoint = SHELLCODE_ENTRYPOINT_CRYSTAL_JPN,
    .shellCodeLength = sizeof(gShellCode_Trade_Crystal_JPN),
    .shellcode = gShellCode_Trade_Crystal_JPN
};

static const struct ShellCodeParams sShellCodeParamsCrystalINTL = {
    .shellcodeEntryPoint = SHELLCODE_ENTRYPOINT_CRYSTAL_INTL,
    .shellCodeLength = sizeof(gShellCode_Trade_Crystal_INTL),
    .shellcode = gShellCode_Trade_Crystal_INTL
};

static void CreateLinkData(u8 * linkData, const struct LinkDataParams * linkDataParams, const struct ShellCodeParams * shellCodeParams, bool8 useStackSmashing) {
    u8 *nameBuff = linkData + SERIAL_PREAMBLE_LENGTH;
    u8 *payload = linkData + linkDataParams->playerIdOffset + sizeof(u16);
    memset(linkData, 0x50, linkDataParams->linkDataLength);
    memset(linkData, SERIAL_PREAMBLE_BYTE, SERIAL_PREAMBLE_LENGTH);
    if (!useStackSmashing) {
        nameBuff[0] = CRYSTAL_MOBILE_CTRL;
        nameBuff[1] = 0x00;
        nameBuff[2] = GBZ80_OPCODE_JUMP; // jp
        nameBuff[3] = shellCodeParams->shellcodeEntryPoint & 0xFF;
        nameBuff[4] = shellCodeParams->shellcodeEntryPoint >> 8;
        memcpy(payload, shellCodeParams->shellcode, shellCodeParams->shellCodeLength);
    }
    else {
        *nameBuff++ = shellCodeParams->lineChar;
        memset(nameBuff, shellCodeParams->nextLineChar, shellCodeParams->nextLineCharsNum);
        nameBuff += shellCodeParams->nextLineCharsNum;
        for (s32 i = 0; i < shellCodeParams->entrypointNum; i++) {
            *nameBuff++ = shellCodeParams->shellcodeEntryPoint & 0xFF;
            *nameBuff++ = shellCodeParams->shellcodeEntryPoint >> 8;
        }
        *nameBuff++ = 0x50;
        memcpy(nameBuff, shellCodeParams->shellcode, shellCodeParams->shellCodeLength);
    }
}

static void CreatePatchList(u8 * patchList, u8 * playerData, const struct LinkDataParams * params) {
    u8 patchAreaLengths[2] = {SERIAL_PATCH_DATA_SIZE, params->linkDataLength - SERIAL_PATCH_DATA_SIZE};
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

static void CreateMailData(u8 * mailData, const struct LinkDataParams * params, u8 language) {
    u8 preambleByte = language == LANGUAGE_KOREAN ? SERIAL_MAIL_PREAMBLE_BYTE_GS_KOR : SERIAL_MAIL_PREAMBLE_BYTE;
    memset(mailData, 0, params->mailDataLength);
    memset(mailData, preambleByte, SERIAL_MAIL_PREAMBLE_LENGTH);
    mailData += SERIAL_MAIL_PREAMBLE_LENGTH;
    mailData += params->mailEntryLength * PARTY_LENGTH;
    *mailData = 0xFF;
}

static void IntrTimer3_LinkGSCTrade(void)
{
    static u32 counterLimit;
    switch (sLinkGSCTrade->timerState)
    {
    case 0:
        sLinkGSCTrade->timerState++;
        counterLimit = gMain.vblankCounter1 + 60;
        REG_SIOCNT &= ~SIO_ENABLE;
        REG_SIODATA8 = sLinkGSCTrade->latestData;
        break;
    case 1:
        sLinkGSCTrade->timerState++;
        REG_SIOCNT |= SIO_ENABLE;
        break;
    case 2:
        if ((s32)(counterLimit - gMain.vblankCounter1) <= 0)
            sLinkGSCTrade->timerState = 0;
        break;
    }
    INTR_CHECK |= INTR_FLAG_TIMER3;
}

static bool8 HandleReceivedByte(u8 *dataPtr) {
    static const u8 handshakeBytes[] = {0x0, 0x61, 0x0, 0xD1, 0x0};
    static const u8 enteringTradeViewLeadBytes[] = {0x75, 0x0, 0x76, 0x0};
    u8 data = *dataPtr;

    switch (sLinkGSCTrade->state)
    {
    case STATE_IDLE:
        data = SERIAL_NO_DATA_BYTE;
        break;
    case STATE_EXCHANGE_SERIAL_CLOCK:
        if (data == SERIAL_MASTER || data == SERIAL_SLAVE)
        {
            sLinkGSCTrade->currentBufferOffset = 0;
            data = handshakeBytes[sLinkGSCTrade->currentBufferOffset];
            sLinkGSCTrade->state++;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        else
            data = SERIAL_MASTER;
        break;
    case STATE_ENTERING_TRADE_ROOM_STANDBY:
        if (data == handshakeBytes[sLinkGSCTrade->currentBufferOffset]) {
            sLinkGSCTrade->state++;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        data = handshakeBytes[sLinkGSCTrade->currentBufferOffset];
        break;
    case STATE_ENTERING_TRADE_ROOM:
        if ((sLinkGSCTrade->currentBufferOffset < sizeof(handshakeBytes) - 1 && data == SERIAL_NO_DATA_BYTE) || data == handshakeBytes[sLinkGSCTrade->currentBufferOffset])
        {
            data = handshakeBytes[sLinkGSCTrade->currentBufferOffset];
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
        }
        else
        {
            sLinkGSCTrade->currentBufferOffset++;
            if (sLinkGSCTrade->currentBufferOffset >= sizeof(handshakeBytes)) {
                sLinkGSCTrade->state = STATE_IDLE;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
                return FALSE;
            }
            else {
                sLinkGSCTrade->state = STATE_ENTERING_TRADE_ROOM_STANDBY;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
                data = handshakeBytes[sLinkGSCTrade->currentBufferOffset];
            }
        }
        break;
    case STATE_ENTERING_TRADE_VIEW_STANDBY:
        if (data == enteringTradeViewLeadBytes[sLinkGSCTrade->currentBufferOffset])
        {
            sLinkGSCTrade->state++;
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
        }
        data = enteringTradeViewLeadBytes[sLinkGSCTrade->currentBufferOffset];
        break;
    case STATE_ENTERING_TRADE_VIEW:
        if (data == SERIAL_NO_DATA_BYTE || data == enteringTradeViewLeadBytes[sLinkGSCTrade->currentBufferOffset]) {
            data = enteringTradeViewLeadBytes[sLinkGSCTrade->currentBufferOffset];
            if (sLinkGSCTrade->callback)
                sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
        }
        else
        {
            sLinkGSCTrade->currentBufferOffset++;
            if (sLinkGSCTrade->currentBufferOffset >= sizeof(enteringTradeViewLeadBytes)) {
                sLinkGSCTrade->state++;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, FALSE);
                sLinkGSCTrade->currentBufferOffset = 0;
                sLinkGSCTrade->currentBufferIndex = 0;
                data = SERIAL_NO_DATA_BYTE;
            }
            else {
                sLinkGSCTrade->state = STATE_ENTERING_TRADE_VIEW_STANDBY;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
                data = enteringTradeViewLeadBytes[sLinkGSCTrade->currentBufferOffset];
            }
        }
        break;
    case STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY:
    STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY_CASE:
        data = sLinkGSCTrade->buffers[sLinkGSCTrade->currentBufferIndex][sLinkGSCTrade->currentBufferOffset];
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
            data = sLinkGSCTrade->buffers[sLinkGSCTrade->currentBufferIndex][sLinkGSCTrade->currentBufferOffset];
            break;
        }
    case STATE_EXCHANGE_DATA:
        if (sLinkGSCTrade->currentBufferOffset < sLinkGSCTrade->bufferLengths[sLinkGSCTrade->currentBufferIndex])
            data = sLinkGSCTrade->buffers[sLinkGSCTrade->currentBufferIndex][sLinkGSCTrade->currentBufferOffset++];
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
                sLinkGSCTrade->currentBufferOffset = 0;
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
            sLinkGSCTrade->currentBufferOffset = (data & 0x0F) * 0x100;
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
                if ((sLinkGSCTrade->currentBufferOffset - (data & 0x0F) * 0x100) == 0x100)
                    sLinkGSCTrade->subState = 3;
            }
            if (sLinkGSCTrade->subState == 0) {
                sLinkGSCTrade->subState++;
                data = 0xFD;
            }
            else if (sLinkGSCTrade->subState == 1) {
                data = sLinkGSCTrade->payload[sLinkGSCTrade->currentBufferOffset];
                if (data == 0xFE || data == 0xFF) {
                    sLinkGSCTrade->subState++;
                    data = 0xFF;
                }
                else
                    sLinkGSCTrade->currentBufferOffset++;
            }
            else if (sLinkGSCTrade->subState == 2) {
                sLinkGSCTrade->subState = 1;
                data = sLinkGSCTrade->payload[sLinkGSCTrade->currentBufferOffset++];
            }
            else if (sLinkGSCTrade->subState == 3) {
                sLinkGSCTrade->subState++;
                data = 0xFE;
            }
            else if (sLinkGSCTrade->subState == 4) {
                data = CalcCRC8(&sLinkGSCTrade->payload[(sLinkGSCTrade->bufferCmd & 0x0F) * 0x100], 0x100);
                sLinkGSCTrade->state = STATE_SENDING_PAYLOAD_STANDBY;
                if (sLinkGSCTrade->callback)
                    sLinkGSCTrade->callback(sLinkGSCTrade->state, TRUE);
            }
        }
        break;
    case STATE_SENDING_PAYLOAD_COMPLETELY:
        return FALSE;
    }

    *dataPtr = data;

    return TRUE;
}

static void IntrSerial_LinkGSCTrade(void) {
    u8 data;
    REG_TM3CNT_H &= ~TIMER_ENABLE;

    data = REG_SIODATA8 & 0xFF;

    if (!HandleReceivedByte(&data))
        return;

    sLinkGSCTrade->latestData = data;
    REG_SIODATA8 = data;
    REG_TM3CNT_L = -(2 * 0x1000000 / 64 / 1000);
    REG_TM3CNT_H |= TIMER_ENABLE;
    sLinkGSCTrade->timerState = 0;

    INTR_CHECK |= INTR_FLAG_SERIAL;
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
    sLinkGSCTrade->currentBufferOffset = 0;

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
        if (sLinkGSCTrade->payload)
            FREE_AND_SET_NULL(sLinkGSCTrade->payload);
        for (i = 0; i < SERIAL_MAX_BUFFERS; i++)
            if (sLinkGSCTrade->buffers[i])
                FREE_AND_SET_NULL(sLinkGSCTrade->buffers[i]);
        FREE_AND_SET_NULL(sLinkGSCTrade);
    }
    gIntrTable[1] = NULL;
    gIntrTable[2] = NULL;

    REG_IME = imeTemp;
}

void SetupLinkGSCTrade(u8 language, bool8 isGS, NotifyStatusChangedCallback callback) {
    const struct LinkDataParams *linkDataParams;
    const struct ShellCodeParams *shellCodeParams;
    s32 i;
    if (language == LANGUAGE_JAPANESE)
        linkDataParams = &sLinkDataParamsGSCJPN;
    else if (language == LANGUAGE_KOREAN)
        linkDataParams = &sLinkDataParamsGSKOR;
    else
        linkDataParams = &sLinkDataParamsGSCINTL;
    if (isGS) {
        if (language == LANGUAGE_JAPANESE)
            shellCodeParams = &sShellCodeParamsGSJPN;
        else if (language == LANGUAGE_KOREAN)
            shellCodeParams = &sShellCodeParamsGSKOR;
        else
            shellCodeParams = &sShellCodeParamsGSINTL;
    }
    else {
        if (language == LANGUAGE_JAPANESE)
            shellCodeParams = &sShellCodeParamsCrystalJPN;
        else
            shellCodeParams = &sShellCodeParamsCrystalINTL;
    }

    if (sLinkGSCTrade)
        return;
    sLinkGSCTrade = (struct LinkGSCTrade*)AllocZeroed(sizeof(struct LinkGSCTrade));
    sLinkGSCTrade->callback = callback;
    sLinkGSCTrade->bufferLengths[0] = SERIAL_RN_PREAMBLE_LENGTH + SERIAL_RNS_LENGTH;
    sLinkGSCTrade->bufferLengths[1] = linkDataParams->linkDataLength;
    sLinkGSCTrade->bufferLengths[2] = SERIAL_PATCH_LIST_LENGTH;
    sLinkGSCTrade->bufferLengths[3] = linkDataParams->mailDataLength;
    sLinkGSCTrade->payload = (u8 *)Alloc(gPayload_GSC_transfer_tool[0] >> 8);
    LZ77UnCompWram(gPayload_GSC_transfer_tool, sLinkGSCTrade->payload);

    for (i = 0; i < SERIAL_MAX_BUFFERS; i++) 
        sLinkGSCTrade->buffers[i] = (u8 *)AllocZeroed(sLinkGSCTrade->bufferLengths[i]);

    for (i = 0; i < SERIAL_RN_PREAMBLE_LENGTH; i++) 
        sLinkGSCTrade->buffers[0][i] = SERIAL_PREAMBLE_BYTE;

    for (i = SERIAL_RN_PREAMBLE_LENGTH; i < SERIAL_RN_PREAMBLE_LENGTH + SERIAL_RNS_LENGTH; i++)
        sLinkGSCTrade->buffers[0][i] = (u8)i;

    CreateLinkData(sLinkGSCTrade->buffers[1], linkDataParams, shellCodeParams, isGS);
    CreatePatchList(sLinkGSCTrade->buffers[2], sLinkGSCTrade->buffers[1], linkDataParams);
    CreateMailData(sLinkGSCTrade->buffers[3], linkDataParams, language);

    gIntrTable[1] = IntrSerial_LinkGSCTrade;
    gIntrTable[2] = IntrTimer3_LinkGSCTrade;
    IntrEnable(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
}