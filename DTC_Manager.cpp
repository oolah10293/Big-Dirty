#include "DTC_Manager.h"

#include <Arduino.h>
#include <string.h>

namespace {

constexpr uint32_t PRETX_HUSH_READ_MS    = 250;
constexpr uint32_t OBSERVE_READ_MS       = 800;
constexpr uint32_t CLEAR_LISTEN_TOTAL_MS = 5000;
constexpr uint32_t POST_CLEAR_DELAY_MS   = 250;
constexpr uint32_t TX_TIMEOUT_MS         = 250;

constexpr int TRAFFIC_ERROR_MAX = 20;
constexpr int TRAFFIC_NONE_MIN  = 50;
constexpr int MAX_CODES = 8;

struct DTCLabel {
    const char* code;
    const char* label;
};

const DTCLabel kKnownDTCs[] = {
    {"P0101", "MAF RANGE"},
    {"P0102", "MAF LOW"},
    {"P0103", "MAF HIGH"},
    {"P0106", "MAP RANGE"},
    {"P0107", "MAP LOW"},
    {"P0108", "MAP HIGH"},
    {"P0112", "IAT LOW"},
    {"P0113", "IAT HIGH"},
    {"P0117", "ECT LOW"},
    {"P0118", "ECT HIGH"},
    {"P0120", "TPS A"},
    {"P0121", "TPS RANGE"},
    {"P0122", "TPS LOW"},
    {"P0123", "TPS HIGH"},
    {"P0128", "COOLANT TEMP"},

    {"P0131", "O2 B1S1 LOW"},
    {"P0132", "O2 B1S1 HIGH"},
    {"P0133", "O2 B1S1 SLOW"},
    {"P0151", "O2 B2S1 LOW"},
    {"P0152", "O2 B2S1 HIGH"},
    {"P0153", "O2 B2S1 SLOW"},
    {"P0171", "SYSTEM LEAN B1"},
    {"P0172", "SYSTEM RICH B1"},
    {"P0174", "SYSTEM LEAN B2"},
    {"P0175", "SYSTEM RICH B2"},

    {"P0300", "RANDOM MISFIRE"},
    {"P0301", "MISFIRE CYL 1"},
    {"P0302", "MISFIRE CYL 2"},
    {"P0303", "MISFIRE CYL 3"},
    {"P0304", "MISFIRE CYL 4"},
    {"P0305", "MISFIRE CYL 5"},
    {"P0306", "MISFIRE CYL 6"},
    {"P0307", "MISFIRE CYL 7"},
    {"P0308", "MISFIRE CYL 8"},

    {"P0700", "TRANS CTRL SYS"},
    {"P0716", "INPUT SPEED RANGE"},
    {"P0717", "INPUT SPEED NO SIG"},
    {"P0722", "OUTPUT SPEED NO SIG"},
    {"P0723", "OUTPUT SPEED INT"},
    {"P0740", "TCC SYS"},
    {"P0751", "SHIFT SOL A PERF"},
    {"P0753", "SHIFT SOL A ELEC"},
    {"P0756", "SHIFT SOL B PERF"},
    {"P0758", "SHIFT SOL B ELEC"},
    {"P0785", "SHIFT/TIME SOL"},
};

constexpr int kKnownDTCCount = sizeof(kKnownDTCs) / sizeof(kKnownDTCs[0]);

enum class Operation : uint8_t {
    Idle = 0,
    StartRead,
    Hush03,
    WaitTx03,
    Observe03,
    Hush07,
    WaitTx07,
    Observe07,
    Classify,
    StartClear,
    WaitTx04,
    ClearListen,
    PostClearDelay
};

struct ServiceResult {
    int trafficItems;
    bool positive;
    bool negative;
    bool decodeError;
};

DTCState dtcState = DTC_STATE_UNREAD;
Operation operation = Operation::Idle;
bool pageActive = false;
bool firstUpdateAfterEnter = false;
bool postClearRead = false;
uint8_t readPass = 0;
uint32_t deadlineMs = 0;
uint32_t trafficStart = 0;
uint8_t requestBytes[5] = {0};
uint8_t activeService = 0;

ServiceResult result03{};
ServiceResult result07{};

char rawDtcList[MAX_CODES][8];
char dtcList[MAX_CODES][48];
int dtcCount = 0;

bool deadlineReached(uint32_t deadline) {
    return static_cast<int32_t>(millis() - deadline) >= 0;
}

void resetServiceResult(ServiceResult &result) {
    result.trafficItems = 0;
    result.positive = false;
    result.negative = false;
    result.decodeError = false;
}

void clearLocalCodes() {
    for (int i = 0; i < MAX_CODES; ++i) {
        rawDtcList[i][0] = '\0';
        dtcList[i][0] = '\0';
    }
    dtcCount = 0;
}

const char *findDTCLabel(const char* code) {
    if (!code || !code[0]) return nullptr;
    for (int i = 0; i < kKnownDTCCount; ++i) {
        if (strcmp(code, kKnownDTCs[i].code) == 0) {
            return kKnownDTCs[i].label;
        }
    }
    return nullptr;
}

void buildDisplayDTC(const char *rawCode, char *out, size_t outSize) {
    if (!rawCode || !out || outSize == 0) return;
    const char* label = findDTCLabel(rawCode);
    if (label) snprintf(out, outSize, "%s - %s", rawCode, label);
    else snprintf(out, outSize, "%s", rawCode);
}

bool hasRawCode(const char* rawCode) {
    if (!rawCode || !rawCode[0]) return false;
    for (int i = 0; i < dtcCount; ++i) {
        if (strcmp(rawDtcList[i], rawCode) == 0) return true;
    }
    return false;
}

bool addDTCUnique(const char *rawCode) {
    if (!rawCode || !rawCode[0]) return false;
    if (hasRawCode(rawCode)) return true;
    if (dtcCount >= MAX_CODET) return true;

    snprintf(rawDtcList[dtcCount], sizeof(rawDtcList[dtcCount]), "%s", rawCode);
    buildDisplayDTC(rawCode, dtcList[dtcCount], sizeof(dtcList[dtcCount]));
    dtcCount++;
    return true;
}

bool formatRawDTC(uint8_t hi, uint8_t lo, char *out, size_t outSize) {
    if (!out || outSize == 0) return false;
    static const char sysMap[4] = {'P','C','B','U'};
    const char system = sysMap[(hi >> 6) & 0x03];
    const int d1 = (hi >> 4) & 0x03;
    const int d2 = hi & 0x0F;
    const int d3 = (lo >> 4) & 0x0F;
    const int d4 = lo & 0x0F;
    snprintf(out, outSize, "%c%d%X%X%X", system, d1, d2, d3, d4);
    return true;
}

bool decodeDTCsFromFrame(const HC58Transport::Frame &frame, uint8_t startIdx) {
    const uint8_t firstPair = startIdx + 4;
    if (firstPair >= frame.length) return false;

    for (uint8_t i = firstPair; i + 1 < frame.length; i += 2) {
        const uint8_t hi = frame.bytes[i];
        const uint8_t lo = frame.bytes[i + 1];
        if (hi == 0x00 && lo == 0x00) break;

        char rawCode[8];
        if (!formatRawDTC(hi, lo, rawCode, sizeof(rawCode)) ||
            !addDTCUnique(rawCode)) {
            return false;
        }
    }
    return true;
}

void releaseTransport() {
    HC58Transport::release(HC58Transport::Client::DTC);
}

void finishOperation(DTCState finalState) {
    operation = Operation::Idle;
    dtcState = finalState;
    activeService = 0;
    releaseTransport();
}

void failOperation(DTCState failureState = DTC_STATE_ERROR) {
    clearLocalCodes();
    finishOperation(failureState);
}

bool claimTransport() {
    return HC58Transport::claim(HC58Transport::Client::DTC);
}

void resetReadResults() {
    resetServiceResult(result03);
    resetServiceResult(result07);
}

void beginReadSequence(bool afterClear) {
    if (!claimTransport()) return;

    clearLocalCodes();
    resetReadResults();
    readPass = 0;
    postClearRead = afterClear;
    dtcState = DTC_STATE_READING;
    operation = Operation::Hush03;
    deadlineMs = millis() + PRETX_HUSH_READ_MS;
}

void beginRetrySequence() {
    clearLocalCodes();
    resetReadResults();
    readPass = 1;
    operation = Operation::Hush03;
    deadlineMs = millis() + PRETX_HUSH_READ_MS;
}

void buildServiceRequest(uint8_t service) {
    requestBytes[0] = 0x68;
    requestBytes[1] = 0x6A;
    requestBytes[2] = 0xF1;
    requestBytes[3] = service;
    requestBytes[4] = 0x00;
}

void sendServiceRequest(uint8_t service, Operation waitState) {
    activeService = service;
    buildServiceRequest(service);
    trafficStart = HC58Transport::snapshotStats().nonEmptyRxItems;

    if (!HC58Transport::sendFrame(HC58Transport::Client::DTC,
                                  requestBytes, sizeof(requestBytes))) {
        failOperation();
        return;
    }

    operation = waitState;
    deadlineMs = millis() + TX_TIMEOUT_MS;
}

bool exactRequestEcho(const HC58Transport::Frame &frame) {
    return frame.length == sizeof(requestBytes) &&
           memcmp(frame.bytes, requestBytes, sizeof(requestBytes)) == 0;
}

int findPositiveReply(const HC58Transport::Frame &frame, uint8_t service) {
    const uint8_t positiveSid = service + 0x40;
    for (uint8_t i = 0; i + 3 < frame.length; ++i) {
        if (frame.bytes[i]     == 0x48 &&
            frame.bytes[i + 1] == 0x6B &&
            frame.bytes[i + 2] == 0x10 &&
            frame.bytes[i + 3] == positiveSid) {
            return i;
        }
    }
    return -1;
}

bool isNegativeReply(const HC58Transport::Frame &frame, uint8_t service) {
    for (uint8_t i = 0; i + 5 < frame.length; ++i) {
        if (frame.bytes[i]     == 0x48 &&
            frame.bytes[i + 1] == 0x6B &&
            frame.bytes[i + 2] == 0x10 &&
            frame.bytes[i + 3] == 0x7F &&
            frame.bytes[i + 4] == service) {
            return true;
        }
    }
    return false;
}

ServiceResult &resultForService(uint8_t service) {
    return service == 0x03 ? result03 : result07;
}

void closeObservation(ServiceResult &result) {
    const uint32_t nowItems = HC58Transport::snapshotStats().nonEmptyRxItems;
    const uint32_t delta = nowItems - trafficStart;
    result.trafficItems = delta > 0x7FFFFFFFUL ? 0x7FFFFFFF : static_cast<int>(delta);
}

void classifyRead() {
    if (result03.decodeError || result07.decodeError) {
        failOperation();
        return;
    }

    if (dtcCount > 0) {
        finishOperation(DTC_STATE_PRESENT);
        return;
    }

    if (result03.negative || r07.negative) {
        finishOperation(DTC_STATE_REQUEST_ERROR);
        return;
    }

    if (result03.trafficItems <= 0 && result07.trafficItems <= 0) {
        failOperation();
        return;
    }

    const int bestTraffic = result03.trafficItems > result07.trafficItems
        ? result03.trafficItems : result07.trafficItems;

    if (postClearRead) {
        finishOperation(bestTraffic >= (TRAFFIC_ERROR_MAX + 1)
            ? DTC_STATE_NONE : DTC_STATE_ERROR);
        return;
    }

    if (bestTraffic >= TRAFFIC_NONE_MIN) {
        finishOperation(DTC_STATE_NONE);
        return;
    }

    if (readPass == 0 &&
        !(result03.trafficItems <= TRAFFIC_ERROR_MAX &&
          result07.trafficItems <= TRAFFIC_ERROR_MAX)) {
        beginRetrySequence();
        return;
    }

    finishOperation(DTC_STATE_ERROR);
}

}  // namespace

