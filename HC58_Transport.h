#pragma once

#include <Arduino.h>

namespace HC58Transport {

constexpr uint8_t MAX_FRAME_BYTES = 20;

enum class Client : uint8_t {
    None = 0,
    PID,
    DTC
};

enum class TxStatus : uint8_t {
    NotInvolved = 0,
    Underrun     = 1,
    LostArb     = 2,
    Successful  = 3
};

struct Completion {
    uint8_t raw;
    bool error;
    bool rxFifoOverrun;
    TxStatus txStatus;
    bool isIfr;
    bool ifrHasCrc;
    uint8_t errorCode;
};

struct Frame {
    uint8_t bytes[MAX_FRAME_BYTES];
    uint8_t length;
    uint8_t completionStatus;
    Completion completion;
    uint32_t completedAtUs;
    bool truncated;
};

struct Stats {
    uint32_t irqEdges;
    uint32_t spiExchanges;
    uint32_t nonEmptyRxItems;
    uint32_t completedFrames;
    uint32_t deliveredFrames;

    uint32_t framesWithError;
    uint32_t rxFifoOverruns;
    uint32_t truncatedFrames;
    uint32_t networkFaults;

    uint32_t txSuccessful;
    uint32_t txLostArbitration;
    uint32_t txUnderrun;

    uint32_t serviceBudgetHits;
    uint32_t stuckLowRecoveries;
    uint32_t ownerConflicts;
    uint32_t hardResets;
};

using FrameSink = void (*)(const Frame &frame);

bool begin(FrameSink sink);
void service();

bool claim(Client client);
void release(Client client);
bool isOwnedBy(Client client);
Client owner();

bool sendFrame(Client client, const uint8_t *bytes, uint8_t length);

bool isReady();
bool intAsserted();

Stats snapshotStats();
void clearStats();
bool hardReset();

}  // namespace HC58Transport
