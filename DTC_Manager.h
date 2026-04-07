#pragma once

#include <stdint.h>
#include <stdbool.h>

enum DTCState {
    DTC_STATE_UNREAD = 0,
    DTC_STATE_READING,
    DTC_STATE_PRESENT,
    DTC_STATE_NONE,
    DTC_STATE_CLEARING,
    DTC_STATE_ERROR,
    DTC_STATE_REQUEST_ERROR
};

void DTCManager_begin();
void DTCManager_enterPage();
void DTCManager_update();
void DTCManager_requestClear();

DTCState DTCManager_getState();
int DTCManager_getCount();
const char* DTCManager_getCode(int index);
bool DTCManager_showClearButton();