void DTCManager_begin() {
    dtcState = DTC_STATE_UNREAD;
    operation = Operation::Idle;
    pageActive = false;
    firstUpdateAfterEnter = false;
    postClearRead = false;
    clearLocalCodes();
    resetReadResults();
}

void DTCManager_enterPage() {
    DTCManager_leavePage();
    pageActive = true;
    firstUpdateAfterEnter = true;
    postClearRead = false;
    clearLocalCodes();
    resetReadResults();
    dtcState = DTC_STATE_READING;
    operation = Operation::StartRead;
}

void DTCManager_leavePage() {
    pageActive = false;
    firstUpdateAfterEnter = false;
    operation = Operation::Idle;
    activeService = 0;
    releaseTransport();
}

void DTCManager_update() {
    if (!pageActive) return;

    if (firstUpdateAfterEnter) {
        firstUpdateAfterEnter = false;
        return;
    }

    switch (operation) {
        case Operation::Idle:
            return;

        case Operation::StartRead:
            beginReadSequence(false);
            return;

        case Operation::Hush03:
            if (deadlineReached(deadlineMs)) {
                sendServiceRequest(0x03, Operation::WaitTx03);
            }
            return;

        case Operation::WaitTx03:
        case Operation::WaitTx07:
        case Operation::WaitTx04:
            if (deadlineReached(deadlineMs)) {

                failOperation();
            }
            return;

        case Operation::Observe03:
            if (deadlineReached(deadlineMs)) {
                closeObservation(result03);
                operation = Operation::Hush07;
                deadlineMs = millis() + PRETX_HUSH_READ_MS;
            }
            return;

        case Operation::Hush07:
            if (deadlineReached(deadlineMs)) {
                sendServiceRequest(0x07, Operation::WaitTx07);
            }
            return;

        case Operation::Observe07:
            if (deadlineReached(deadlineMs)) {
                closeObservation(result07);
                operation = Operation::Classify;
            }
            return;

        case Operation::Classify:
            classifyRead();
            return;

        case Operation::StartClear:
            if (!claimTransport()) return;
            sendServiceRequest(0x04, Operation::WaitTx04);
            return;

        case Operation::ClearListen:
            if (deadlineReached(deadlineMs)) {
                operation = Operation::PostClearDelay;
                deadlineMs = millis() + POST_CLEAR_DELAY_MS;
            }
            return;

        case Operation::PostClearDelay:
            if (deadlineReached(deadlineMs)) {
                beginReadSequence(true);
            }
            return;
    }
}

