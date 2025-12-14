#pragma once
#include <stdint.h>

// ---- List of all UI pages ----
// Order matters: 0 MUST be BootLogo
enum PageID {
    PAGE_BOOTLOGO = 0,
    PAGE_ENV = 1,
    PAGE_PID1 = 2,
    PAGE_PID2 = 3,
    PAGE_PID3 = 4,
    PAGE_DTC = 5,
    PAGE_MAX
};

// Initialize the page system
void PageManager_begin();

// Called on every loop
void PageManager_update();

// Switch to a different page
void PageManager_switchTo(PageID id);

// Get currently active page
PageID PageManager_current();

// Register a page (called during begin)
void PageManager_registerPage(void (*beginFn)(), void (*updateFn)());
