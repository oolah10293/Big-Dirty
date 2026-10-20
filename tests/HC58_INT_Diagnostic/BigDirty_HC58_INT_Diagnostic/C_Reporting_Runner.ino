// -----------------------------------------------------------------------------
// Evidence output
// -----------------------------------------------------------------------------
static void printIrqEvidence(const IrqSnapshot &irq,
                             uint32_t requestStartUs) {
  Serial.print(" IRQ_COUNT=");
  Serial.print(irq.count);
  Serial.print(" IRQ_OFFSETS_US=");

  if (irq.count == 0) {
    Serial.print('-');
  } else {
    for (uint8_t i = 0; i < irq.count; ++i) {
      if (i) Serial.print(',');
      Serial.print((uint32_t)(irq.timesUs[i] - requestStartUs));
    }
  }

  Serial.print(" IRQ_OVERFLOW=");
  Serial.print(irq.overflow ? 1 : 0);
}

static void printFrame(uint16_t requestNumber,
                       uint8_t frameIndex,
                       const FrameRecord &frame,
                       uint32_t requestStartUs) {
  Serial.print("FRAME REQ=");
  Serial.print(requestNumber);
  Serial.print(" IDX=");
  Serial.print(frameIndex);
  Serial.print(" TYPE=");
  Serial.print(frameName(frame));
  Serial.print(" BYTES=");

  if (frame.length == 0) {
    Serial.print('-');
  } else {
    for (uint8_t i = 0; i < frame.length; ++i) {
      if (i) Serial.print(' ');
      printHexByte(frame.bytes[i]);
    }
  }

  Serial.print(" CC=");
  printHexByte(frame.completionCode);
  Serial.print(" ERRF=");
  Serial.print((frame.completionCode >> 7) & 0x01);
  Serial.print(" RFO=");
  Serial.print((frame.completionCode >> 6) & 0x01);
  Serial.print(" TMS=");
  Serial.print(tmsName((frame.completionCode >> 4) & 0x03));
  Serial.print(" IFR=");
  Serial.print((frame.completionCode >> 3) & 0x01);
  Serial.print(" IFRC=");
  Serial.print((frame.completionCode >> 2) & 0x01);
  Serial.print(" ERRC=");
  Serial.print(frame.completionCode & 0x03);
  Serial.print(" COMPLETE_US=");
  Serial.print((uint32_t)(frame.completedAtUs - requestStartUs));
  Serial.print(" BYTE_OVERFLOW=");
  Serial.println(frame.byteOverflow ? 1 : 0);
}

struct BatchStats {
  uint16_t requests;
  uint16_t validReplies;
  uint16_t timeouts;
  uint16_t requestsWithIrq;
  uint16_t requestsWithMultipleIrqs;
  uint32_t irqEdges;
  uint16_t irqLogOverflows;
  uint16_t cleanupRequired;
  uint16_t staleItemsBeforeRequest;
  uint16_t frameListOverflows;
};

static void clearBatchStats(BatchStats &stats) {
  memset(&stats, 0, sizeof(stats));
}

static void printRequestEvidence(uint8_t mode,
                                 uint16_t requestNumber,
                                 uint32_t requestStartUs,
                                 const RequestCapture &capture,
                                 const IrqSnapshot &irq,
                                 int intLevelBefore,
                                 int intLevelAfter) {
  Serial.print("RESULT MODE=");
  Serial.print(modeName(mode));
  Serial.print(" REQ=");
  Serial.print(requestNumber);
  Serial.print(" PID=0B");
  Serial.print(" VALID=");
  Serial.print(capture.expectedMapReplyFound ? 1 : 0);
  Serial.print(" MAP_KPA=");
  if (capture.expectedMapReplyFound) Serial.print(capture.mapKpa);
  else Serial.print('-');
  Serial.print(" TIMEOUT=");
  Serial.print(capture.timedOut ? 1 : 0);
  Serial.print(" CLEANUP_REQUIRED=");
  Serial.print(capture.cleanupRequired ? 1 : 0);
  Serial.print(" INT_BEFORE=");
  Serial.print(intLevelBefore);
  Serial.print(" INT_AFTER=");
  Serial.print(intLevelAfter);
  printIrqEvidence(irq, requestStartUs);
  Serial.print(" FRAMES=");
  Serial.print(capture.frameCount);
  Serial.print(" FIFO_ITEMS=");
  Serial.print(capture.nonEmptyItems);
  Serial.print(" SPI_CYCLES=");
  Serial.print(capture.spiServiceCycles);
  Serial.print(" FRAME_LIST_OVERFLOW=");
  Serial.println(capture.frameListOverflow ? 1 : 0);

  for (uint8_t i = 0; i < capture.frameCount; ++i) {
    printFrame(requestNumber, i, capture.frames[i], requestStartUs);
  }
}

