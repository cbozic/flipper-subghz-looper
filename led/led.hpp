#pragma once
#include "easy_flipper/easy_flipper.h"

class SubGhzLooperApp;

class SubGhzLooperLed
{
private:
    void *appContext;                                // reference to the app context
    VariableItemList *variable_item_list = nullptr;   // variable item list for settings
    VariableItem *variable_item_enabled = nullptr;    // variable item for the LED-on-broadcast toggle
    ViewDispatcher **view_dispatcher_ref;              // reference to the view dispatcher

    static uint32_t callbackToSubmenu(void *context); // callback to switch to the main menu
    static void enabledChangedCallback(VariableItem *item);
    void updateEnabledText();

public:
    SubGhzLooperLed(ViewDispatcher **view_dispatcher, void *appContext);
    ~SubGhzLooperLed();
};
