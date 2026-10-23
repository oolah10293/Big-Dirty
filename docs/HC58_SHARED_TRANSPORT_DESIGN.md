# Shared MC68HC58 Production Transport Design

Status: **design complete enough for implementation review**

Branch: `design/hc58-shared-transport`

This document defines the proposed production architecture for replacing Big Dirty's duplicated, delay-driven HC58 receive code with one shared, continuously serviced, interrupt-driven transport.

It is a design document. No live cluster firmware has been changed by this branch.

## Evidence basis

The design is based on the standalone hardware test preserved under:

- `tests/HC58_INT_Diagnostic/`
- `tests/HC58_INT_Diagnostic/RESULTS_2026-10-23.md`
- `tests/HC58_INT_Diagnostic/results/`

Verified on the actual Teensy 4.1, Big Dirty interface board, Fixed Until Broken HC58 module, and live J1850 bus:

- configuration `0x12` produced 20/20 valid MAP replies and zero INT edges
- configuration `0x02` produced frame-completion interrupts
- IRQ-only reception produced 20/20 valid MAP replies with zero timeouts and no fallback polling
- a separate 100-request IRQ-only run produced 100/100 valid MAP replies, 227 interrupt edges, zero timeouts, zero IRQ-log overflows, zero frame-list overflows, and zero fallback cleanup
- unrelated J1850 frames appeared both before and between the request echo and expected PCM reply

Those results prove the hardware path and default interrupt mode. They also establish the production requirements below.

## Existing production architecture

`PID_Manager.cpp` and `DTC_Manager.cpp` currently duplicate ownership of:

- HC58 pins
- SPI1 configuration
- HC58 reset/configuration
- the two-byte SPI exchange
- transmit-frame loading
- RxFIFO draining

The live PID path currently:

1. sends one Mode 01 request
2. waits 9 ms plus 750 microseconds
3. repeatedly polls RxFIFO
4. declares capture complete after a 15 ms quiet gap or an 80 ms maximum window
5. searches a combined byte buffer for the expected response

The DTC path currently uses separate fixed waits and long drain windows, including 800 ms read windows and an approximately five-second Mode 04 clear/listen loop.

These paths work, but they infer receive timing rather than using the HC58's frame-completion signal and completion-code boundaries.

## Goals

The shared transport must:

1. be the only owner of the HC58 hardware and SPI1
2. use the proven `0x02` configuration with default interrupts enabled and IMOD left zero
3. keep the interrupt service routine minimal
4. service RxFIFO continuously, including when no PID or DTC request is active
5. construct frame boundaries from RFS status and completion codes
6. deliver complete frames in normal application context, never from the ISR
7. preserve unrelated vehicle traffic without confusing it for a requested reply
8. allow one controlled transmitter/client owner at a time
9. remove guessed post-transmit and quiet-gap receive timing from live PID reception
10. make PID and DTC operations non-blocking state machines
11. use fixed-size storage only; no heap allocation
12. expose enough counters to prove that the production integration is not overrunning or dropping traffic

## Non-goals for the first production implementation

The first implementation should not:

- change the supported PID list or decoding formulas
- change the DTC labels or user interface
- introduce IMOD=1 first-byte interrupts
- add a general-purpose J1850 traffic monitor
- reinterpret or remove the existing `0x10 / 0xC0` post-configuration exchange
- change the HC58 SPI speed, mode, CS setup, or CS hold timing
- combine unrelated display/UI cleanup with the transport work

## Proposed module boundary

### `HC58_Transport.h/.cpp`

This module owns:

- Teensy pins 0, 1, 24, 25, 26, and 27
- SPI1 initialization and all HC58 SPI exchanges
- reset and configuration
- the INT ISR and interrupt-generation counter
- TxFIFO loading
- RxFIFO servicing
- partial-frame assembly across service calls
- completion-code decoding
- frame delivery
- transport statistics and fault counters
- transmitter ownership arbitration between PID and DTC clients

No other source file may call `SPI1`, toggle HC58 CS/RST, or read/flush HC58 RxFIFO after migration.

### Protocol consumers

`PID_Manager` and `DTC_Manager` remain responsible for protocol meaning:

