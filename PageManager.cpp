#include "PageManager.h"
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"

// Pages
#include "Page_BootLogo.h"
#include "Page_Env.h"
#include "Page_PID1.h"
#include "Page_PID2.h"
#include "Page_PID3.h"
#include "Page_DTC.h"

// Current and queued page tracking
static PageID currentPage = PAGE_BOOTLOGO;
static PageID nextPage    = PAGE_BOOTLOGO;
static bool pageChangePending = false;

// Page handler table
static void (*pageBegin[PAGE_MAX])()  = { nullptr };
static void (*pageUpdate[PAGE_MAX])() = { nullptr };

// PAGE CHANGE DEBOUNCE — prevents page switch loops
static uint32_t blockingInputUntil = 0;
static uint32_t lastPageChangeTime = 0;
#define PAGE_CHANGE_DEBOUNCE_MS 500

void PageManager_begin()
{
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
    lastPageChangeTime = millis();
    if (pageBegin[currentPage]) pageBegin[currentPage]();
    Touch_reset();   // <<< ensures first page also starts with no "finger down"

}

void PageManager_update()
{
    // Ignore all touch input during the post-switch guard window
    if (millis() < blockingInputUntil) {
        Touch_reset();
        return;
    }

    if (pageChangePending) {
        pageChangePending = false;
        currentPage = nextPage;
        lastPageChangeTime = millis();
        if (pageBegin[currentPage]) {
            pageBegin[currentPage]();
        }
        Touch_reset();
        blockingInputUntil = millis() + 200;
    }

    if (millis() - lastPageChangeTime >= PAGE_CHANGE_DEBOUNCE_MS) {
        if (pageUpdate[currentPage]) {
            pageUpdate[currentPage]();
        }
    }
}

void PageManager_switchTo(PageID id)
{
    if (id >= PAGE_MAX) return;
    nextPage = id;
    pageChangePending = true;
}

PageID PageManager_current()
{
    return currentPage;
}
