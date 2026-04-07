#include "Page_BootLogo.h"
#include "Display_ST7796S.h"
#include "RawImage.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"

static uint32_t bootStartTime = 0;
static bool fadeActive = false;
static uint8_t fadeStep = 0;
static uint32_t lastFadeTime = 0;

#define BOOT_TIMEOUT_MS   3000       // show logo about 3 seconds before auto-exit
#define FADE_STEPS        32
#define FADE_INTERVAL_MS  25         // 32 × 25ms ≈ 800ms fade-out

void Page_BootLogo_begin()
{
    bootStartTime = millis();
    fadeActive = false;
    fadeStep = 0;
    lastFadeTime = 0;

    setBacklightPWM(0);              // keep screen dark first
    fillScreen_FAST(0x0000);         // black wipe first

    setBacklightPWM(255);            // now reveal the screen
    drawRAW480x320("/bowtie.raw");   // let it visibly draw
}

void Page_BootLogo_update()
{
    uint32_t now = millis();

    // Trigger fade from either touch or timeout
    TouchPoint tp = readTouch();
    if (!fadeActive && (tp.touched || (now - bootStartTime >= BOOT_TIMEOUT_MS)))
    {
        fadeActive = true;
        fadeStep = 0;
        lastFadeTime = now;
    }

    // Run fade non-blocking
    if (fadeActive && (now - lastFadeTime >= FADE_INTERVAL_MS))
    {
        lastFadeTime = now;

        fadeStep++;

        int brightness = 255 - ((255 * fadeStep) / FADE_STEPS);
        if (brightness < 0) brightness = 0;
        setBacklightPWM(brightness);

        if (fadeStep >= FADE_STEPS)
        {
            setBacklightPWM(0);      // stay dark until ENV draws
            fadeActive = false;
            PageManager_switchTo(PAGE_ENV);
            return;
        }
    }
}