- request scheduling
- matching a response header/service/PID
- decoding PID values
- decoding and deduplicating DTCs
- request/response deadlines
- user-visible state

They receive complete frames from the transport and ignore frames that do not belong to their active request.

### Frame dispatch

The transport has one lightweight frame sink called from `HC58Transport_service()` in normal loop context.

The application-level sink calls:

```cpp
PIDManager_onHc58Frame(frame);
DTCManager_onHc58Frame(frame);
```

Each manager returns immediately when it has no active request. The sink must not draw, delay, print, or start another transmission. It only copies relevant bytes and advances manager state.

This immediate dispatch avoids maintaining a background frame queue that could fill while the cluster is on ENV, boot, or another non-requesting page.

## Hardware/configuration baseline

Initial production settings remain:

| Setting | Value |
|---|---:|
| SPI peripheral | SPI1 |
| SPI frequency | 100 kHz |
| Bit order | MSB first |
| SPI mode | Mode 3 |
| CS setup | 40 us |
| CS hold | 40 us |
| INT pin | Teensy 0, falling edge |
| Configuration byte | `0x02` |
| IMOD | 0, default sources only |

The first implementation should retain the exact tested initialization exchanges:

```text
02 / 1A   current immediate-configuration command with IMSK cleared
10 / C0   existing post-configuration exchange
```

The standalone test deliberately retained `0x1A`, so `0x12 -> 0x02` was the only configuration change. A future cleanup may evaluate the pure immediate-configuration command `0x18`, but it should not be mixed into the first production integration.

### Reset timing

The first implementation should use the sequence validated by the interrupt diagnostic and existing PID path:

```text
RST low  500 ms
RST high   5 ms
configure
settle      2 ms before normal use
```

The DTC manager's existing 40 ms/120 ms sequence is an alternate historical sequence. It should disappear when the transport becomes the sole owner rather than remain as a second reset path.

## Interrupt handling

The ISR does not access SPI or decode anything.

Proposed behavior:

```cpp
volatile uint32_t g_hc58IrqGeneration = 0;
volatile bool g_hc58IrqPending = false;

void hc58InterruptHandler() {
    g_hc58IrqGeneration++;
    g_hc58IrqPending = true;
}
```

Production may also retain a last-IRQ timestamp for diagnostics, but timestamps are not required for operation.

No `Serial`, `delay`, GPIO sequencing, SPI transaction, frame assembly, or manager callback belongs in the ISR.

## Continuous service contract

`HC58Transport_service()` must be called regardless of the active page.

Recommended top-level placement:

```cpp
void loop() {
    HC58Transport_service();

    PageManager_update();

    HC58Transport_service();

    Overlay_Warnings_update(...);
    backlightControl_update();
}
```

The second call protects the transport after a page update or redraw that occupied the CPU for a noticeable period.

The service function should run when either condition is true:

```cpp
g_hc58IrqPending || digitalRead(PIN_INT) == LOW
```

Checking the physical line as well as the software flag gives the main loop a recovery path if an edge flag is lost while INT remains asserted.

The function should continue until:

- INT is high
- no newer IRQ generation has appeared
- no complete frame is known to remain in RxFIFO

A bounded item budget, such as 64 RxFIFO items per call, prevents a malformed or continuously active bus from monopolizing the loop. Hitting the budget increments a counter and leaves service pending for the next loop pass.

## SPI exchange naming

The existing managers name the first argument `cmd` even though the first transferred byte is the transmit/configuration/data byte and the second is the HC58 command byte.

The shared module should use unambiguous names:

```cpp
exchange(
    uint8_t dataOut,
    uint8_t commandOut,
    uint8_t &statusIn,
    uint8_t &rxDataIn
);
```

This is a naming correction only; it does not change the tested byte order.

## RxFIFO service algorithm

Each service exchange uses:

```text
dataOut    = 0x00
commandOut = 0x02   RFC=10, read and flush one byte/completion code
```

The RFS field is `(status >> 5) & 0x07`.

