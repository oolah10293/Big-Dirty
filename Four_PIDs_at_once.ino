/********************************************************************
 * Build_2025-10-19m_GoldenTiming_4PID_InsightLogger
 * -------------------------------------------------
 * Multi-PID live data logger for MAP, IAT, MAF, and Throttle
 * Uses the tuned golden timing chain from 2025-10-19l_LiveStable.
 *
 *   Post-TX   = 8.3 ms
 *   EOD-settle= 750 us
 *   Drain-win = 13.0 ms
 *   Poll-gap  = 5 ms
 *
 * Logs two lines per PID cycle:
 *   [TX] timestamp, PID, drain info, and sync stats
 *   Decoded engineering values (MAP, IAT, MAF, TPS)
 *
 * Platform: Arduino Uno (16 MHz)
 * HC58 Init: CCR-first (0x12→0x1A), RENA + TENA (0x10→0xC0)
 ********************************************************************/

#include <SPI.h>
#define csPIN  10
#define rstPIN 8

// === Golden timing constants ===
const float POSTTX_MS        = 8.3;
const float DRAIN_WINDOW_MS  = 13.0;
const uint16_t EOD_US        = 750;
const uint16_t POLL_GAP_MS   = 5;

const byte pids[] = {0x0B, 0x0F, 0x10, 0x11};   // MAP, IAT, MAF, TPS
const int NUM_PIDS = sizeof(pids);
byte pidIndex = 0;

byte frame[96];
int  frameLen = 0;

// -----------------------------------------------------------------
// SPI primitives
// -----------------------------------------------------------------
void SPIcycle(byte cmd, byte data, byte &status, byte &resp) {
  digitalWrite(csPIN, LOW);
  delayMicroseconds(40);
  status = SPI.transfer(cmd);
  resp   = SPI.transfer(data);
  digitalWrite(csPIN, HIGH);
  delayMicroseconds(40);
}

void sendPIDRequest(byte pid) {
  byte s, d;
  SPIcycle(0x68, 0x14, s, d);
  SPIcycle(0x6A, 0x04, s, d);
  SPIcycle(0xF1, 0x04, s, d);
  SPIcycle(0x01, 0x04, s, d);
  SPIcycle(pid, 0x0C, s, d);
}

bool drainFIFO(float drainWindow_ms, int &lenOut) {
  byte s, d;
  frameLen = 0;
  unsigned long start = millis();
  while (millis() - start < drainWindow_ms) {
    SPIcycle(0x00, 0x02, s, d);
    if (s != 0x10 && s != 0x00) {
      if (frameLen < (int)sizeof(frame)) frame[frameLen++] = d;
    }
  }
  lenOut = frameLen;
  for (int i = 0; i < frameLen - 4; i++) {
    if (frame[i]==0x48 && frame[i+1]==0x6B &&
        frame[i+2]==0x10 && frame[i+3]==0x41) return true;
  }
  return false;
}

// -----------------------------------------------------------------
// Parse standard Mode 01 PID replies
// -----------------------------------------------------------------
void parsePID(byte *buf, int len) {
  for (int i = 0; i < len - 4; i++) {
    if (buf[i]==0x48 && buf[i+1]==0x6B && buf[i+2]==0x10 && buf[i+3]==0x41) {
      byte pid = buf[i+4];
      switch (pid) {
        case 0x0B: { int map = buf[i+5];
                     Serial.print("MAP="); Serial.print(map); Serial.print(" "); break; }
        case 0x0F: { int iat = buf[i+5]-40;
                     Serial.print("IAT="); Serial.print(iat); Serial.print(" "); break; }
        case 0x10: { float maf = ((buf[i+5]*256)+buf[i+6])/100.0;
                     Serial.print("MAF="); Serial.print(maf,2); Serial.print(" "); break; }
        case 0x11: { int tps = (buf[i+5]*100)/255;
                     Serial.print("TPS="); Serial.print(tps); Serial.print(" "); break; }
      }
    }
  }
}

// -----------------------------------------------------------------
void hc58_init() {
  byte s,d;
  digitalWrite(rstPIN, LOW); delay(500);
  digitalWrite(rstPIN, HIGH); delay(5);
  SPIcycle(0x12, 0x1A, s, d);  // CCR-first
  SPIcycle(0x10, 0xC0, s, d);  // RENA + TENA
  Serial.println(F("HC58 initialized (CCR-first, RENA+TENA)"));
}

// -----------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(rstPIN, OUTPUT);
  pinMode(csPIN, OUTPUT);
  SPI.begin();
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
  hc58_init();

  Serial.println(F("\n=== Build_2025-10-19m_GoldenTiming_4PID_InsightLogger ==="));
  Serial.println(F("Polling MAP, IAT, MAF, TPS — dual-line log mode"));
  Serial.println(F("PostTX=8.3ms, EOD=750us, Drain=13.0ms, Gap=5ms\n"));
}

// -----------------------------------------------------------------
void loop() {
  static uint32_t good=0, bad=0, total=0;
  static float syncHistory[3] = {0,0,0};
  static uint8_t idx=0;
  static unsigned long lastReport=0;

  byte pid = pids[pidIndex];
  pidIndex = (pidIndex + 1) % NUM_PIDS;

  unsigned long tStart = millis();
  sendPIDRequest(pid);
  delay((unsigned long)POSTTX_MS);
  delayMicroseconds(EOD_US);

  int len=0;
  bool ok = drainFIFO(DRAIN_WINDOW_MS, len);
  total++;
  if (ok) good++; else bad++;

  // Compute instantaneous and rolling sync
  float sync = (total>0)? (100.0*good/total):0;
  syncHistory[idx] = sync;
  idx = (idx + 1) % 3;
  float avgSync = (syncHistory[0] + syncHistory[1] + syncHistory[2]) / 3.0;

  // Diagnostic line
  Serial.print("[TX] t="); Serial.print(tStart);
  Serial.print(" PID="); Serial.print(pid, HEX);
  Serial.print(" Drain="); Serial.print(DRAIN_WINDOW_MS,1);
  Serial.print("ms Len="); Serial.print(len);
  Serial.print(" Sync="); Serial.print(avgSync,1);
  Serial.println("%");

  // Result line
  if (ok) {
    parsePID(frame, len);
    Serial.println();
  } else {
    Serial.println("NO FRAME");
  }

  // Periodic summary
  unsigned long now = millis();
  if (now - lastReport >= 3000) {
    Serial.print("SyncAvg="); Serial.print(avgSync,1);
    Serial.println("%");
    lastReport = now;
  }

  delay(POLL_GAP_MS);
}
