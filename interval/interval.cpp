#include "interval/interval.hpp"
#include "app.hpp"

static const char *const kUnitNames[3] = {"Seconds", "Minutes", "Hours"};

SubGhzLooperInterval::SubGhzLooperInterval(ViewDispatcher **view_dispatcher, void *appContext)
    : appContext(appContext), view_dispatcher_ref(view_dispatcher)
{
    if (!easy_flipper_set_variable_item_list(&variable_item_list, SubGhzLooperViewInterval,
                                             nullptr, callbackToSubmenu, view_dispatcher, this))
    {
        return;
    }

    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);

    variable_item_value = variable_item_list_add(variable_item_list, "Interval", 255, valueChangedCallback, this);
    variable_item_set_current_value_index(variable_item_value, static_cast<uint8_t>(app->intervalValue - 1));
    updateValueText();

    variable_item_unit = variable_item_list_add(variable_item_list, "Unit", 3, unitChangedCallback, this);
    variable_item_set_current_value_index(variable_item_unit, app->intervalUnit);
    updateUnitText();
}

SubGhzLooperInterval::~SubGhzLooperInterval()
{
    if (variable_item_list && view_dispatcher_ref && *view_dispatcher_ref)
    {
        view_dispatcher_remove_view(*view_dispatcher_ref, SubGhzLooperViewInterval);
        variable_item_list_free(variable_item_list);
        variable_item_list = nullptr;
        variable_item_value = nullptr;
        variable_item_unit = nullptr;
    }
}

uint32_t SubGhzLooperInterval::callbackToSubmenu(void *context)
{
    UNUSED(context);
    return SubGhzLooperViewSubmenu;
}

void SubGhzLooperInterval::updateValueText()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(app->intervalValue));
    variable_item_set_current_value_text(variable_item_value, buf);
}

void SubGhzLooperInterval::updateUnitText()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    variable_item_set_current_value_text(variable_item_unit, kUnitNames[app->intervalUnit]);
}

void SubGhzLooperInterval::valueChangedCallback(VariableItem *item)
{
    SubGhzLooperInterval *instance = static_cast<SubGhzLooperInterval *>(variable_item_get_context(item));
    uint8_t index = variable_item_get_current_value_index(item);
    // Update the app's in-memory setting only; it is persisted once on app exit.
    static_cast<SubGhzLooperApp *>(instance->appContext)->intervalValue = static_cast<int32_t>(index) + 1;
    instance->updateValueText();
}

void SubGhzLooperInterval::unitChangedCallback(VariableItem *item)
{
    SubGhzLooperInterval *instance = static_cast<SubGhzLooperInterval *>(variable_item_get_context(item));
    uint8_t index = variable_item_get_current_value_index(item);
    // Update the app's in-memory setting only; it is persisted once on app exit.
    static_cast<SubGhzLooperApp *>(instance->appContext)->intervalUnit = index;
    instance->updateUnitText();
}
