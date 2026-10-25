# HC58 Shared Transport Live Test 2 — Hardware Results

Date: **2026-10-24**

Branch under test:

    test/hc58-shared-transport-live

Tested commit:

    8f6c261699c483f7b82a2824a6d3aca33fe8bd89

Build ID:

    BIGDIRTY_BUILD=HC58_INT_LIVE_TEST_2

## Purpose

Live Test 2 was created after Live Test 1 showed correct visible behavior but accumulated HC58 RxFIFO-overrun completion flags during long display activity.

The revision reduced redundant page-entry clearing and inserted `HC58Transport::service()` checkpoints into unavoidable display and SD drawing work.

## Hardware observations

The live vehicle test completed without an observed functional failure.

Observed behavior:

- normal boot
- full engine warm-up, long enough for the thermostat to open
- repeated page cycling
- ENV/PID pages remained visually correct
- TPS remained responsive
- other displayed PIDs appeared to read correctly
- engine start caused no observed problem
- ignition-off caused no observed problem
- the last portion of PID3 and PID2/PID3-to-page-4 activity in the serial log occurred with the key/ignition off

No mixed ENV/PID1 labels and values were observed.

## Transport result

The captured Live Test 2 USB diagnostic log contains 81 diagnostic samples.

Final counters:

    IRQ=16536
    FRAME=16545
    RFO=0
    ERR=0
    TRUNC=0
    BUDGET=0
    STUCK=0
    CONFLICT=0
    OWNER=1
    PAGE=4

Across the entire captured run, the maximum value of each transport-failure counter remained zero:

    RFO=0
    ERR=0
    TRUNC=0
    BUDGET=0
    STUCK=0
    CONFLICT=0

Pages 1 through 5 were all represented in the log.

## Comparison with Live Test 1

Live Test 1 recorded its first RxFIFO overun at 3,955 completed frames and ended with 36 overrun flags at 58,689 completed frames.

Live Test 2 reached 16,545 completed frames with `RFO=0`, more than four times the Live Test 1 first-overrun point.

This is strong evidence that the display-service and redundant-clear changes corrected the observed FIFO-starvation problem under the tested conditions.

## Ignition-off tail

Near the end of the log, completed-frame growth changed from roughly 400 frames per five-second sample to:

    +89
    +19
    +17
    +19
    +18

The user identified this portion as ignition-off.

The PID manager remains active while a PID page is displayed. With the PCM no longer replying, it can still transmit a request, receive its own HC58 transmit completion, wait for the response deadline, release ownership, and try again. `OWNER=1` in a five-second sample therefore means a PID request was in progress at the instant the sample was printed; it does not indicate stuck ownership.

`CONFLICT=0` throughout the run confirms that no competing client attempted to take the transmitter while another owner held it.

## DTC-page ownership sampling

`OWNER` is transmitter ownership, not the active page.

The DTC manager uses owner value `2` only while a Mode 03/07/04 operation is actively holding the HC58 transmitter. The diagnostic line is printed only every five seconds, so a normal DTC scan can claim and release owner `2` between samples. Seeing `PAGE=5 OWNER=0` therefore expected after the scan has completed.

This log does not by itself prove a particular stored/pending DTC result; it only shows that the DTC page was visited and that the shared transport remained clean while it was active.

## Result

**PASS — Live Test 2 eliminated the previously observed RxFIFO-overrun flags during this hardware run.**

The tested interrupt transport, page-transition fix, reduced redraw policy, and display-service checkpoints behaved correctly during the captured full-warm-up/page-cycling/ignition-off test.

No further functional transport change is indicated by this log.

## Preserved raw evidence

Original log:

    big dirty log.txt

Original byte length:

    7219

Original SHA-256:

    7462e6aea04800b2691f5815b9302ea9db0efdea8fe6bd57525b2e17e2795c80

Repository copy:

    tests/HC58_INT_Diagnostic/results/live_test_2_2026-10-24.txt.gz

Deterministic gzip SHA-256:

    e23c1d269915ee522c64332cbb605b2ba4928c23c57a9ccb5c86b8fbd375fca7

The compressed repository copy expands byte-for-byte to the original uploaded log.