// mode: 0=masked+poll, 1=IRQ enabled+poll, 2=IRQ enabled+IRQ-only
static BatchStats runBatch(uint8_t mode, uint16_t requestCount) {
  BatchStats stats{};
  clearBatchStats(stats);
  stats.requests = requestCount;

  const uint8_t config =
      (mode == 0) ? CONFIG_INTERRUPTS_MASKED : CONFIG_DEFAULT_INTERRUPTS;

  Serial.println();
  Serial.println("================================================================");
  Serial.print("BATCH_BEGIN MODE=");
  Serial.print(modeName(mode));
  Serial.print(" REQUESTS=");
  Serial.print(requestCount);
  Serial.print(" CONFIG=0x");
  printHexByte(config);
  Serial.println();

  resetAndConfigureHc58(config);
  const uint16_t initialDiscard = discardRxFifo();
  resetRequestIrqLog();

  Serial.print("INIT INT_LEVEL=");
  Serial.print(digitalRead(PIN_INT));
  Serial.print(" DISCARDED_ITEMS=");
  Serial.println(initialDiscard);

  for (uint16_t requestNumber = 1; requestNumber <= requestCount; ++requestNumber) {
    const uint32_t intervalStartMs = millis();

    const uint16_t stale = discardRxFifo();
    stats.staleItemsBeforeRequest += stale;

    resetRequestIrqLog();
    const int intLevelBefore = digitalRead(PIN_INT);
    const uint32_t requestStartUs = micros();

    sendMapRequest();

    RequestCapture capture{};
    clearCapture(capture);

    if (mode == 2) captureOnlyAfterInterrupt(capture);
    else captureWithTimedPolling(capture);

    const IrqSnapshot irq = snapshotRequestIrqs();
    const int intLevelAfter = digitalRead(PIN_INT);

    if (capture.expectedMapReplyFound) stats.validReplies++;
    if (capture.timedOut) stats.timeouts++;
    if (irq.count > 0) stats.requestsWithIrq++;
    if (irq.count > 1) stats.requestsWithMultipleIrqs++;
    stats.irqEdges += irq.count;
    if (irq.overflow) stats.irqLogOverflows++;
    if (capture.cleanupRequired) stats.cleanupRequired++;
    if (capture.frameListOverflow) stats.frameListOverflows++;

    if (stale > 0) {
      Serial.print("STALE REQ=");
      Serial.print(requestNumber);
      Serial.print(" ITEMS=");
      Serial.println(stale);
    }

    printRequestEvidence(mode, requestNumber, requestStartUs, capture, irq,
                         intLevelBefore, intLevelAfter);

    const uint32_t elapsedMs = millis() - intervalStartMs;
    if (elapsedMs < REQUEST_INTERVAL_MS) {
      delay(REQUEST_INTERVAL_MS - elapsedMs);
    }
  }

  Serial.print("BATCH_SUMMARY MODE=");
  Serial.print(modeName(mode));
  Serial.print(" REQUESTS=");
  Serial.print(stats.requests);
  Serial.print(" VALID=");
  Serial.print(stats.validReplies);
  Serial.print(" TIMEOUTS=");
  Serial.print(stats.timeouts);
  Serial.print(" IRQ_EDGES=");
  Serial.print(stats.irqEdges);
  Serial.print(" REQUESTS_WITH_IRQ=");
  Serial.print(stats.requestsWithIrq);
  Serial.print(" REQUESTS_WITH_MULTIPLE_IRQS=");
  Serial.print(stats.requestsWithMultipleIrqs);
  Serial.print(" IRQ_LOG_OVERFLOWS=");
  Serial.print(stats.irqLogOverflows);
  Serial.print(" CLEANUP_REQUIRED=");
  Serial.print(stats.cleanupRequired);
  Serial.print(" STALE_ITEMS=");
  Serial.print(stats.staleItemsBeforeRequest);
  Serial.print(" FRAME_LIST_OVERFLOWS=");
  Serial.println(stats.frameListOverflows);
  Serial.println("BATCH_END");

  return stats;
}

