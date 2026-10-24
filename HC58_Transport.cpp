#include "HC58_Transport.h"

#include <SPI.h>
#include <string.h>

namespace HC58Transport {
namespace {

// Teensy 4.1 <-> Fixed Until Broken HC58 board through Big Dirty interface.
constexpr int PIN_INT  = 0;
constexpr int PIN_MISO = 1;
constexpr int PIN_RST  = 24;
constexpr int PIN_CS   = 25;
constexpr int PIN_MOSI = 26;
constexpr int PIN_SCK  = 27;

constexpr uint32_t SPI_HZ      = 100000;
constexpr uint32_t CS_SETUP_US = 40;
constexpr uint32_t CS_HOLD_US  = 40;
constexpr uint8_t SERVICE_ITEM_BUDGET = 64;

// Proven A/B configuration: IMSK cleared, IMOD left at zero.
constexpr uint8_t CONFIG_INTERRUPTS_ENABLED = 0x02;
constexpr uint8_t CONFIG_IMMEDIATE_AND_FLUSH_ONE = 0x1A;
constexpr uint8_t LEGACY_POST_CONFIG_DATA = 0x10;
constexpr uint8_t LEGACY_POST_CONFIG_COMMAND = 0xC0;

volatile uint32_t g_irqGeneration = 0;
volatile uint32_t g_totalIrqEdges = 0;
volatile bool g_irqPending = false;

FrameSink g_sink = nullptr;
Client g_owner = Client::None;
bool g_ready = false;

uint8_t g_partialBytes[MAX_FRAME_BYTES];
uint8_t g_partialLength = 0;
bool g_partialTruncated = false;

Stats g_stats{};
uint8_t g_stuckLowPasses = 0;

void irqHandler() {
    g_irqGeneration++;
    g_totalIrqEdges++;
    g_irqPending = true;
}

void clearAssembly() {
    g_partialLength = 0;
    g_partialTruncated = false;
}

void exchange(uint8_t dataOut, uint8_t commandOut,
              uint8_t &statusIn, uint8_t &rxDataIn) {
    digitalWrite(PIN_CS, LOW);
    delayMicroseconds(CS_SETUP_US);

    statusIn = SPI1.transfer(dataOut);
    rxDataIn = SPI1.transfer(commandOut);

    digitalWrite(PIN_CS, HIGH);
    delayMicroseconds(CS_HOLD_US);
    g_stats.spiExchanges++;

    if (statusIn & 0x08) {
        g_stats.networkFaults++;
    }
}

Completion decodeCompletion(uint8_t raw) {
    Completion c{};
    c.raw = raw;
    c.error = (raw & 0x80) != 0;
    c.rxFifoOverrun = (raw & 0x40) != 0;
    c.txStatus = static_cast<TxStatus>((raw >> 4) & 0x03);
    c.isIfr = (raw & 0x08) != 0;
    c.ifrHasCrc = (raw & 0x04) != 0;
    c.errorCode = raw & 0x03;
    return c;
}

bool completionAtHead(uint8_t rfs) {
    return rfs == 0x05 || rfs == 0x06 || rfs == 0x07;
}

void appendDataByte(uint8_t value) {
    if (g_partialLength < MAX_FRAME_BYTES) {
        g_partialBytes[g_partialLength++] = value;
    } else {
        g_partialTruncated = true;
    }
}

void finalizeFrame(uint8_t completionStatus, uint8_t completionCode) {
    Frame frame{};
    frame.length = g_partialLength;
    for (uint8_t i = 0; i < g_partialLength; ++i) {
        frame.bytes[i] = g_partialBytes[i];
    }
    frame.completionStatus = completionStatus;
    frame.completion = decodeCompletion(completionCode);
    frame.completedAtUs = micros();
    frame.truncated = g_partialTruncated;

    g_stats.completedFrames++;
    if (frame.completion.error) g_stats.framesWithError++;
    if (frame.completion.rxFifoOverrun) g_stats.rxFifoOverruns++;
    if (frame.truncated) g_stats.truncatedFrames++;

    switch (frame.completion.txStatus) {
        case TxStatus::Successful:
            g_stats.txSuccessful++;
            break;
        case TxStatus::LostArb:
            g_stats.txLostArbitration++;
            break;
        case TxStatus::Underrun:
            g_stats.txUnderrun++;
            break;
        default:
            break;
    }

    clearAssembly();

    if (g_sink) {
        g_stats.deliveredFrames++;
        g_sink(frame);
    }
}

bool serviceOneItem(uint8_t &rfsOut) {
    uint8_t status = 0;
    uint8_t data = 0;

    // RFC=10 returns and flushes the current byte/completion code.
    exchange(0x00, 0x02, status, data);
    const uint8_t rfs = (status >> 5) & 0x07;
    rfsOut = rfs;

    if (rfs == 0x00) {
        return false;
    }

    g_stats.nonEmptyRxItems++;

    if (completionAtHead(rfs)) {
        finalizeFrame(status, data);
    } else {
        appendDataByte(data);
    }

    return true;
}

bool resetAndConfigure(bool countReset) {
    detachInterrupt(digitalPinToInterrupt(PIN_INT));

    digitalWrite(PIN_RST, LOW);
    delay(500);
    digitalWrite(PIN_RST, HIGH);
    delay(5);

    uint8_t status = 0;
    uint8_t data = 0;
    exchange(CONFIG_INTERRUPTS_ENABLED,
             CONFIG_IMMEDIATE_AND_FLUSH_ONE,
             status, data);
    exchange(LEGACY_POST_CONFIG_DATA,
             LEGACY_POST_CONFIG_COMMAND,
             status, data);
    delay(2);

    clearAssembly();
    g_stuckLowPasses = 0;

    noInterrupts();
    g_irqGeneration = 0;
    g_irqPending = false;
    interrupts();

    attachInterrupt(digitalPinToInterrupt(PIN_INT), irqHandler, FALLING);

    if (digitalRead(PIN_INT) == LOW) {
        noInterrupts();
        g_irqPending = true;
        interrupts();
    }

    if (countReset) g_stats.hardResets++;
    return true;
}

}  // namespace

bool begin(FrameSink sink) {
    if (g_ready) {
        g_sink = sink;
        return true;
    }

    g_sink = sink;
    g_owner = Client::None;
    memset(&g_stats, 0, sizeof(g_stats));
    noInterrupts();
    g_totalIrqEdges = 0;
    interrupts();
    clearAssembly();

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

    g_ready = resetAndConfigure(false);
    return g_ready;
}

void service() {
    if (!g_ready) return;

    uint32_t generationAtStart;
    bool pendingAtStart;
    noInterrupts();
    generationAtStart = g_irqGeneration;
    pendingAtStart = g_irqPending;
    g_irqPending = false;
    interrupts();

    if (!pendingAtStart && digitalRead(PIN_INT) != LOW) {
        return;
    }

    uint8_t budget = SERVICE_ITEM_BUDGET;
    bool emptySeenWhileLow = false;
    bool stopAfterCompletion = false;

    while (budget > 0) {
        budget--;
        uint8_t rfs = 0;
        const bool hadItem = serviceOneItem(rfs);

        if (!hadItem) {
            if (!emptySeenWhileLow && digitalRead(PIN_INT) == LOW) {
                // One extra status exchange handles a condition that arrived while
                // CS was asserted or a just-updated FIFO status.
                emptySeenWhileLow = true;
                continue;
            }
            break;
        }

        emptySeenWhileLow = false;

        if (completionAtHead(rfs)) {
            if (rfs == 0x06) {
                // Another complete frame is already buffered.
                continue;
            }

            // RFS=101 may have bytes from a following incomplete frame behind
            // this completion code. Do not poll into that frame; wait for its
            // own completion interrupt. RFS=111 is completion-only.
            stopAfterCompletion = true;
            break;
        }
    }

    if (budget == 0 && !stopAfterCompletion) {
        g_stats.serviceBudgetHits++;
        noInterrupts();
        g_irqPending = true;
        interrupts();
    }

    uint32_t generationNow;
    bool pendingNow;
    noInterrupts();
    generationNow = g_irqGeneration;
    pendingNow = g_irqPending;
    interrupts();

    const bool lineLow = digitalRead(PIN_INT) == LOW;
    const bool newGeneration = generationNow != generationAtStart;

    if (lineLow || newGeneration || pendingNow) {
        noInterrupts();
        g_irqPending = true;
        interrupts();
    }

    if (lineLow && emptySeenWhileLow) {
        g_stuckLowPasses++;
        g_stats.stuckLowRecoveries++;
    } else if (!lineLow) {
        g_stuckLowPasses = 0;
    }
}

bool claim(Client client) {
    if (client == Client::None) return false;
    if (g_owner == Client::None || g_owner == client) {
        g_owner = client;
        return true;
    }
    g_stats.ownerConflicts++;
    return false;
}

void release(Client client) {
    if (client != Client::None && g_owner == client) {
        g_owner = Client::None;
    }
}

bool isOwnedBy(Client client) {
    return g_owner == client;
}

Client owner() {
    return g_owner;
}

bool sendFrame(Client client, const uint8_t *bytes, uint8_t length) {
    if (!g_ready || !bytes || length == 0 || length > 12) return false;
    if (g_owner != client || client == Client::None) {
        g_stats.ownerConflicts++;
        return false;
    }

    // Clear any already-asserted completion before loading a new request.
    service();

    uint8_t status = 0;
    uint8_t data = 0;
    bool rxObserved = false;

    if (length == 1) {
        exchange(bytes[0], 0x1C, status, data);
        rxObserved = ((status >> 5) & 0x07) != 0;
    } else {
        for (uint8_t i = 0; i < length; ++i) {
            uint8_t command = 0x04;
            if (i == 0) command = 0x14;
            else if (i == length - 1) command = 0x0C;
            exchange(bytes[i], command, status, data);
            if (((status >> 5) & 0x07) != 0) rxObserved = true;
        }
    }

    // A transmit exchange reads status and therefore clears INT. If RxFIFO was
    // already nonempty, make sure normal-loop service still drains it even if
    // the physical interrupt does not immediately reassert.
    if (rxObserved || digitalRead(PIN_INT) == LOW) {
        noInterrupts();
        g_irqPending = true;
        interrupts();
    }

    return true;
}

bool isReady() {
    return g_ready;
}

bool intAsserted() {
    return g_ready && digitalRead(PIN_INT) == LOW;
}

Stats snapshotStats() {
    Stats copy;
    noInterrupts();
    copy = g_stats;
    copy.irqEdges = g_totalIrqEdges;
    interrupts();
    return copy;
}

void clearStats() {
    noInterrupts();
    memset(&g_stats, 0, sizeof(g_stats));
    g_totalIrqEdges = 0;
    interrupts();
}

bool hardReset() {
    if (!g_ready) return false;
    g_owner = Client::None;
    return resetAndConfigure(true);
}

}  // namespace HC58Transport
