#include "DTC_Manager.h"

#include <Arduino.h>
#include <SPI.h>
#include <string.h>

// ============================================================
// Teensy 4.1 <-> HC58 wiring
// ============================================================
static const int PIN_INT  = 0;
static const int PIN_MISO = 1;
static const int PIN_RST  = 24;
static const int PIN_CS   = 25;
static const int PIN_MOSI = 26;
static const int PIN_SCK  = 27;

// ============================================================
// HC58 / SPI settings
// ============================================================
static const uint32_t SPI_HZ      = 100000;
static const uint32_t CS_SETUP_US = 40;
static const uint32_t CS_HOLD_US  = 40;

// ============================================================
// Timing
// ============================================================
static const uint32_t POSTTX_US              = 8300;
static const uint32_t EOD_US                 = 750;
static const uint32_t DRAIN_MS_MODE03        = 800;
static const uint32_t DRAIN_MS_CLEAR         = 400;
static const uint32_t PRETX_HUSH_MODE03_MS   = 250;
static const uint32_t POST_CLEAR_DELAY_MS    = 250;
static const uint32_t CLEAR_LISTEN_TOTAL_MS  = 5000;

// ============================================================
// Traffic heuristics for no-code detection
// ============================================================
static const int TRAFFIC_ERROR_MAX = 20;   // 0-20  -> read error
static const int TRAFFIC_NONE_MIN  = 50;   // 50+   -> alive/no codes

// ============================================================
// DTC manager state
// ============================================================
static DTCState dtcState = DTC_STATE_UNREAD;

static const int MAX_CODES = 8;
static char dtcList[MAX_CODES][48];
static int dtcCount = 0;

static bool hc58Ready = false;
static bool readPending = false;
static bool clearPending = false;
static bool firstUpdateAfterEnter = false;
static bool postClearRead = false;

// RX buffer
static uint8_t rx[256];
static int rxLen = 0;

// ============================================================
// Generic code lookup table (Phase 1 curated list)
// ============================================================
struct DTCLabel {
    const char* code;
    const char* label;
};

static const DTCLabel kKnownDTCs[] = {
    // Air / load / throttle / temp
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

    // Upstream O2 / fueling
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

    // Misfire
    {"P0300", "RANDOM MISFIRE"},
    {"P0301", "MISFIRE CYL 1"},
    {"P0302", "MISFIRE CYL 2"},
    {"P0303", "MISFIRE CYL 3"},
    {"P0304", "MISFIRE CYL 4"},
    {"P0305", "MISFIRE CYL 5"},
    {"P0306", "MISFIRE CYL 6"},
    {"P0307", "MISFIRE CYL 7"},
    {"P0308", "MISFIRE CYL 8"},

    // Transmission
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

static const int kKnownDTCCount = sizeof(kKnownDTCs) / sizeof(kKnownDTCs[0]);

// ============================================================
// Helpers
// ============================================================
static void clearLocalCodes() {
    for (int i = 0; i < MAX_CODES; i++) {
        dtcList[i][0] = '\0';
    }
    dtcCount = 0;
}

static const char* findDTCLabel(const char* code) {
    if (!code || !code[0]) return nullptr;

    for (int i = 0; i < kKnownDTCCount; i++) {
        if (strcmp(code, kKnownDTCs[i].code) == 0) {
            return kKnownDTCs[i].label;
        }
    }

    return nullptr;
}

static void buildDisplayDTC(const char* rawCode, char* out, size_t outSize) {
    if (!rawCode || !out || outSize == 0) return;

    const char* label = findDTCLabel(rawCode);
    if (label) {
        snprintf(out, outSize, "%s - %s", rawCode, label);
    } else {
        snprintf(out, outSize, "%s", rawCode);
    }
}

static void SPIcycle(uint8_t cmd, uint8_t data, uint8_t &status, uint8_t &resp) {
    digitalWrite(PIN_CS, LOW);
    delayMicroseconds(CS_SETUP_US);
    status = SPI1.transfer(cmd);
    resp   = SPI1.transfer(data);
    digitalWrite(PIN_CS, HIGH);
    delayMicroseconds(CS_HOLD_US);
}

static void hc58_init() {
    uint8_t s, d;

    digitalWrite(PIN_RST, LOW);
    delay(40);
    digitalWrite(PIN_RST, HIGH);
    delay(120);

    SPIcycle(0x12, 0x1A, s, d);   // CCR-first
    SPIcycle(0x10, 0xC0, s, d);   // RENA + TENA
}

static void hc58_beginIfNeeded() {
    if (hc58Ready) return;

    pinMode(PIN_INT, INPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_CS, OUTPUT);

    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_RST, HIGH);

    SPI1.setMISO(PIN_MISO);
    SPI1.setMOSI(PIN_MOSI);
    SPI1.setSCK(PIN_SCK);
    SPI1.begin();
    SPI1.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE3));

    hc58_init();
    hc58Ready = true;
}

