#pragma once
#include "easy_flipper/easy_flipper.h"

class SubGhzLooperApp;

class SubGhzLooperInterval
{
private:
    void *appContext;                                // reference to the app context
    VariableItemList *variable_item_list = nullptr;   // variable item list for settings
    VariableItem *variable_item_value = nullptr;      // variable item for the interval value
    VariableItem *variable_item_unit = nullptr;       // variable item for the interval unit
    ViewDispatcher **view_dispatcher_ref;             // reference to the view dispatcher

    static uint32_t callbackToSubmenu(void *context); // callback to switch to the main menu
    static void valueChangedCallback(VariableItem *item);
    static void unitChangedCallback(VariableItem *item);
    void updateValueText();
    void updateUnitText();

public:
    SubGhzLooperInterval(ViewDispatcher **view_dispatcher, void *appContext);
    ~SubGhzLooperInterval();
};