| RFS | Meaning | Production action |
|---:|---|---|
| 000 | invalid/empty | stop this drain pass |
| 001 | more than one data byte, no completion | append data; continue |
| 010 | data plus a completion code later in FIFO | append data; continue until completion |
| 011 | 13th byte received, no completion | append/mark overlength as needed; continue draining available data |
| 100 | one data byte | append; read again once, then stop if empty |
| 101 | completion at head; later bytes but no later completion | finalize current frame; stop and wait for the next frame-completion interrupt |
| 110 | completion at head; another complete frame available | finalize current frame; continue immediately |
| 111 | completion only | finalize current frame; stop |

### Partial frames

The current assembly buffer persists across calls. This is required when:

- unrelated traffic begins before our request transmits
- a 13th-byte condition is serviced before frame completion
- a following frame is partially buffered behind a completion code reported as RFS=101

### Frame buffer size

Normal J1850 frames are limited to 12 data bytes excluding CRC. The design API reserves 20 bytes, matching the HC58 RxFIFO capacity, so abnormal or block-mode traffic can be observed without immediately corrupting memory.

If more than 20 bytes belong to one frame:

- continue flushing until its completion code
- retain the first 20 bytes
- mark the frame truncated
- increment a frame-overflow counter
- do not allow PID or DTC consumers to accept it as a valid reply

## Completion codes

Every delivered frame contains:

- frame bytes
- frame length
- raw completion code
- status byte observed with the completion code
- completion timestamp
- truncation flag

The transport decodes and counts:

- ERRF
- RFO
- TMS: not involved, underrun, lost arbitration, successful transmit
- IFR
- IFRC
- ERRC

A consumer must reject a reply when:

- RFO is set
- ERRF is set
- the frame is truncated
- the expected header/service/PID does not match

Background frames with errors are still counted, then discarded after delivery.

## Frame identity: never infer from IRQ order

The 100-request test observed:

- 73 requests with `OUR_REQUEST -> PCM_MAP_REPLY`
- 23 with `OUR_REQUEST -> BUS_FRAME -> PCM_MAP_REPLY`
- 4 with `BUS_FRAME -> OUR_REQUEST -> PCM_MAP_REPLY`

Therefore these assumptions are forbidden:

```text
first IRQ  = our request completed
second IRQ = PCM reply
```

Every frame is identified from its bytes and completion code.

Examples:

```text
Our Mode 01 request: 68 6A F1 01 PID
PCM Mode 01 reply:   48 6B 10 41 PID ...
Mode 03 positive:    48 6B 10 43 ...
Mode 07 positive:    48 6B 10 47 ...
Negative response:   48 6B 10 7F service reason
```

## Transmit ownership

Only one protocol client may own the transmitter at a time.

Proposed clients:

```cpp
HC58Client::None
HC58Client::PID
HC58Client::DTC
```

API behavior:

- `claim(client)` succeeds only when unclaimed or already held by that client
- `sendFrame(client, ...)` fails when the caller is not the owner
- `release(client)` succeeds only for the current owner
- owner conflicts increment a diagnostic counter

The UI page model already prevents intentional PID and DTC requests from running simultaneously, but explicit ownership protects against future regressions.

## Transmit flow

`sendFrame()` loads one complete normal frame with the existing commands:

```text
first byte        command 0x14
intermediate byte command 0x04
last byte         command 0x0C
```

It returns after loading TxFIFO. It does not wait for bus idle, transmission, or a response.

The manager tracks two separate deadlines:

1. **transmit deadline** — wait for an exact echo of the request with `TMS=TX_OK`
2. **response deadline** — begins after successful request transmission and waits for the expected PCM response

This matters because the live test captured unrelated bus traffic before our transmitted request on four of 100 requests. Starting the response deadline only after the successful request echo avoids treating arbitration delay as a PCM timeout.

When `TMS=LOST_ARB` is observed, the HC58's existing automatic retry behavior remains authoritative. The manager continues waiting for the eventual `TMS=TX_OK` echo unless its transmit deadline expires.

`TMS=UNDERRUN`, RFO, or a relevant ERRF condition fails the active request.

## PID manager design

The PID manager becomes non-blocking.

### State

```text
IDLE
WAIT_TX_ECHO
WAIT_RESPONSE
```

Per-request fields:

