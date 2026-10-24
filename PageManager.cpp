#include "PageManager.h"
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"
#include "PID_Manager.h"
#include "DTC_Manager.h"

#include "Page_BootLogo.h"
#include "Page_Env.h"
#include "Page_PID1.h"
#include "Page_PID2.h"
#include "Page_PID3.h"
#include "Page_DTC.h"

static PageID currentPage = PAGE_BOOTLOGO;
static PageID nextPage    = PAGE_BOOTLOGO;
static bool pageChangePending = false;

static void (*pageBegin[PAGE_MAX])()  = { nullptr };
static void (*pageUpdate[PAGE_MAX])() = { nullptr };

static uint32_t blockingInputUntil = 0;
static uint32_t lastPageChangeTime = 0;
constexpr uint32_t PAGE_SWITCH_GUARD_MS = 200;
constexpr uint32_t PAGE_CHANGE_DEBOUNCE_MS = 500;

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
    lastPageChangeTime = millis();
    if (pageBegin[currentPage]) pageBegin[currentPage]();
    Touch_reset();
}

void PageManager_update() {
    // The destination page's one-shot full redraw is deliberately deferred
    // until after the touch/SPI guard. This prevents live PID value-only
    // updates from appearing over stale ENV labels if the transition redraw
    // is disturbed by the touch transaction that requested it.
    if (pageChangePending && millis() < blockingInputUntil) {
        Touch_reset();
        return;
    }

    if (pageChangePending) {
        pageChangePending = false;

        if (currentPage == PAGE_DTC && nextPage != PAGE_DTC) {
            DTCManager_leavePage();
        }

        // Cancels any outstanding PID matcher/ownership when leaving a PID
        // page. PID page begin() calls this again with its own page, which is
        // harmless and preserves the existing clear-on-entry behavior.
        PIDManager_setPage(nextPage);

        currentPage = nextPage;
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
