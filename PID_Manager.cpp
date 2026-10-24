#include "PID_Manager.h"

#include <Arduino.h>
#include <string.h>

namespace {

constexpr uint16_t POLL_GAP_MS = 10;
constexpr uint16_t TX_TIMEOUT_MS = 250;
constexpr uint16_t RESPONSE_TIMEOUT_MS = 250;

constexpr uint8_t PID_CALC_LOAD = 0x04;
constexpr uint8_t PID_STFT1     = 0x06;
constexpr uint8_t PID_LTFT1     = 0x07;
constexpr uint8_t PID_STFT2     = 0x08;
constexpr uint8_t PID_LTFT2     = 0x09;
constexpr uint8_t PID_MAP       = 0x0B;
constexpr uint8_t PID_TIMING    = 0x0E;
constexpr uint8_t PID_IAT       = 0x0F;
constexpr uint8_t PID_MAF       = 0x10;
constexpr uint8_t PID_TPS       = 0x11;
constexpr uint8_t PID_O2_B1     = 0x14;
constexpr uint8_t PID_O2_B2     = 0x15;

enum class RequestState : uint8_t {
    Idle = 0,
    WaitTxEcho,
    WaitResponse
};

PageID activePage = PAGE_BOOTLOGO;
char slotValues[PID_SLOT_COUNT][8];
bool slotValid[PID_SLOT_COUNT];

RequestState requestState = RequestState::Idle;
uint8_t requestedPid = 0;
PIDSlot requestedSlot = PID_SLOT_Q1;
uint8_t requestBytes[5] = {0};
uint32_t requestDeadlineMs = 0;
uint32_t nextPollMs = 0;
uint8_t pollIndex = 0;

bool deadlineReached(uint32_t deadline) {
    return static_cast<int32_t>(millis() - deadline) >= 0;
}

bool isPidPage(PageID page) {
    return page == PAGE_PID1 || page == PAGE_PID2 || page == PAGE_PID3;
}

void copyValue(char *dst, const char *src, size_t dstSize) {
    if (!dst || !src || dstSize == 0) return;
    snprintf(dst, dstSize, "%s", src);
}

void clearAllSlots() {
    for (int i = 0; i < PID_SLOT_COUNT; ++i) {
        slotValues[i][0] = '\0';
        slotValid[i] = false;
    }
}

void clearSlot(PIDSlot slot) {
    if (slot < 0 || slot >= PID_SLOT_COUNT) return;
    slotValues[slot][0] = '\0';
    slotValid[slot] = false;
}

void setSlotValue(PIDSlot slot, const char *value) {
    if (slot < 0 || slot >= PID_SLOT_COUNT || !value) return;
    copyValue(slotValues[slot], value, sizeof(slotValues[slot]));
    slotValid[slot] = true;
}

void releaseRequest() {
    HC58Transport::release(HC58Transport::Client::PID);
    requestState = RequestState::Idle;
    requestedPid = 0;
    requestDeadlineMs = 0;
    nextPollMs = millis() + POLL_GAP_MS;
}

void failRequest() {
    clearSlot(requestedSlot);
    releaseRequest();
}

void cancelRequest() {
    if (requestState != RequestState::Idle ||
        HC58Transport::isOwnedBy(HC58Transport::Client::PID)) {
        HC58Transport::release(HC58Transport::Client::PID);
    }
    requestState = RequestState::Idle;
    requestedPid = 0;
    requestDeadlineMs = 0;
}

bool decodeTPS(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0 || buf[4] != PID_TPS) return false;
    int value = (buf[5] * 100) / 255;
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    snprintf(out, outSize, "%d", value);
    return true;
}

bool decodeIAT(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0 || buf[4] != PID_IAT) return false;
    const int tempC = static_cast<int>(buf[5]) - 40;
    const int tempF = (tempC * 9 / 5) + 32;
    snprintf(out, outSize, "%d", tempF);
    return true;
}

bool decodeMAP(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0 || buf[4] != PID_MAP) return false;
    snprintf(out, outSize, "%d", static_cast<int>(buf[5]));
    return true;
}

bool decodeMAF(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    if (!buf || len < 7 || !out || outSize == 0 || buf[4] != PID_MAF) return false;
    const int raw = (static_cast<int>(buf[5]) << 8) | buf[6];
    int value = (raw + 50) / 100;
    if (value < 0) value = 0;
    if (value > 999) value = 999;
    snprintf(out, outSize, "%d", value);
    return true;
}