- requested PID
- destination display slot
- exact transmitted frame bytes
- transmit deadline
- response deadline
- request token/counter for diagnostics

### Update flow

When no request is active and the page's poll schedule is due:

1. claim the transport as PID
2. load `68 6A F1 01 PID`
3. enter `WAIT_TX_ECHO`

The frame callback:

- ignores unrelated frames
- recognizes the exact request echo and requires `TMS=TX_OK`
- then starts the response deadline and enters `WAIT_RESPONSE`
- recognizes `48 6B 10 41 PID`
- copies the response bytes, advances to IDLE, releases transport ownership, and lets existing decode functions update the slot

On transmit or response timeout:

- clear the destination slot as the current code does
- release ownership
- schedule the next normal poll

The existing PID page weighting and decoding formulas remain unchanged.

The old 9 ms post-TX wait, 750 us EOD wait, 15 ms quiet gap, and 80 ms capture window are removed from receive scheduling. A response timeout remains, but only as a failure deadline.

## DTC manager design

The DTC manager also becomes non-blocking. It claims the transport for the complete DTC operation so a PID request cannot be inserted between Mode 03, Mode 07, or Mode 04 steps.

### Combined read sequence

Proposed states:

```text
READ_HUSH_03
WAIT_TX_03
OBSERVE_03
READ_HUSH_07
WAIT_TX_07
OBSERVE_07
CLASSIFY
DONE
```

The initial implementation preserves the current timings as non-blocking deadlines:

- 250 ms pre-transmit hush
- up to 800 ms Mode 03 observation
- up to 800 ms Mode 07 observation

During each observation window, the DTC manager receives every completed frame from the transport and records:

- matching positive response
- matching negative response
- total nonempty RxFIFO items or equivalent transport traffic delta
- frame/completion errors

Positive frames are decoded immediately and merged into the existing unique-code list. Unrelated frames are counted and ignored.

### Existing traffic heuristics

The current no-code logic uses byte-count thresholds of 20 and 50. These are empirical, not protocol constants.

The first implementation should expose the transport's nonempty-item counter delta for each observation window so the old heuristic has the closest available equivalent. The exact thresholds must be validated on the vehicle before treating the DTC migration as complete.

Do not silently replace the existing no-code behavior with an assumed protocol response that has not been observed on this PCM.

### Mode 04 clear sequence

Proposed states:

```text
WAIT_TX_04
CLEAR_LISTEN
POST_CLEAR_DELAY
READ_HUSH_03
...
```

The current five-second listen period and 250 ms post-clear delay are preserved, but they become deadlines rather than blocking loops.

While those deadlines run:

- `HC58Transport_service()` continues every main-loop pass
- the screen can continue updating
- touch/page behavior can remain responsive or be deliberately locked by UI state rather than by a blocked CPU
- background frames cannot accumulate unserviced for five seconds

After the delay, the manager performs the same combined Mode 03 + Mode 07 reread and existing post-clear classification.

## Main application integration

### Setup

Proposed order:

```cpp
HC58Transport_begin(onHc58Frame);
PIDManager_begin();
DTCManager_begin();
PageManager_begin();
```

`PIDManager_begin()` and `DTCManager_begin()` no longer initialize SPI1 or reset the HC58.

The current main sketch should explicitly call `DTCManager_begin()`; today DTC initialization is deferred through its own manager path.

### Loop

The transport is serviced before and after the page manager.

Managers continue receiving their normal `update()` calls from their pages, but those updates only advance deadlines, send a request when allowed, and consume already-captured results. They do not spin in receive loops.

### Page changes

Leaving a PID page cancels any pending PID matcher and releases PID ownership. It does not reset the HC58 or flush unrelated traffic.

Entering DTC claims DTC ownership before beginning the read sequence. Leaving DTC cancels the sequence and releases ownership, unless a deliberate clear operation is defined as non-cancelable by the UI.

## Statistics and diagnostics

The transport should expose a snapshot containing at least:

- total IRQ edges
- total SPI exchanges
- total nonempty RxFIFO items
- completed frames
- frames delivered to the sink
- frames seen with ERRF
- RFO occurrences
- frame-byte overflow/truncation count
- TMS successful, lost-arbitration, and underrun counts
- service-budget hits
- INT-stuck-low recoveries
- owner conflicts
- hard resets

