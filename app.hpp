#pragma once
#include "font/font.h"
#include "easy_flipper/easy_flipper.h"
#include "subghz_tx/subghz_tx.h"
#include "run/run.hpp"
#include "interval/interval.hpp"
#include "led/led.hpp"
#include "files/files.hpp"
#include "about/about.hpp"

#define TAG "SubGhzLooper"
#define VERSION "1.0"
#define VERSION_TAG TAG " " VERSION
#define APP_ID "subghz_looper"

typedef enum
{
    SubGhzLooperSubmenuFiles = 0,
    SubGhzLooperSubmenuInterval = 1,
    SubGhzLooperSubmenuLed = 2,
    SubGhzLooperSubmenuRun = 3,
    SubGhzLooperSubmenuAbout = 4,
} SubGhzLooperSubmenuIndex;

typedef enum
{
    SubGhzLooperViewMain = 0,
    SubGhzLooperViewSubmenu = 1,
    SubGhzLooperViewAbout = 2,
    SubGhzLooperViewInterval = 3,
    SubGhzLooperViewLed = 4,
} SubGhzLooperView;

typedef enum
{
    SubGhzLooperCustomEventReturnToMenu = 0, // tear down the active custom screen and return to the submenu
} SubGhzLooperCustomEvent;

class SubGhzLooperApp
{
private:
    std::unique_ptr<SubGhzLooperAbout> about;       // About class instance
    std::unique_ptr<SubGhzLooperFiles> files;       // Files class instance (custom full-screen view)
    std::unique_ptr<SubGhzLooperInterval> interval; // Interval settings class instance
    std::unique_ptr<SubGhzLooperLed> led;           // LED-on-broadcast settings class instance
    std::unique_ptr<SubGhzLooperRun> run;           // Run class instance (custom full-screen view)
    Submenu *submenu = nullptr;                     // Submenu for the app
    SubGhzTx *subghzTx = nullptr;                   // Radio TX driver, allocated once and shared across Run sessions
    FuriTimer *timer = nullptr;                     // Timer for custom view redraws
    //
    static uint32_t callbackExitApp(void *context);                    // Callback to exit the app
    void callbackSubmenuChoices(uint32_t index);                       // Callback for submenu choices
    void createAppDataPath(const char *appId = APP_ID);                // Create the app data path in storage
    static bool customEventCallback(void *context, uint32_t event);    // Handle dispatcher custom events (runs on dispatcher thread)
    void startCustomScreen();                                          // Start the shared viewport+timer for Files/Run
    void stopCustomScreen();                                           // Tear down the viewport/timer/worker and return to submenu
    static void submenuChoicesCallback(void *context, uint32_t index); // Callback for submenu choices
    static void timerCallback(void *context);                          // Timer callback for custom view redraws

public:
    SubGhzLooperApp();
    ~SubGhzLooperApp();
    //
    Gui *gui = nullptr;                       // GUI instance for the app
    ViewDispatcher *viewDispatcher = nullptr; // ViewDispatcher for managing views
    ViewPort *viewPort = nullptr;             // ViewPort for drawing and input handling (Files/Run instances)
    //
    // Interval setting: single in-memory source of truth, loaded once at start and
    // persisted once at exit (avoids an SD write on every value change).
    int32_t intervalValue = 5; // 1..255
    uint8_t intervalUnit = 1;  // 0=Seconds, 1=Minutes, 2=Hours
    //
    // LED-on-broadcast setting: same in-memory/persist-once pattern as the interval setting.
    bool ledOnBroadcast = false; // off by default (preserves current no-LED behavior)
    //
    SubGhzTx *acquireSubGhzTx();                                                                                // lazily allocate (once) and return the shared radio TX driver
    bool loadChar(const char *path_name, char *value, size_t value_size, const char *appId = APP_ID);           // load a string from storage
    void runDispatcher();                                                                                       // run the app's view dispatcher to handle views and events
    bool saveChar(const char *path_name, const char *value, const char *appId = APP_ID, bool overwrite = true); // save a string to storage
    static void viewPortDraw(Canvas *canvas, void *context);                                                    // draw callback for the ViewPort (Files/Run instances)
    static void viewPortInput(InputEvent *event, void *context);                                                // input callback for the ViewPort (Files/Run instances)
};
