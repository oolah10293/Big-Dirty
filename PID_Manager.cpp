#include "PID_Manager.h"

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
// Timing — aligned with your working sketch
// ============================================================
static const uint32_t POSTTX_MS      = 9;
static const uint16_t EOD_US         = 750;
static const uint16_t QUIET_GAP_MS   = 15;
static const uint16_t MAX_CAPTURE_MS = 80;
static const uint16_t POLL_GAP_MS    = 10;

// ============================================================
// Mode 01 PIDs
// ============================================================
static const uint8_t PID_CALC_LOAD = 0x04;
static const uint8_t PID_MAP       = 0x0B;
static const uint8_t PID_IAT       = 0x0F;
static const uint8_t PID_MAF       = 0x10;
static const uint8_t PID_TPS       = 0x11;
static const uint8_t PID_TIMING    = 0x0E;
static const uint8_t PID_O2_B1     = 0x14;
static const uint8_t PID_O2_B2     = 0x15;
static const uint8_t PID_STFT1     = 0x06;
static const uint8_t PID_LTFT1     = 0x07;
static const uint8_t PID_STFT2     = 0x08;
static const uint8_t PID_LTFT2     = 0x09;

// ============================================================
// Manager state
// ============================================================
static PageID activePage = PAGE_BOOTLOGO;

static char slotValues[PID_SLOT_COUNT][8];
static bool slotValid[PID_SLOT_COUNT];

static bool hc58Ready = false;
static uint32_t nextPollMs = 0;

// Shared page-local round-robin index
// PAGE_PID1: 0=TPS, 1=IAT, 2=MAP, 3=MAF
// PAGE_PID2: 0=Load, 1=Timing, 2=O2 B1, 3=O2 B2
static uint8_t pollIndex = 0;

// RX buffers
static uint8_t frame[128];
static int frameLen = 0;

static uint8_t cleanResp[16];
static int cleanRespLen = 0;

// ============================================================
// Helpers
// ============================================================
static void copyValue(char *dst, const char *src, size_t dstSize) {
    if (!dst || !src || dstSize == 0) return;
    snprintf(dst, dstSize, "%s", src);
}

static void clearAllSlots() {
    for (int i = 0; i < PID_SLOT_COUNT; i++) {
        slotValues[i][0] = '\0';
        slotValid[i] = false;
    }
}

static void clearSlot(PIDSlot slot) {
    if (slot < 0 || slot >= PID_SLOT_COUNT) return;
    slotValues[slot][0] = '\0';
    slotValid[slot] = false;
}

static void setSlotValue(PIDSlot slot, const char *value) {
    if (slot < 0 || slot >= PID_SLOT_COUNT) return;
    copyValue(slotValues[slot], value, sizeof(slotValues[slot]));
    slotValid[slot] = true;
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
    delay(500);
    digitalWrite(PIN_RST, HIGH);
    delay(5);

    SPIcycle(0x12, 0x1A, s, d);   // CCR-first
    SPIcycle(0x10, 0xC0, s, d);   // RENA + TENA
}

static void sendPIDRequest(uint8_t pid) {
    uint8_t s, d;

    SPIcycle(0x68, 0x14, s, d);
    SPIcycle(0x6A, 0x04, s, d);
    SPIcycle(0xF1, 0x04, s, d);
    SPIcycle(0x01, 0x04, s, d);
    SPIcycle(pid,  0x0C, s, d);
}

static bool drainFIFOUntilQuiet(uint16_t quietGapMs, uint16_t maxCaptureMs, int &lenOut) {
    uint8_t s, d;
    frameLen = 0;

    unsigned long startMs    = millis();
    unsigned long lastByteMs = millis();

    while ((millis() - startMs) < maxCaptureMs) {
        SPIcycle(0x00, 0x02, s, d);

        if (s != 0x10 && s != 0x00) {
            if (frameLen < (int)sizeof(frame)) {
                frame[frameLen++] = d;
            }
            lastByteMs = millis();
        }

        if (frameLen > 0 && (millis() - lastByteMs) >= quietGapMs) {
            break;
        }
    }

    lenOut = frameLen;
    return (frameLen > 0);
}