Per-client statistics should include:

- requests sent
- transmit timeouts
- response timeouts
- matched replies
- rejected/error replies
- cancellations

Serial diagnostics should be compile-time optional and disabled in normal production use.

## Failure handling

### RxFIFO overrun

When RFO is set:

- mark the current frame invalid
- increment the overrun counter
- continue to its completion boundary
- do not deliver it as a usable PID/DTC reply
- keep the transport alive; do not immediately reset unless overruns repeat

### INT remains low

If service reaches RFS=000 but INT remains low:

1. perform one additional status exchange
2. check for a newly arrived condition
3. if still inconsistent, increment a stuck-low counter and leave service pending
4. hard reset only after a bounded repeated-failure threshold

### Timeout

A request timeout cancels only the protocol request/matcher. It does not stop background service and does not automatically reset the HC58.

### NETF

A status byte with NETF set should be recorded separately. Recovery should follow a deliberate transport reset/abort policy, not be treated as an ordinary missing PCM response.

## CPU blocking audit

Interrupt-driven HC58 reception still depends on the main loop servicing the pending flag. SPI is intentionally not performed in the ISR.

Before production signoff, audit operations that can hold the CPU long enough to threaten the 20-byte RxFIFO:

- full-screen fills and page redraws
- SD boot-image drawing
- DHT20 calls
- any remaining `delay()` calls
- DTC operations

The DTC blocking loops are removed by this design. Page redraws remain rare; transport counters will show whether they cause RFO. If they do, add explicit service points around large drawing operations rather than moving SPI into the ISR.

## Implementation sequence

This is one feature effort on a dedicated branch; the sequence below is for reviewable commits, not separate user-facing packages.

1. Add `HC58_Transport.h/.cpp` with hardware ownership, interrupt service, frame assembly, and stats.
2. Add frame dispatch hooks to PID and DTC managers without deleting their protocol decoders.
3. Convert PID manager to the non-blocking request/echo/reply state machine.
4. Convert DTC read and clear to non-blocking state machines while preserving current classification behavior.
5. Remove duplicated HC58/SPI code and old delay/quiet-gap drain functions from both managers.
6. Add optional serial transport summaries.
7. Compile and perform live hardware regression testing.

No firmware ZIP or flashable package should be produced until an implementation revision has been reviewed and explicitly authorized for hardware testing.

## Production validation criteria

The implementation is not ready to merge until all of the following are verified on hardware:

### Live PID pages

- all three PID pages populate correctly
- values continue updating during extended operation
- no response is accepted for the wrong PID
- unrelated frames do not alter a displayed value
- no RFO, frame truncation, or service-budget counter increases during normal use

### Page/UI stress

- repeated page cycling does not cause HC58 overruns
- touch and display remain functional
- the ENV -> PID1 redraw fix remains intact
- boot and full-page redraws do not leave INT stuck low

### DTC

- known stored code is read through Mode 03
- pending-code behavior through Mode 07 remains correct
- duplicates are still suppressed
- no-code classification matches Save 14 behavior
- Mode 04 clear completes and the combined reread reports the correct state
- the five-second clear sequence no longer blocks transport service

### Long run

- extended live operation shows zero RFO
- zero frame buffer overflow
- zero unexplained owner conflicts
- no growing backlog or stuck-low INT condition
- request timeout behavior is visible and recoverable

## Design decisions currently locked

- one shared hardware owner
- configuration `0x02`
- IMOD remains zero
- ISR only records an event
- frame completion and RFS define receive boundaries
- continuous service on every page
- exact frame-content matching
- one transmitting client at a time
- no heap allocation
- managers become non-blocking

## Open implementation details

These require code review or hardware validation, not guesswork:

- exact PID transmit and response timeout values
- whether a direct frame callback or a one-frame handoff buffer is cleaner in the final C++ implementation
- calibrated DTC traffic thresholds under the new transport counter
- hard-reset threshold for repeated stuck-low or overrun conditions
- whether full-screen drawing ever requires an internal HC58 service hook