static void sendHeader_Functional() {
    uint8_t s, d;
    SPIcycle(0x68, 0x14, s, d);
    SPIcycle(0x6A, 0x04, s, d);
    SPIcycle(0xF1, 0x04, s, d);
}

static void send03() {
    uint8_t s, d;

    delay(PRETX_HUSH_MODE03_MS);
    sendHeader_Functional();

    SPIcycle(0x03, 0x0C, s, d);
    SPIcycle(0x00, 0x0C, s, d);
}

static void send04() {
    uint8_t s, d;

    sendHeader_Functional();

    SPIcycle(0x04, 0x0C, s, d);
    SPIcycle(0x00, 0x0C, s, d);
}

static int drain(unsigned winMs) {
    uint8_t s, d;
    rxLen = 0;

    unsigned long t0 = millis();

    while ((millis() - t0) < winMs && rxLen < (int)sizeof(rx)) {
        SPIcycle(0x00, 0x02, s, d);
        if (s != 0x00 && s != 0x10) {
            rx[rxLen++] = d;
        }
    }

    return rxLen;
}

static bool findPositiveReply03(int &idx) {
    idx = -1;

    for (int i = 0; i < rxLen - 4; i++) {
        if (rx[i]     == 0x48 &&
            rx[i + 1] == 0x6B &&
            rx[i + 2] == 0x10 &&
            rx[i + 3] == 0x43) {
            idx = i;
            return true;
        }
    }

    return false;
}

static bool findNegativeReply03(int &idx, uint8_t &reason) {
    idx = -1;
    reason = 0x00;

    for (int i = 0; i < rxLen - 5; i++) {
        if (rx[i]     == 0x48 &&
            rx[i + 1] == 0x6B &&
            rx[i + 2] == 0x10 &&
            rx[i + 3] == 0x7F &&
            rx[i + 4] == 0x03) {

            idx = i;
            reason = rx[i + 5];
            return true;
        }
    }

    return false;
}

static bool formatRawDTC(uint8_t hi, uint8_t lo, char *out, size_t outSize) {
    if (!out || outSize == 0) return false;

    static const char sysMap[4] = {'P','C','B','U'};

    char system = sysMap[(hi >> 6) & 0x03];
    int d1 = (hi >> 4) & 0x03;
    int d2 = hi & 0x0F;
    int d3 = (lo >> 4) & 0x0F;
    int d4 = lo & 0x0F;

    snprintf(out, outSize, "%c%d%X%X%X", system, d1, d2, d3, d4);
    return true;
}

static bool decodeStoredDTCsFromReply(int startIdx) {
    clearLocalCodes();

    // Expected PCM format:
    // 48 6B 10 43 A B C D 00 00 ...
    int firstPair = startIdx + 4;

    if (firstPair >= rxLen) {
        return false;
    }

    for (int i = firstPair; i + 1 < rxLen && dtcCount < MAX_CODES; i += 2) {
        uint8_t hi = rx[i];
        uint8_t lo = rx[i + 1];

        if (hi == 0x00 && lo == 0x00) {
            break;
        }

        char rawCode[8];
        if (!formatRawDTC(hi, lo, rawCode, sizeof(rawCode))) {
            return false;
        }

        buildDisplayDTC(rawCode, dtcList[dtcCount], sizeof(dtcList[dtcCount]));
        dtcCount++;
    }

    return true;
}

static int performMode03Read() {
    send03();
    delayMicroseconds(POSTTX_US + EOD_US);
    return drain(DRAIN_MS_MODE03);
}

