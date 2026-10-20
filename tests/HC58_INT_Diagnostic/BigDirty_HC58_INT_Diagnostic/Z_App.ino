void setup() {
  Serial.begin(115200);

  // This is a diagnostic logger. Waiting prevents the evidence from being lost
  // before the USB serial monitor is opened.
  while (!Serial) {
    yield();
  }

  delay(500);

  Serial.println();
  Serial.println("Big Dirty MC68HC58 INT Diagnostic");
  Serial.println("Build: complete masked / IRQ-poll / IRQ-only proof sketch");
  Serial.println("Target: Teensy 4.1 + Fixed Until Broken Rev C module");
  Serial.println("Test PID: Mode 01 PID 0B (MAP)");
  Serial.println("USB serial: 115200 baud");

  setupSpi1();
  attachInterrupt(digitalPinToInterrupt(PIN_INT), hc58InterruptHandler, FALLING);

  Serial.print("INT pin at startup: ");
  Serial.println(digitalRead(PIN_INT));
  Serial.println("Starting full test in 3 seconds...");
  delay(3000);

  runFullTest(DEFAULT_REQUESTS_PER_MODE);
  printHelp();
}

void loop() {
  if (!Serial.available()) {
    yield();
    return;
  }

  const char command = (char)Serial.read();

  switch (command) {
    case 'R':
    case 'r':
      runFullTest(DEFAULT_REQUESTS_PER_MODE);
      break;

    case 'M':
    case 'm':
      runBatch(0, LONG_REQUESTS);
      break;

    case 'P':
    case 'p':
      runBatch(1, LONG_REQUESTS);
      break;

    case 'I':
    case 'i':
      runBatch(2, LONG_REQUESTS);
      break;

    case 'H':
    case 'h':
      printHelp();
      break;

    case '\r':
    case '\n':
      break;

    default:
      Serial.print("Unknown command: ");
      Serial.println(command);
      printHelp();
      break;
  }
}
