#include "PageManager.h"
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"
#include "PID_Manager.h"
#include "DTC_Manager.h"
#include "Overlay_Warnings.h"

#include "Page_BootLogo.h"
#include "Page_Env.h"
#include "Page_PID1.h"
#include "Page_PID2.h"
#include "Page_PID3.h"
#include "Page_DTC.h"

static PageID currentPage = PAGE_BOOTLOGO;
static PageID nextPage    = PAGE_BOOTLOGO;
static bool pageChangePending = false;
static bool transitionClearedScreen = false;

static void (*pageBegin[PAGE_MAX])()  = { nullptr };
static void (*pageUpdate[PAGE_MAX])() = { nullptr };

static uint32_t blockingInputUntil = 0;
static uint32_t lastPageChangeTime = 0;
constexpr uint32_t PAGE_SWITCH_GUARD_MS = 200;
constexpr uint32_t PAGE_CHANGE_DEBOUNCE_MS = 500;

static bool isGaugePage(PageID page) {
    return page == PAGE_ENV ||
           page == PAGE_PID1 ||
           page == PAGE_PID2 ||
           page == PAGE_PID3;
}

void PageManager_begin() {
    pageBegin[PAGE_BOOTLOGO]   = Page_BootLogo_begin;
    pageUpdate[PAGE_BOOTLOGO]  = Page_BootLogo_update;

    pageBegin[PAGE_ENV]        = Page_Env_begin;
    pageUpdate[PAGE_ENV]       = Page_Env_update;

    pageBegin[PAGE_PID1]       = Page_PID1_begin;
    pageUpdate[PAGE_PID1]      = Page_PID1_update;

    pageBegin[PAGE_PID2]       = Page_PID2_begin;
    pageUpdate[PAGE_PID2]      = Page_PID2_update;

    pageBegin[PAGE_PID3]       = Page_PID3_begin;
    pageUpdate[PAGE_PID3]      = Page_PID3_update;

    pageBegin[PAGE_DTC]        = Page_DTC_begin;
    pageUpdate[PAGE_DTC]       = Page_DTC_update;

    currentPage = PAGE_BOOTLOGO;
    nextPage = PAGE_BOOTLOGO;
    pageChangePending = false;
    transitionClearedScreen = false;
    lastPageChangeTime = millis();
    if (pageBegin[currentPage]) pageBegin[currentPage]();
    Touch_reset();
}

void PageManager_update() {
    // The destination page's one-shot redraw remains deferred until after the
    // touch/SPI guard. This is the ENV -> PID1 mixed-label/value fix.
    if (pageChangePending && millis() < blockingInputUntil) {
        Touch_reset();
        return;
    }

    if (pageChangePending) {
        pageChangePending = false;
        const PageID oldPage = currentPage;
        const PageID destination = nextPage;

        if (oldPage == PAGE_DTC && destination != PAGE_DTC) {
            DTCManager_leavePage();
        }

        // Cancel any old PID matcher/ownership before display work. PID page
        // begin() calls this again with the same destination, which is harmless.
        PIDManager_setPage(destination);

        // Gauge pages share the same four-quadrant/background layout, so a
        // gauge-to-gauge transition only clears each destination quadrant once.
        // Crossing to/from boot or DTC gets exactly one chunked full-screen clear.
        transitionClearedScreen =
            !(isGaugePage(oldPage) && isGaugePage(destination));

        if (transitionClearedScreen) {
            fillScreen_FAST(0x0000);
            Overlay_Warnings_screenCleared();
        }

        currentPage = destination;
        lastPageChangeTime = millis();

        if (pageBegin[currentPage]) {
            pageBegin[currentPage]();
        }
        Touch_reset();
    }

    if (millis() - lastPageChangeTime >= PAGE_CHANGE_DEBOUNCE_MS) {
        if (pageUpdate[currentPage]) {
            pageUpdate[currentPage]();
        }
    }
}

void PageManager_switchTo(PageID id) {
    if (id >= PAGE_MAX) return;
    nextPage = id;
    pageChangePending = true;
    blockingInputUntil = millis() + PAGE_SWITCH_GUARD_MS;
}

PageID PageManager_current() {
    return currentPage;
}

bool PageManager_transitionClearedScreen() {
    return transitionClearedScreen;
}
