#include "led/led.hpp"
#include "app.hpp"

static const char *const kEnabledNames[2] = {"Off", "On"};

SubGhzLooperLed::SubGhzLooperLed(ViewDispatcher **view_dispatcher, void *appContext)
    : appContext(appContext), view_dispatcher_ref(view_dispatcher)
{
    if (!easy_flipper_set_variable_item_list(&variable_item_list, SubGhzLooperViewLed,
                                             nullptr, callbackToSubmenu, view_dispatcher, this))
    {
        return;
    }

    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);

    variable_item_enabled = variable_item_list_add(variable_item_list, "LED on Broadcast", 2, enabledChangedCallback, this);
    variable_item_set_current_value_index(variable_item_enabled, app->ledOnBroadcast ? 1 : 0);
    updateEnabledText();
}

SubGhzLooperLed::~SubGhzLooperLed()
{
    if (variable_item_list && view_dispatcher_ref && *view_dispatcher_ref)
    {
        view_dispatcher_remove_view(*view_dispatcher_ref, SubGhzLooperViewLed);
        variable_item_list_free(variable_item_list);
        variable_item_list = nullptr;
        variable_item_enabled = nullptr;
    }
}

uint32_t SubGhzLooperLed::callbackToSubmenu(void *context)
{
    UNUSED(context);
    return SubGhzLooperViewSubmenu;
}

void SubGhzLooperLed::updateEnabledText()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    variable_item_set_current_value_text(variable_item_enabled, kEnabledNames[app->ledOnBroadcast ? 1 : 0]);
}

void SubGhzLooperLed::enabledChangedCallback(VariableItem *item)
{
    SubGhzLooperLed *instance = static_cast<SubGhzLooperLed *>(variable_item_get_context(item));
    uint8_t index = variable_item_get_current_value_index(item);
    // Update the app's in-memory setting only; it is persisted once on app exit.
    static_cast<SubGhzLooperApp *>(instance->appContext)->ledOnBroadcast = (index != 0);
    instance->updateEnabledText();
}
