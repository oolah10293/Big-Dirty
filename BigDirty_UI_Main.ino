#include <SPI.h>
#include <SD.h>
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"
#include "RawImage.h"
#include "PageManager.h"
#include "PinConfig.h"
#include "Overlay_Warnings.h"
#include "HC58_Transport.h"
#include "PID_Manager.h"
#include "DTC_Manager.h"

static void onHc58Frame(const HC58Transport::Frame &frame) {
  PIDManager_onHc58Frame(frame);
  DTCManager_onHc58Frame(frame);
}

static uint32_t nextHc58DiagnosticMs = 0;
static bool buildIdPrinted = false;

static void printHc58DiagnosticsIfConnected() {
  if (!Serial) return;

  if (!buildIdPrinted) {
    Serial.println("BIGDIRTY_BUILD=HC58_INT_LIVE_TEST_2");
    buildIdPrinted = true;
  }

  uint32_t now = millis();
  if (static_cast<int32_t>(now - nextHc58DiagnosticMs) < 0) return;
  nextHc58DiagnosticMs = now + 5000;

  HC58Transport::Stats s = HC58Transport::snapshotStats();
  Serial.print("HC58 IRQ=");
  Serial.print(s.irqEdges);
  Serial.print(" FRAME=");
  Serial.print(s.completedFrames);
  Serial.print(" RFO=");
  Serial.print(s.rxFifoOverruns);
  Serial.print(" ERR=");
  Serial.print(s.framesWithError);
  Serial.print(" TRUNC=");
  Serial.print(s.truncatedFrames);
  Serial.print(" BUDGET=");
  Serial.print(s.serviceBudgetHits);
  Serial.print(" STUCK=");
  Serial.print(s.stuckLowRecoveries);
  Serial.print(" CONFLICT=");
  Serial.print(s.ownerConflicts);
  Serial.print(" OWNER=");
  Serial.print(static_cast<int>(HC58Transport::owner()));
  Serial.print(" PAGE=");
  Serial.println(static_cast<int>(PageManager_current()));
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SD.begin(BUILTIN_SDCARD);
  initDisplay();
  backlightControl_init();
  initTouch();

  pinMode(PIN_SEATBELT, INPUT);
  pinMode(PIN_BRAKE, INPUT);
  pinMode(PIN_HIGHBEAM, INPUT);
  pinMode(PIN_CHECK_ENGINE, INPUT);
  pinMode(PIN_4WD, INPUT);

  Overlay_Warnings_begin();
  HC58Transport::begin(onHc58Frame);
  PIDManager_begin();
  DTCManager_begin();
  PageManager_begin();
}

void loop() {
  // Service completed J1850 frames continuously, regardless of active page.
  HC58Transport::service();

  bool seatbeltOn    = (digitalRead(PIN_SEATBELT) == HIGH);
  bool brakeOn       = (digitalRead(PIN_BRAKE) == HIGH);
  bool highBeamOn    = (digitalRead(PIN_HIGHBEAM) == HIGH);
  bool checkEngineOn = (digitalRead(PIN_CHECK_ENGINE) == LOW);
  bool fourWdOn      = (digitalRead(PIN_4WD) == LOW);

  PageManager_update();

  // A second service point protects RxFIFO after page drawing or sensor work.
  HC58Transport::service();

  if (PageManager_current() != PAGE_BOOTLOGO) {
    Overlay_Warnings_update(
      seatbeltOn,
      brakeOn,
      highBeamOn,
      checkEngineOn,
      fourWdOn
    );
  }

  backlightControl_update();
  printHc58DiagnosticsIfConnected();
}