static void printVerdict(const BatchStats &masked,
                         const BatchStats &irqPoll,
                         const BatchStats &irqOnly) {
  const bool maskedReceiverWorked = masked.validReplies == masked.requests;
  const bool maskedHadNoIrqs = masked.irqEdges == 0;

  const bool irqPollReceiverWorked = irqPoll.validReplies == irqPoll.requests;
  const bool irqPollSawIrqs = irqPoll.requestsWithIrq == irqPoll.requests;

  const bool irqOnlyReceiverWorked = irqOnly.validReplies == irqOnly.requests;
  const bool irqOnlySawIrqs = irqOnly.requestsWithIrq == irqOnly.requests;
  const bool irqOnlyHadNoTimeouts = irqOnly.timeouts == 0;
  const bool irqOnlyNeededNoCleanup = irqOnly.cleanupRequired == 0;

  Serial.println();
  Serial.println("################################################################");
  Serial.print("VERDICT MASKED_RECEIVE=");
  Serial.println(maskedReceiverWorked ? "PASS" : "FAIL");
  Serial.print("VERDICT MASKED_ZERO_IRQ=");
  Serial.println(maskedHadNoIrqs ? "PASS" : "FAIL");
  Serial.print("VERDICT IRQ_POLL_RECEIVE=");
  Serial.println(irqPollReceiverWorked ? "PASS" : "FAIL");
  Serial.print("VERDICT IRQ_CORRELATION=");
  Serial.println(irqPollSawIrqs ? "PASS" : "FAIL");
  Serial.print("VERDICT IRQ_ONLY_RECEIVE=");
  Serial.println(irqOnlyReceiverWorked ? "PASS" : "FAIL");
  Serial.print("VERDICT IRQ_ONLY_EVERY_REQUEST_HAD_IRQ=");
  Serial.println(irqOnlySawIrqs ? "PASS" : "FAIL");
  Serial.print("VERDICT IRQ_ONLY_ZERO_TIMEOUTS=");
  Serial.println(irqOnlyHadNoTimeouts ? "PASS" : "FAIL");
  Serial.print("VERDICT IRQ_ONLY_NO_FALLBACK_CLEANUP=");
  Serial.println(irqOnlyNeededNoCleanup ? "PASS" : "FAIL");

  const bool fullPass = maskedReceiverWorked && maskedHadNoIrqs &&
                        irqPollReceiverWorked && irqPollSawIrqs &&
                        irqOnlyReceiverWorked && irqOnlySawIrqs &&
                        irqOnlyHadNoTimeouts && irqOnlyNeededNoCleanup;

  Serial.print("OVERALL_INT_PROOF=");
  Serial.println(fullPass ? "PASS" : "FAIL");
  Serial.println("################################################################");
}

static void runFullTest(uint16_t requestCount) {
  Serial.println();
  Serial.println("FULL_TEST_BEGIN");
  Serial.println("The same MAP request is used in all three modes.");
  Serial.println("IRQ_ONLY never polls RxFIFO before an ISR-recorded falling edge.");

  const BatchStats masked  = runBatch(0, requestCount);
  const BatchStats irqPoll = runBatch(1, requestCount);
  const BatchStats irqOnly = runBatch(2, requestCount);

  printVerdict(masked, irqPoll, irqOnly);
  Serial.println("FULL_TEST_END");
}

static void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  R - rerun complete 20/20/20 proof suite");
  Serial.println("  M - run 100 requests, interrupts masked + polling");
  Serial.println("  P - run 100 requests, interrupts enabled + polling");
  Serial.println("  I - run 100 requests, interrupts enabled + IRQ-only receive");
  Serial.println("  H - print this help");
}