static bool extractPCMResponse(uint8_t requestedPid, uint8_t *buf, int len, uint8_t *out, int &outLen) {
    outLen = 0;

    int needLen = 6;  // default for 1-data-byte Mode 01 PIDs

    if (requestedPid == PID_MAF || requestedPid == PID_O2_B1 || requestedPid == PID_O2_B2) {
        needLen = 7;  // 2 data bytes
    }

    for (int i = 0; i <= len - needLen; i++) {
        if (buf[i]     == 0x48 &&
            buf[i + 1] == 0x6B &&
            buf[i + 2] == 0x10 &&
            buf[i + 3] == 0x41 &&
            buf[i + 4] == requestedPid) {

            for (int j = 0; j < needLen; j++) {
                out[j] = buf[i + j];
            }
            outLen = needLen;
            return true;
        }
    }

    return false;
}

// ============================================================
// Decode helpers
// ============================================================
static bool decodeTPS(uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0) return false;
    if (buf[4] != PID_TPS) return false;

    int tps = (buf[5] * 100) / 255;
    if (tps < 0) tps = 0;
    if (tps > 100) tps = 100;

    snprintf(out, outSize, "%d", tps);
    return true;
}

static bool decodeIAT(uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0) return false;
    if (buf[4] != PID_IAT) return false;

    int tempC = (int)buf[5] - 40;
    int tempF = (tempC * 9 / 5) + 32;

    snprintf(out, outSize, "%d", tempF);
    return true;
}

static bool decodeMAP(uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0) return false;
    if (buf[4] != PID_MAP) return false;

    int kpa = (int)buf[5];
    if (kpa < 0) kpa = 0;
    if (kpa > 255) kpa = 255;

    snprintf(out, outSize, "%d", kpa);
    return true;
}

static bool decodeMAF(uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 7 || !out || outSize == 0) return false;
    if (buf[4] != PID_MAF) return false;

    int raw = ((int)buf[5] << 8) | buf[6];
    int maf = (raw + 50) / 100;   // rounded integer g/s

    if (maf < 0) maf = 0;
    if (maf > 999) maf = 999;

    snprintf(out, outSize, "%d", maf);
    return true;
}

static bool decodeCalcLoad(uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0) return false;
    if (buf[4] != PID_CALC_LOAD) return false;

    int load = (buf[5] * 100) / 255;
    if (load < 0) load = 0;
    if (load > 100) load = 100;

    snprintf(out, outSize, "%d", load);
    return true;
}

static bool decodeTiming(uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0) return false;
    if (buf[4] != PID_TIMING) return false;

    int timing = ((int)buf[5] / 2) - 64;

    snprintf(out, outSize, "%d", timing);
    return true;
}

static bool decodeO2mV(uint8_t expectedPid, uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 7 || !out || outSize == 0) return false;
    if (buf[4] != expectedPid) return false;

    // Mode 01 PIDs 0x14/0x15:
    // A = voltage in 0.005 V steps
    int mv = (int)buf[5] * 5;

    if (mv < 0) mv = 0;
    if (mv > 1275) mv = 1275;

    snprintf(out, outSize, "%d", mv);
    return true;
}

static bool decodeFuelTrim(uint8_t expectedPid, uint8_t *buf, int len, char *out, size_t outSize) {
    if (!buf || len < 6 || !out || outSize == 0) return false;
    if (buf[4] != expectedPid) return false;

    int val = ((int)buf[5] - 128) * 100 / 128;

    // clamp for sanity
    if (val < -100) val = -100;
    if (val > 100)  val = 100;

    snprintf(out, outSize, "%d", val);
    return true;
}

// ============================================================
// HC58 bring-up
// ============================================================
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