bool decodeCalcLoad(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0 || buf[4] != PID_CALC_LOAD) return false;
    int value = (buf[5] * 100) / 255;
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    snprintf(out, outSize, "%d", value);
    return true;
}

bool decodeTiming(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0 || buf[4] != PID_TIMING) return false;
    const int value = (static_cast<int>(buf[5]) / 2) - 64;
    snprintf(out, outSize, "%d", value);
    return true;
}

bool decodeO2mV(uint8_t expectedPid, const uint8_t *buf, uint8_t len,
                char *out, size_t outSize) {
    if (!buf || len < 7 || !out || outSize == 0 || buf[4] != expectedPid) return false;
    int value = static_cast<int>(buf[5]) * 5;
    if (value < 0) value = 0;
    if (value > 1275) value = 1275;
    snprintf(out, outSize, "%d", value);
    return true;
}

bool decodeFuelTrim(uint8_t expectedPid, const uint8_t *buf, uint8_t len,
                    char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0 || buf[4] != expectedPid) return false;
    int value = (static_cast<int>(buf[5]) - 128) * 100 / 128;
    if (value < -100) value = -100;
    if (value > 100) value = 100;
    snprintf(out, outSize, "%d", value);
    return true;
}

bool decodeResponse(const uint8_t *buf, uint8_t len, char *out, size_t outSize) {
    switch (requestedPid) {
        case PID_TPS:       return decodeTPS(buf, len, out, outSize);
        case PID_IAT:       return decodeIAT(buf, len, out, outSize);
        case PID_MAP:       return decodeMAP(buf, len, out, outSize);
        case PID_MAF:       return decodeMAF(buf, len, out, outSize);
        case PID_CALC_LOAD: return decodeCalcLoad(buf, len, out, outSize);
        case PID_TIMING:    return decodeTiming(buf, len, out, outSize);
        case PID_O2_B1:     return decodeO2mV(PID_O2_B1, buf, len, out, outSize);
        case PID_O2_B2:     return decodeO2mV(PID_O2_B2, buf, len, out, outSize);
        case PID_STFT1:
        case PID_LTFT1:
        case PID_STFT2:
        case PID_LTFT2:
            return decodeFuelTrim(requestedPid, buf, len, out, outSize);
        default:
            return false;
    }
}

bool exactRequestEcho(const HC58Transport::Frame &frame) {
    return frame.length == sizeof(requestBytes) &&
           memcmp(frame.bytes, requestBytes, sizeof(requestBytes)) == 0;
}

const uint8_t *findPositiveReply(const HC58Transport::Frame &frame, uint8_t &remaining) {
    const uint8_t need = (requestedPid == PID_MAF ||
                          requestedPid == PID_O2_B1 ||
                          requestedPid == PID_O2_B2) ? 7 : 6;

    if (frame.length < need) return nullptr;
    for (uint8_t i = 0; i + need <= frame.length; ++i) {
        if (frame.bytes[i]     == 0x48 &&
            frame.bytes[i + 1] == 0x6B &&
            frame.bytes[i + 2] == 0x10 &&
            frame.bytes[i + 3] == 0x41 &&
            frame.bytes[i + 4] == requestedPid) {
            remaining = frame.length - i;
            return &frame.bytes[i];
        }
    }
    return nullptr;
}

bool isNegativeMode01Reply(const HC58Transport::Frame &frame) {
    for (uint8_t i = 0; i + 5 < frame.length; ++i) {
        if (frame.bytes[i]     == 0x48 &&
            frame.bytes[i + 1] == 0x6B &&
            frame.bytes[i + 2] == 0x10 &&
            frame.bytes[i + 3] == 0x7F &&
            frame.bytes[i + 4] == 0x01) {
            return true;
        }
    }
    return false;
}

