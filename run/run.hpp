#pragma once
#include "easy_flipper/easy_flipper.h"

class SubGhzLooperApp;
struct SubGhzTx;

class SubGhzLooperRun
{
public:
    static constexpr size_t kMaxFiles = 64;
    static constexpr size_t kMaxPathLength = 160; // "/ext/subghz/" + up to a 128-char filename

private:
    void *appContext;                        // reference to the app context
    volatile bool shouldReturnToMenu = false; // flag to signal return to menu
    volatile bool ctlPause = false;           // worker pauses the loop while true; starts running so it broadcasts immediately on entering Run
    volatile bool ctlRequestExit = false;     // worker exits promptly while true
    volatile bool isTransmitting = false;     // true while a signal is actively being sent
    volatile uint32_t secondsRemaining = 0;   // countdown to the next transmission
    volatile bool cycleCompleted = false;     // set once a transmit cycle has finished (counts below are meaningful)
    volatile uint32_t lastCycleSent = 0;      // files transmitted OK in the most recent cycle
    volatile uint32_t lastCycleFailed = 0;    // files that failed to transmit in the most recent cycle

    FuriThread *thread = nullptr; // background worker thread
    SubGhzTx *tx = nullptr;       // radio TX driver instance (borrowed from the app, not owned)

    // LED-on-broadcast: cached copy of the app setting (read once at construction) and the
    // notification service handle used to blink the LED, only opened when the setting is on.
    bool ledOnBroadcast = false;
    NotificationApp *notifications = nullptr;

    // Battery reading is cached (GUI-thread only) so we don't hit the fuel gauge every redraw
    uint8_t batteryPct = 0;   // last-read battery percentage
    uint32_t batteryTick = 0; // furi tick of the last read (0 = never read yet)

    char filePaths[kMaxFiles][kMaxPathLength];
    size_t fileCount = 0;
    uint32_t intervalSeconds = 60;

    static int32_t workerThreadTrampoline(void *context);
    static bool shouldAbortCallback(void *context);
    void workerThreadMain();
    void loadSelectedFiles();
    void loadIntervalSeconds();
    void loadLedSetting();

public:
    SubGhzLooperRun(void *appContext);
    ~SubGhzLooperRun();

    bool isActive() const { return !shouldReturnToMenu; } // Check if the run is active
    void updateDraw(Canvas *canvas);                       // update and draw the run
    void updateInput(InputEvent *event);                   // update input for the run
};
