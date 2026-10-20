// -----------------------------------------------------------------------------
// RxFIFO servicing based on the HC58 RFS field
// -----------------------------------------------------------------------------
static bool completionAtHead(uint8_t rfs) {
  return rfs == 0x05 || rfs == 0x06 || rfs == 0x07;
}

static void inspectExpectedReply(RequestCapture &capture, const FrameRecord &frame) {
  for (uint8_t i = 0; i + 5 < frame.length; ++i) {
    if (frame.bytes[i]     == 0x48 &&
        frame.bytes[i + 1] == 0x6B &&
        frame.bytes[i + 2] == 0x10 &&
        frame.bytes[i + 3] == 0x41 &&
        frame.bytes[i + 4] == TEST_PID_MAP) {
      capture.expectedMapReplyFound = true;
      capture.mapKpa = frame.bytes[i + 5];
      return;
    }
  }
}

static void finalizeCurrentFrame(RequestCapture &capture,
                                 uint8_t completionCode,
                                 uint8_t completionStatus) {
  if (capture.frameCount >= MAX_FRAMES_PER_REQUEST) {
    capture.frameListOverflow = true;
    capture.currentLength = 0;
    capture.currentOverflow = false;
    return;
  }

  FrameRecord &frame = capture.frames[capture.frameCount++];
  memset(&frame, 0, sizeof(frame));

  frame.length = capture.currentLength;
  frame.byteOverflow = capture.currentOverflow;
  for (uint8_t i = 0; i < frame.length; ++i) {
    frame.bytes[i] = capture.currentFrame[i];
  }

  frame.completionCode = completionCode;
  frame.completionStatus = completionStatus;
  frame.completedAtUs = micros();

  inspectExpectedReply(capture, frame);

  capture.currentLength = 0;
  capture.currentOverflow = false;
}

static bool serviceOneRxItem(RequestCapture &capture, uint8_t *rfsOut) {
  uint8_t status = 0;
  uint8_t data = 0;

  // RFC=10 flushes the byte/completion code returned in this same exchange.
  spiCycle(0x00, 0x02, status, data);
  capture.spiServiceCycles++;

  const uint8_t rfs = (status >> 5) & 0x07;
  if (rfsOut) *rfsOut = rfs;

  if (rfs == 0x00) {
    capture.emptyReads++;
    return false;
  }

  capture.nonEmptyItems++;

  if (completionAtHead(rfs)) {
    finalizeCurrentFrame(capture, data, status);
  } else {
    if (capture.currentLength < MAX_FRAME_BYTES) {
      capture.currentFrame[capture.currentLength++] = data;
    } else {
      capture.currentOverflow = true;
    }
  }

  return true;
}

static uint16_t serviceUntilEmpty(RequestCapture &capture) {
  uint16_t serviced = 0;

  // 20-byte RxFIFO plus completion codes. 64 is deliberately bounded but
  // comfortably above one normal burst.
  for (uint8_t i = 0; i < 64; ++i) {
    if (!serviceOneRxItem(capture, nullptr)) break;
    serviced++;
  }

  return serviced;
}

static uint16_t serviceCompletedFramesAfterInterrupt(RequestCapture &capture) {
  uint16_t serviced = 0;

  // Service the data leading to the completion code that caused INT. If RFS=110,
  // another complete frame is already queued, so it is also safe to continue.
  // For RFS=101, bytes from a following but not-yet-complete frame are present;
  // stop immediately after flushing the current completion code and wait for the
  // next completion interrupt rather than polling into that next frame.
  for (uint8_t i = 0; i < 64; ++i) {
    uint8_t rfs = 0;
    if (!serviceOneRxItem(capture, &rfs)) break;
    serviced++;

    if (completionAtHead(rfs)) {
      if (rfs == 0x06) continue;  // another complete frame is already available
      break;                       // 101 or 111: wait for the next INT condition
    }
  }

  return serviced;
}

static uint16_t discardRxFifo() {
  RequestCapture discard{};
  clearCapture(discard);
  return serviceUntilEmpty(discard);
}

// -----------------------------------------------------------------------------
// Receive strategies
// -----------------------------------------------------------------------------
static void captureWithTimedPolling(RequestCapture &capture) {
  delay(POST_TX_MS);
  delayMicroseconds(EOD_SETTLE_US);

  const uint32_t startedMs = millis();
  uint32_t lastNonEmptyMs = startedMs;
  bool sawAnything = false;

  while ((millis() - startedMs) < POLL_CAPTURE_MAX_MS) {
    if (serviceOneRxItem(capture, nullptr)) {
      sawAnything = true;
      lastNonEmptyMs = millis();
    } else if (sawAnything &&
               (millis() - lastNonEmptyMs) >= POLL_QUIET_GAP_MS) {
      break;
    }
  }
}

static void captureOnlyAfterInterrupt(RequestCapture &capture) {
  const uint32_t startedMs = millis();
  uint8_t handledIrqCount = 0;

  while ((millis() - startedMs) < IRQ_ONLY_TIMEOUT_MS) {
    uint8_t observedIrqCount;

    noInterrupts();
    observedIrqCount = g_irqCount;
    interrupts();

    if (observedIrqCount != handledIrqCount) {
      serviceCompletedFramesAfterInterrupt(capture);
      handledIrqCount = observedIrqCount;

      if (capture.expectedMapReplyFound) return;
      continue;
    }

    // Intentionally no RxFIFO polling here. A successful IRQ-only result must
    // be reached through an ISR-recorded falling edge.
    yield();
  }

  capture.timedOut = true;

  // Cleanup is allowed only after the result has already been marked failed.
  // It is never used to turn an IRQ-only timeout into a success.
  capture.cleanupRequired = (discardRxFifo() > 0);
}