void selectNextRequest(uint8_t &pid, PIDSlot &slot) {
    if (activePage == PAGE_PID1) {
        switch (pollIndex) {
            case 0:
            case 1:
            case 3:
            case 5:
                pid = PID_TPS; slot = PID_SLOT_Q1; break;
            case 2:
                pid = PID_MAP; slot = PID_SLOT_Q3; break;
            case 4:
                pid = PID_MAF; slot = PID_SLOT_Q4; break;
            default:
                pid = PID_IAT; slot = PID_SLOT_Q2; break;
        }
        pollIndex = (pollIndex + 1) % 7;
        return;
    }

    if (activePage == PAGE_PID2) {
        switch (pollIndex) {
            case 0:
            case 2:
            case 4:
            case 6:
                pid = PID_TIMING; slot = PID_SLOT_Q2; break;
            case 1:
            case 5:
                pid = PID_CALC_LOAD; slot = PID_SLOT_Q1; break;
            case 3:
                pid = PID_O2_B1; slot = PID_SLOT_Q3; break;
            default:
                pid = PID_O2_B2; slot = PID_SLOT_Q4; break;
        }
        pollIndex = (pollIndex + 1) % 8;
        return;
    }

    switch (pollIndex) {
        case 0: pid = PID_STFT1; slot = PID_SLOT_Q1; break;
        case 1: pid = PID_LTFT1; slot = PID_SLOT_Q2; break;
        case 2: pid = PID_STFT2; slot = PID_SLOT_Q3; break;
        default: pid = PID_LTFT2; slot = PID_SLOT_Q4; break;
    }
    pollIndex = (pollIndex + 1) & 0x03;
}

void startNextRequest() {
    uint8_t pid = 0;
    PIDSlot slot = PID_SLOT_Q1;
    selectNextRequest(pid, slot);

    if (!HC58Transport::claim(HC58Transport::Client::PID)) {
        nextPollMs = millis() + POLL_GAP_MS;
        return;
    }

    requestedPid = pid;
    requestedSlot = slot;
    requestBytes[0] = 0x68;
    requestBytes[1] = 0x6A;
    requestBytes[2] = 0xF1;
    requestBytes[3] = 0x01;
    requestBytes[4] = pid;

    if (!HC58Transport::sendFrame(HC58Transport::Client::PID,
                                  requestBytes, sizeof(requestBytes))) {
        failRequest();
        return;
    }

    requestState = RequestState::WaitTxEcho;
    requestDeadlineMs = millis() + TX_TIMEOUT_MS;
}

}  // namespace

void PIDManager_begin() {
    activePage = PAGE_BOOTLOGO;
    clearAllSlots();
    requestState = RequestState::Idle;
    nextPollMs = millis();
    pollIndex = 0;
}

void PIDManager_setPage(PageID page) {
    cancelRequest();
    activePage = page;
    clearAllSlots();
    nextPollMs = millis();
    pollIndex = 0;
}

void PIDManager_update() {
    if (!isPidPage(activePage) || !HC58Transport::isReady()) {
        cancelRequest();
        return;
    }

    if (requestState != RequestState::Idle) {
        if (deadlineReached(requestDeadlineMs)) {
            failRequest();
        }
        return;
    }

    if (static_cast<int32_t>(millis() - nextPollMs) >= 0) {
        startNextRequest();
    }
}

void PIDManager_onHc58Frame(const HC58Transport::Frame &frame) {
    if (requestState == RequestState::Idle ||
        !HC58Transport::isOwnedBy(HC58Transport::Client::PID)) {
        return;
    }

    if (frame.completion.rxFifoOverrun) {
        failRequest();
        return;
    }

    if (requestState == RequestState::WaitTxEcho) {
        if (!exactRequestEcho(frame)) {
            return;
        }

        if (frame.truncated || frame.completion.error ||
            frame.completion.txStatus == HC58Transport::TxStatus::Underrun) {
            failRequest();
            return;
        }

        if (frame.completion.txStatus == HC58Transport::TxStatus::Successful) {
            requestState = RequestState::WaitResponse;
            requestDeadlineMs = millis() + RESPONSE_TIMEOUT_MS;
        }
        // Lost arbitration is intentionally ignored; the HC58 retries.
        return;
    }

    if (frame.truncated || frame.completion.error) {
        return;
    }

    uint8_t available = 0;
    const uint8_t *reply = findPositiveReply(frame, available);
    if (reply) {
        char value[8];
        if (decodeResponse(reply, available, value, sizeof(value))) {
            setSlotValue(requestedSlot, value);
        } else {
            clearSlot(requestedSlot);
        }
        releaseRequest();
        return;
    }

    if (isNegativeMode01Reply(frame)) {
        failRequest();
    }
}

bool PIDManager_isValid(PIDSlot slot) {
    if (slot < 0 || slot >= PID_SLOT_COUNT) return false;
    return slotValid[slot];
}

const char* PIDManager_getValue(PIDSlot slot) {
    if (slot < 0 || slot >= PID_SLOT_COUNT) return "";
    return slotValues[slot];
}

const char* PIDManager_getDisplayText(PIDSlot slot) {
    if (slot < 0 || slot >= PID_SLOT_COUNT) return "---";
    return slotValid[slot] ? slotValues[slot] : "---";
}
