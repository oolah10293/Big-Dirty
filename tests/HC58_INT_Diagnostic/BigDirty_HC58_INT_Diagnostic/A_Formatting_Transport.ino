// -----------------------------------------------------------------------------
// Formatting helpers
// -----------------------------------------------------------------------------
static void printHexByte(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

static const char *modeName(uint8_t mode) {
  switch (mode) {
    case 0: return "MASKED_POLL";
    case 1: return "IRQ_POLL";
    case 2: return "IRQ_ONLY";
    default: return "UNKNOWN";
  }
}

static const char *tmsName(uint8_t tms) {
  switch (tms & 0x03) {
    case 0: return "NOT_INVOLVED";
    case 1: return "UNDERRUN";
    case 2: return "LOST_ARB";
    case 3: return "TX_OK";
    default: return "?";
  }
}

static const char *frameName(const FrameRecord &frame) {
  if (frame.length >= 5 &&
      frame.bytes[0] == 0x68 && frame.bytes[1] == 0x6A &&
      frame.bytes[2] == 0xF1 && frame.bytes[3] == 0x01 &&
      frame.bytes[4] == TEST_PID_MAP) {
    return "OUR_REQUEST";
  }

  for (uint8_t i = 0; i + 5 < frame.length; ++i) {
    if (frame.bytes[i]     == 0x48 &&
        frame.bytes[i + 1] == 0x6B &&
        frame.bytes[i + 2] == 0x10 &&
        frame.bytes[i + 3] == 0x41 &&
        frame.bytes[i + 4] == TEST_PID_MAP) {
      return "PCM_MAP_REPLY";
    }
  }

  return "BUS_FRAME";
}

// -----------------------------------------------------------------------------
// HC58 transport
// -----------------------------------------------------------------------------
static void spiCycle(uint8_t dataOut, uint8_t commandOut,
                     uint8_t &statusIn, uint8_t &dataIn) {
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(CS_SETUP_US);

  statusIn = SPI1.transfer(dataOut);
  dataIn   = SPI1.transfer(commandOut);

  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(CS_HOLD_US);
}

static void setupSpi1() {
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
}

static void resetAndConfigureHc58(uint8_t configurationByte) {
  uint8_t status = 0;
  uint8_t data = 0;

  digitalWrite(PIN_RST, LOW);
  delay(500);
  digitalWrite(PIN_RST, HIGH);
  delay(5);

  spiCycle(configurationByte, CONFIG_IMMEDIATE_AND_FLUSH_ONE, status, data);

  // Preserve the known-working Big Dirty initialization sequence in both A/B
  // cases. This keeps IMSK as the only configuration difference.
  spiCycle(LEGACY_POST_CONFIG_DATA, LEGACY_POST_CONFIG_COMMAND, status, data);

  delay(2);
}

static void sendMapRequest() {
  uint8_t status = 0;
  uint8_t data = 0;

  spiCycle(0x68, 0x14, status, data);  // first byte
  spiCycle(0x6A, 0x04, status, data);
  spiCycle(0xF1, 0x04, status, data);
  spiCycle(0x01, 0x04, status, data);
  spiCycle(TEST_PID_MAP, 0x0C, status, data);  // last byte
}
