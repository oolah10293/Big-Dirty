#include <Arduino.h>
#include <SPI.h>
#include <string.h>

/*
  Big Dirty - MC68HC58 INT Diagnostic

  One standalone sketch runs the complete first interrupt test discussed:
    1) current masked configuration + timed polling control
    2) default interrupts enabled + the same timed polling
    3) default interrupts enabled + receive service only after INT

  No display, touch, SD card, sensors, warning inputs, or page code is used.
  The existing Big Dirty HC58 wiring and SPI1 pin assignments are preserved.
*/

// -----------------------------------------------------------------------------
// Teensy 4.1 <-> Fixed Until Broken Rev C HC58 module
// -----------------------------------------------------------------------------
static constexpr uint8_t PIN_INT  = 0;
static constexpr uint8_t PIN_MISO = 1;
static constexpr uint8_t PIN_RST  = 24;
static constexpr uint8_t PIN_CS   = 25;
static constexpr uint8_t PIN_MOSI = 26;
static constexpr uint8_t PIN_SCK  = 27;

// -----------------------------------------------------------------------------
// HC58 / SPI settings copied from the working Big Dirty firmware
// -----------------------------------------------------------------------------
static constexpr uint32_t SPI_HZ      = 100000;
static constexpr uint32_t CS_SETUP_US = 40;
static constexpr uint32_t CS_HOLD_US  = 40;

// Current Big Dirty configuration: IMSK=1, IMOD=0, OSCD=01, 4X=0.
static constexpr uint8_t CONFIG_INTERRUPTS_MASKED = 0x12;

// Exact A/B change: clear only IMSK (bit 4).
// IMSK=0, IMOD=0, OSCD=01, 4X=0.
static constexpr uint8_t CONFIG_DEFAULT_INTERRUPTS = 0x02;

// Preserve the current firmware's initialization command and post-config cycle
// so the A/B comparison changes only IMSK.
static constexpr uint8_t CONFIG_IMMEDIATE_AND_FLUSH_ONE = 0x1A;
static constexpr uint8_t LEGACY_POST_CONFIG_DATA        = 0x10;
static constexpr uint8_t LEGACY_POST_CONFIG_COMMAND     = 0xC0;

// -----------------------------------------------------------------------------
// Test request and timing
// -----------------------------------------------------------------------------
static constexpr uint8_t TEST_PID_MAP = 0x0B;

static constexpr uint32_t POST_TX_MS          = 9;
static constexpr uint32_t EOD_SETTLE_US       = 750;
static constexpr uint32_t POLL_QUIET_GAP_MS   = 15;
static constexpr uint32_t POLL_CAPTURE_MAX_MS = 80;
static constexpr uint32_t IRQ_ONLY_TIMEOUT_MS = 500;
static constexpr uint32_t REQUEST_INTERVAL_MS = 1000;

static constexpr uint16_t DEFAULT_REQUESTS_PER_MODE = 20;
static constexpr uint16_t LONG_REQUESTS             = 100;

// -----------------------------------------------------------------------------
// ISR evidence capture
// -----------------------------------------------------------------------------
static constexpr uint8_t MAX_IRQ_TIMESTAMPS = 16;

volatile uint32_t g_irqTimesUs[MAX_IRQ_TIMESTAMPS];
volatile uint8_t  g_irqCount = 0;
volatile bool     g_irqOverflow = false;
volatile uint32_t g_totalIrqEdges = 0;

void hc58InterruptHandler() {
  const uint8_t index = g_irqCount;
  if (index < MAX_IRQ_TIMESTAMPS) {
    g_irqTimesUs[index] = micros();
    g_irqCount = index + 1;
  } else {
    g_irqOverflow = true;
  }
  g_totalIrqEdges++;
}

struct IrqSnapshot {
  uint8_t count;
  bool overflow;
  uint32_t timesUs[MAX_IRQ_TIMESTAMPS];
};

static void resetRequestIrqLog() {
  noInterrupts();
  g_irqCount = 0;
  g_irqOverflow = false;
  interrupts();
}

static IrqSnapshot snapshotRequestIrqs() {
  IrqSnapshot snapshot{};

  noInterrupts();
  snapshot.count = g_irqCount;
  snapshot.overflow = g_irqOverflow;
  for (uint8_t i = 0; i < snapshot.count && i < MAX_IRQ_TIMESTAMPS; ++i) {
    snapshot.timesUs[i] = g_irqTimesUs[i];
  }
  interrupts();

  return snapshot;
}

// -----------------------------------------------------------------------------
// Capture records
// -----------------------------------------------------------------------------
static constexpr uint8_t MAX_FRAME_BYTES = 24;
static constexpr uint8_t MAX_FRAMES_PER_REQUEST = 12;

struct FrameRecord {
  uint8_t bytes[MAX_FRAME_BYTES];
  uint8_t length;
  uint8_t completionCode;
  uint8_t completionStatus;
  uint32_t completedAtUs;
  bool byteOverflow;
};

struct RequestCapture {
  FrameRecord frames[MAX_FRAMES_PER_REQUEST];
  uint8_t frameCount;

  uint8_t currentFrame[MAX_FRAME_BYTES];
  uint8_t currentLength;
  bool currentOverflow;
  bool frameListOverflow;

  bool expectedMapReplyFound;
  uint8_t mapKpa;

  uint16_t spiServiceCycles;
  uint16_t nonEmptyItems;
  uint16_t emptyReads;
  bool timedOut;
  bool cleanupRequired;
};

static void clearCapture(RequestCapture &capture) {
  memset(&capture, 0, sizeof(capture));
}
