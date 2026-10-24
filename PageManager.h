#pragma once
#include <stdint.h>

enum PageID {
    PAGE_BOOTLOGO = 0,
    PAGE_ENV = 1,
    PAGE_PID1 = 2,
    PAGE_PID2 = 3,
    PAGE_PID3 = 4,
    PAGE_DTC = 5,
    PAGE_MAX
};

void PageManager_begin();
void PageManager_update();
void PageManager_switchTo(PageID id);
PageID PageManager_current();

// Valid during the destination page's begin() call. True means PageManager
// already performed one chunked full-screen black clear for this transition.
bool PageManager_transitionClearedScreen();

void PageManager_registerPage(void (*beginFn)(), void (*updateFn)());
