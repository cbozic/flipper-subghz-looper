#ifndef EASY_FLIPPER_H
#define EASY_FLIPPER_H

#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/modules/submenu.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/widget.h>
#include <gui/modules/variable_item_list.h>

#ifdef __cplusplus
#include <memory>
extern "C"
{
#endif

#define EASY_TAG "EasyFlipper"

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
        void *context);

    /**
     * @brief Initialize a ViewDispatcher object
     * @param view_dispatcher The ViewDispatcher object to initialize
     * @param gui The GUI object
     * @param context The context to pass to the event callback
     * @return true if successful, false otherwise
     */
    bool easy_flipper_set_view_dispatcher(ViewDispatcher **view_dispatcher, Gui *gui, void *context);

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
        ViewDispatcher **view_dispatcher);

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
        ViewDispatcher **view_dispatcher);

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
        void *context);

#ifdef __cplusplus
}
#endif

#endif