static bool classifyNoPositiveNoNegativeInitial(int firstLen) {
    if (firstLen >= TRAFFIC_NONE_MIN) {
        dtcState = DTC_STATE_NONE;
        return true;
    }

    if (firstLen <= TRAFFIC_ERROR_MAX) {
        dtcState = DTC_STATE_ERROR;
        return false;
    }

    // 21-49: ambiguous -> rescan once
    int secondLen = performMode03Read();

    int negIdx = -1;
    uint8_t negReason = 0x00;
    if (findNegativeReply03(negIdx, negReason)) {
        dtcState = DTC_STATE_REQUEST_ERROR;
        return false;
    }

    int posIdx = -1;
    if (findPositiveReply03(posIdx)) {
        if (!decodeStoredDTCsFromReply(posIdx)) {
            dtcState = DTC_STATE_ERROR;
            return false;
        }

        dtcState = (dtcCount > 0) ? DTC_STATE_PRESENT : DTC_STATE_NONE;
        return (dtcState != DTC_STATE_ERROR);
    }

    if (secondLen >= TRAFFIC_NONE_MIN) {
        dtcState = DTC_STATE_NONE;
        return true;
    }

    dtcState = DTC_STATE_ERROR;
    return false;
}

static bool classifyNoPositiveNoNegativePostClear(int firstLen) {
    if (firstLen >= TRAFFIC_ERROR_MAX + 1) {
        dtcState = DTC_STATE_NONE;
        return true;
    }

    dtcState = DTC_STATE_ERROR;
    return false;
}

static bool doMode03Read() {
    if (!hc58Ready) {
        hc58_beginIfNeeded();
        if (!hc58Ready) {
            dtcState = DTC_STATE_ERROR;
            return false;
        }
    }

    int firstLen = performMode03Read();

    // Bus totally dead
    if (firstLen <= 0) {
        clearLocalCodes();
        dtcState = DTC_STATE_ERROR;
        return false;
    }

    int negIdx = -1;
    uint8_t negReason = 0x00;
    if (findNegativeReply03(negIdx, negReason)) {
        clearLocalCodes();
        dtcState = DTC_STATE_REQUEST_ERROR;
        return false;
    }

    int posIdx = -1;
    if (findPositiveReply03(posIdx)) {
        if (!decodeStoredDTCsFromReply(posIdx)) {
            clearLocalCodes();
            dtcState = DTC_STATE_ERROR;
            return false;
        }

        dtcState = (dtcCount > 0) ? DTC_STATE_PRESENT : DTC_STATE_NONE;
        return true;
    }

    clearLocalCodes();

    if (postClearRead) {
        return classifyNoPositiveNoNegativePostClear(firstLen);
    }

    return classifyNoPositiveNoNegativeInitial(firstLen);
}

static void doMode04ClearThenReread() {
    if (!hc58Ready) {
        hc58_beginIfNeeded();
        if (!hc58Ready) {
            dtcState = DTC_STATE_ERROR;
            return;
        }
    }

    send04();
    delayMicroseconds(POSTTX_US + EOD_US);

    unsigned long t0 = millis();
    while ((millis() - t0) < CLEAR_LISTEN_TOTAL_MS) {
        drain(DRAIN_MS_CLEAR);
        delay(50);
    }

    delay(POST_CLEAR_DELAY_MS);

    postClearRead = true;
    doMode03Read();
    postClearRead = false;
}

// ============================================================
// Public API
// ============================================================
void DTCManager_begin() {
    dtcState = DTC_STATE_UNREAD;
    clearLocalCodes();
    readPending = false;
    clearPending = false;
    firstUpdateAfterEnter = false;
    postClearRead = false;
    hc58_beginIfNeeded();
}

void DTCManager_enterPage() {
    clearLocalCodes();
    dtcState = DTC_STATE_READING;
    readPending = true;
    clearPending = false;
    firstUpdateAfterEnter = true;
    postClearRead = false;
}

void DTCManager_requestClear() {
    if (dtcState != DTC_STATE_PRESENT) return;

    clearLocalCodes();
    dtcState = DTC_STATE_CLEARING;
    clearPending = true;
    readPending = false;
    postClearRead = false;
}

void DTCManager_update() {
    if (readPending) {
        if (firstUpdateAfterEnter) {
            firstUpdateAfterEnter = false;
            return;   // allow page to show --- DTC CODES first
        }

        readPending = false;
        doMode03Read();
        return;
    }

    if (clearPending) {
        clearPending = false;
        dtcState = DTC_STATE_READING;   // visually use ---
        doMode04ClearThenReread();
        return;
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
    return (dtcState == DTC_STATE_PRESENT);
}