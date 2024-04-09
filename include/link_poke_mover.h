#ifndef GUARD_LINK_POKE_MOVER_H
#define GUARD_LINK_POKE_MOVER_H

#define MAX_BUFFER_SIZE 256

enum {
    TRANSFER_IDLE,
    TRANSFER_BUSY,
    TRANSFER_FAILED,
    TRANSFER_COMPLETE,
    TRANSFER_WAIT_SYNC,
    TRANSFER_SYNC_COMPLETE,
};

enum {
    STATE_IDLE,
    STATE_EXCHANGE_SERIAL_CLOCK,
    STATE_ENTERING_TRADE_ROOM_STANDBY,
    STATE_ENTERING_TRADE_ROOM,
    STATE_ENTERING_TRADE_VIEW_STANDBY,
    STATE_ENTERING_TRADE_VIEW,
    STATE_EXCHANGE_DATA_LEAD_BYTE_STANDBY,
    STATE_EXCHANGE_DATA_LEAD_BYTE,
    STATE_EXCHANGE_DATA,
    STATE_SENDING_PAYLOAD_STANDBY,
    STATE_SENDING_PAYLOAD,
    STATE_SENDING_PAYLOAD_COMPLETELY
};

typedef void (*NotifyStatusChangedCallback)(u8 changedStatus, bool8 dataHasReached);

u8 CalcCRC8(const u8 *data, int len);

void SetupLinkPokeMover(NotifyStatusChangedCallback callbak);
void ExecBufferCommand(u8 cmd);
void ExecEraseCommand(u8 cmd, u8 *buffer, u16 length);
void FinishSyncErase();
u8 *GetLinkPokeMoverBuffer();
void ExitLinkPokeMover();

void SetupLinkGSCTrade(bool8 isJPN, NotifyStatusChangedCallback callback);
void TryHandshakeWithGSC();
void TryEnteringGSCTradeView();
void ExitLinkGSCTrade();

#endif