// ============================================================
// Shared poll helper
// ============================================================
static void pollOnePidToSlot(uint8_t pid, PIDSlot slot) {
    sendPIDRequest(pid);

    delay(POSTTX_MS);
    delayMicroseconds(EOD_US);

    int len = 0;
    bool gotFrame = drainFIFOUntilQuiet(QUIET_GAP_MS, MAX_CAPTURE_MS, len);

    if (!(gotFrame && extractPCMResponse(pid, frame, len, cleanResp, cleanRespLen))) {
        clearSlot(slot);
        return;
    }

    char buf[8];
    bool ok = false;

    switch (pid) {
        case PID_TPS:
            ok = decodeTPS(cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_IAT:
            ok = decodeIAT(cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_MAP:
            ok = decodeMAP(cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_MAF:
            ok = decodeMAF(cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_CALC_LOAD:
            ok = decodeCalcLoad(cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_TIMING:
            ok = decodeTiming(cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_O2_B1:
            ok = decodeO2mV(PID_O2_B1, cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_O2_B2:
            ok = decodeO2mV(PID_O2_B2, cleanResp, cleanRespLen, buf, sizeof(buf));
            break;

        case PID_STFT1:
        case PID_LTFT1:
        case PID_STFT2:
        case PID_LTFT2:
            ok = decodeFuelTrim(pid, cleanResp, cleanRespLen, buf, sizeof(buf));
            break;
        default:
            ok = false;
            break;
    }

    if (ok) {
        setSlotValue(slot, buf);
    } else {
        clearSlot(slot);
    }
}

// ============================================================
// Per-page poll cycles
// ============================================================
static void pollPID1Cycle() {
    switch (pollIndex) {
        case 0:
            pollOnePidToSlot(PID_TPS, PID_SLOT_Q1);
            break;

        case 1:
            pollOnePidToSlot(PID_TPS, PID_SLOT_Q1);
            break;

        case 2:
            pollOnePidToSlot(PID_MAP, PID_SLOT_Q3);
            break;

        case 3:
            pollOnePidToSlot(PID_TPS, PID_SLOT_Q1);
            break;

        case 4:
            pollOnePidToSlot(PID_MAF, PID_SLOT_Q4);
            break;

        case 5:
            pollOnePidToSlot(PID_TPS, PID_SLOT_Q1);
            break;

        default:
            pollOnePidToSlot(PID_IAT, PID_SLOT_Q2);
            break;
    }

    pollIndex++;
    if (pollIndex >= 7) {
        pollIndex = 0;
    }
}

static void pollPID2Cycle() {
    switch (pollIndex) {
        case 0:
            pollOnePidToSlot(PID_TIMING, PID_SLOT_Q2);
            break;

        case 1:
            pollOnePidToSlot(PID_CALC_LOAD, PID_SLOT_Q1);
            break;

        case 2:
            pollOnePidToSlot(PID_TIMING, PID_SLOT_Q2);
            break;

        case 3:
            pollOnePidToSlot(PID_O2_B1, PID_SLOT_Q3);
            break;

        case 4:
            pollOnePidToSlot(PID_TIMING, PID_SLOT_Q2);
            break;

        case 5:
            pollOnePidToSlot(PID_CALC_LOAD, PID_SLOT_Q1);
            break;

        case 6:
            pollOnePidToSlot(PID_TIMING, PID_SLOT_Q2);
            break;

        default:
            pollOnePidToSlot(PID_O2_B2, PID_SLOT_Q4);
            break;
    }

    pollIndex++;
    if (pollIndex >= 8) {
        pollIndex = 0;
    }
}

static void pollPID3Cycle() {
    switch (pollIndex) {
        case 0:
            pollOnePidToSlot(PID_STFT1, PID_SLOT_Q1);
            break;
        case 1:
            pollOnePidToSlot(PID_LTFT1, PID_SLOT_Q2);
            break;
        case 2:
            pollOnePidToSlot(PID_STFT2, PID_SLOT_Q3);
            break;
        default:
            pollOnePidToSlot(PID_LTFT2, PID_SLOT_Q4);
            break;
    }

    pollIndex = (pollIndex + 1) & 0x03;
}

// ============================================================
// Public API
// ============================================================
void PIDManager_begin() {
    activePage = PAGE_BOOTLOGO;
    clearAllSlots();
    nextPollMs = millis();
    pollIndex = 0;
    hc58_beginIfNeeded();
}

void PIDManager_setPage(PageID page) {
    activePage = page;
    clearAllSlots();
    nextPollMs = millis();
    pollIndex = 0;
}

void PIDManager_update() {
    if (activePage != PAGE_PID1 &&
        activePage != PAGE_PID2 &&
        activePage != PAGE_PID3) {
        return;
    }


    if (!hc58Ready) {
        hc58_beginIfNeeded();
        if (!hc58Ready) return;
    }

    uint32_t now = millis();
    if (now < nextPollMs) {
        return;
    }

    if (activePage == PAGE_PID1) {
        pollPID1Cycle();
    } else if (activePage == PAGE_PID2) {
        pollPID2Cycle();
    } else if (activePage == PAGE_PID3) {
        pollPID3Cycle();
    }

    nextPollMs = millis() + POLL_GAP_MS;
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