void DTCManager_requestClear() {
    if (!pageActive || dtcState != DTC_STATE_PRESENT) return;

    clearLocalCodes();
    resetReadResults();
    postClearRead = false;
    dtcState = DTC_STATE_CLEARING;
    operation = Operation::StartClear;
}

void DTCManager_onHc58Frame(const HC58Transport::Frame &frame) {
    if (!pageActive || operation == Operation::Idle ||
        !HC58Transport::isOwnedBy(HC58Transport::Client::DTC)) {
        return;
    }

    if (frame.completion.rxFifoOverrun) {
        failOperation();
        return;
    }

    const bool waitingForTx = operation == Operation::WaitTx03 ||
                              operation == Operation::WaitTx07 ||
                              operation == Operation::WaitTx04;

    if (waitingForTx && exactRequestEcho(frame)) {
        if (frame.truncated || frame.completion.error ||
            frame.completion.txStatus == HC58Transport::TxStatus::Underrun) {
            failOperation();
            return;
        }

        if (frame.completion.txStatus == HC58Transport::TxStatus::Successful) {
            if (operation == Operation::WaitTx03) {
                operation = Operation::Observe03;
                deadlineMs = millis() + OBSERVE_READ_MS;
            } else if (operation == Operation::WaitTx07) {
                operation = Operation::Observe07;
                deadlineMs = millis() + OBSERVE_READ_MS;
            } else {
                operation = Operation::ClearListen;
                deadlineMs = millis() + CLEAR_LISTEN_TOTAL_MS;
            }
        }
        // Lost arbitration is left to the HC58's automatic retry.
        return;
    }

    const bool observing = operation == Operation::Observe03 ||
                           operation == Operation::Observe07;
    if (!observing || frame.truncated || frame.completion.error) return;

    const uint8_t service = operation == Operation::Observe03 ? 0x03 : 0x07;
    ServiceResult &result = resultForService(service);

    const int positiveIndex = findPositiveReply(frame, service);
    if (positiveIndex >= 0) {
        result.positive = true;
        if (!decodeDTCsFromFrame(frame, static_cast<uint8_t>(positiveIndex))) {
            result.decodeError = true;
        }
    }

    if (isNegativeReply(frame, service)) {
        result.negative = true;
    }
}

DTCState DTCManager_getState() {
    return dtcState;
}

int DTCManager_getCount() {
    return dtcCount;
}

const char* DTCManager_getCode(int index) {
    if (index < 0 || index >= dtcCount) return nullptr;
    return dtcList[index];
}

bool DTCManager_showClearButton() {
    return dtcState == DTC_STATE_PRESENT;
}
