#include <easy_flipper/easy_flipper.h>

/**
 * @brief Initialize a View object
 * @param view The View object to initialize
 * @param view_id The ID/Index of the view
 * @param draw_callback The draw callback function (set to NULL if not needed)
 * @param input_callback The input callback function (set to NULL if not needed)
 * @param previous_callback The previous callback function (can be set to NULL)
 * @param view_dispatcher The ViewDispatcher object
 * @return true if successful, false otherwise
 */
bool easy_flipper_set_view(
    View **view,
    int32_t view_id,
    void draw_callback(Canvas *, void *),
    bool input_callback(InputEvent *, void *),
    uint32_t (*previous_callback)(void *),
    ViewDispatcher **view_dispatcher,
    void *context)
{
    if (!view || !view_dispatcher)
    {
        FURI_LOG_E(EASY_TAG, "Invalid arguments provided to set_view");
        return false;
    }
    *view = view_alloc();
    if (!*view)
    {
        FURI_LOG_E(EASY_TAG, "Failed to allocate View");
        return false;
    }
    if (draw_callback)
    {
        view_set_draw_callback(*view, draw_callback);
    }
    if (input_callback)
    {
        view_set_input_callback(*view, input_callback);
    }
    if (context)
    {
        view_set_context(*view, context);
    }
    if (previous_callback)
    {
        view_set_previous_callback(*view, previous_callback);
    }
    view_dispatcher_add_view(*view_dispatcher, view_id, *view);
    return true;
}

/**
 * @brief Initialize a ViewDispatcher object
 * @param view_dispatcher The ViewDispatcher object to initialize
 * @param gui The GUI object
 * @param context The context to pass to the event callback
 * @return true if successful, false otherwise
 */
bool easy_flipper_set_view_dispatcher(ViewDispatcher **view_dispatcher, Gui *gui, void *context)
{
    if (!view_dispatcher)
    {
        FURI_LOG_E(EASY_TAG, "Invalid arguments provided to set_view_dispatcher");
        return false;
    }
    *view_dispatcher = view_dispatcher_alloc();
    if (!*view_dispatcher)
    {
        FURI_LOG_E(EASY_TAG, "Failed to allocate ViewDispatcher");
        return false;
    }
    view_dispatcher_attach_to_gui(*view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    if (context)
    {
        view_dispatcher_set_event_callback_context(*view_dispatcher, context);
    }
    return true;
}

/**
 * @brief Initialize a Submenu object
 * @note This does not set the items in the submenu
 * @param submenu The Submenu object to initialize
 * @param view_id The ID/Index of the view
 * @param title The title/header of the submenu
 * @param previous_callback The previous callback function (can be set to NULL)
 * @param view_dispatcher The ViewDispatcher object
 * @return true if successful, false otherwise
 */
bool easy_flipper_set_submenu(
    Submenu **submenu,
    int32_t view_id,
    const char *title,
    uint32_t(previous_callback)(void *),
    ViewDispatcher **view_dispatcher)
{
    if (!submenu)
    {
        FURI_LOG_E(EASY_TAG, "Invalid arguments provided to set_submenu");
        return false;
    }
    *submenu = submenu_alloc();
    if (!*submenu)
    {
        FURI_LOG_E(EASY_TAG, "Failed to allocate Submenu");
        return false;
    }
    if (title)
    {
        submenu_set_header(*submenu, title);
    }
    if (previous_callback)
    {
        view_set_previous_callback(submenu_get_view(*submenu), previous_callback);
    }
    view_dispatcher_add_view(*view_dispatcher, view_id, submenu_get_view(*submenu));
    return true;
}

/**
 * @brief Initialize a Widget object
 * @param widget The Widget object to initialize
 * @param view_id The ID/Index of the view
 * @param text The text to display in the widget
 * @param previous_callback The previous callback function (can be set to NULL)
 * @param view_dispatcher The ViewDispatcher object
 * @return true if successful, false otherwise
 */
bool easy_flipper_set_widget(
    Widget **widget,
    int32_t view_id,
    const char *text,
    uint32_t(previous_callback)(void *),
    ViewDispatcher **view_dispatcher)
{
    if (!widget)
    {
        FURI_LOG_E(EASY_TAG, "Invalid arguments provided to set_widget");
        return false;
    }
    *widget = widget_alloc();
    if (!*widget)
    {
        FURI_LOG_E(EASY_TAG, "Failed to allocate Widget");
        return false;
    }
    if (text)
    {
        widget_add_text_scroll_element(*widget, 0, 0, 128, 64, text);
    }
    if (previous_callback)
    {
        view_set_previous_callback(widget_get_view(*widget), previous_callback);
    }
    view_dispatcher_add_view(*view_dispatcher, view_id, widget_get_view(*widget));
    return true;
}

/**
 * @brief Initialize a VariableItemList object
 * @note This does not set the items in the VariableItemList
 * @param variable_item_list The VariableItemList object to initialize
 * @param view_id The ID/Index of the view
 * @param enter_callback The enter callback function (can be set to NULL)
 * @param previous_callback The previous callback function (can be set to NULL)
 * @param view_dispatcher The ViewDispatcher object
 * @param context The context to pass to the enter callback (usually the app)
 * @return true if successful, false otherwise
 */
bool easy_flipper_set_variable_item_list(
    VariableItemList **variable_item_list,
    int32_t view_id,
    void (*enter_callback)(void *, uint32_t),
    uint32_t(previous_callback)(void *),
    ViewDispatcher **view_dispatcher,
    void *context)
{
    if (!variable_item_list)
    {
        FURI_LOG_E(EASY_TAG, "Invalid arguments provided to set_variable_item_list");
        return false;
    }
    *variable_item_list = variable_item_list_alloc();
    if (!*variable_item_list)
    {
        FURI_LOG_E(EASY_TAG, "Failed to allocate VariableItemList");
        return false;
    }
    if (enter_callback)
    {
        variable_item_list_set_enter_callback(*variable_item_list, enter_callback, context);
    }
    if (previous_callback)
    {
        view_set_previous_callback(variable_item_list_get_view(*variable_item_list), previous_callback);
    }
    view_dispatcher_add_view(*view_dispatcher, view_id, variable_item_list_get_view(*variable_item_list));
    return true;
}
