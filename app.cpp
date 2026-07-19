#include "app.hpp"
#include <storage/storage.h>
#include <stdlib.h>

SubGhzLooperApp::SubGhzLooperApp()
{
    gui = static_cast<Gui *>(furi_record_open(RECORD_GUI));

    // Allocate ViewDispatcher
    if (!easy_flipper_set_view_dispatcher(&viewDispatcher, gui, this))
    {
        FURI_LOG_E(TAG, "Failed to allocate view dispatcher");
        return;
    }

    // Custom events let worker/GUI threads request teardown on the dispatcher thread
    view_dispatcher_set_custom_event_callback(viewDispatcher, customEventCallback);

    // Submenu
    if (!easy_flipper_set_submenu(&submenu, SubGhzLooperViewSubmenu,
                                  VERSION_TAG, callbackExitApp, &viewDispatcher))
    {
        FURI_LOG_E(TAG, "Failed to allocate submenu");
        return;
    }

    submenu_add_item(submenu, "Select Files", SubGhzLooperSubmenuFiles, submenuChoicesCallback, this);
    submenu_add_item(submenu, "Interval", SubGhzLooperSubmenuInterval, submenuChoicesCallback, this);
    submenu_add_item(submenu, "Run", SubGhzLooperSubmenuRun, submenuChoicesCallback, this);
    submenu_add_item(submenu, "About", SubGhzLooperSubmenuAbout, submenuChoicesCallback, this);

    createAppDataPath();

    // Load persisted interval setting once into memory
    char buf[16];
    if (loadChar("interval_value", buf, sizeof(buf)))
    {
        int parsed = atoi(buf);
        if (parsed >= 1 && parsed <= 255)
        {
            intervalValue = parsed;
        }
    }
    if (loadChar("interval_unit", buf, sizeof(buf)))
    {
        int parsed = atoi(buf);
        if (parsed >= 0 && parsed <= 2)
        {
            intervalUnit = static_cast<uint8_t>(parsed);
        }
    }

    // Switch to the submenu view
    view_dispatcher_switch_to_view(viewDispatcher, SubGhzLooperViewSubmenu);
}

SubGhzLooperApp::~SubGhzLooperApp()
{
    // Persist the interval setting once on exit
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(intervalValue));
    saveChar("interval_value", buf);
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(intervalUnit));
    saveChar("interval_unit", buf);

    // Stop and free timer first
    if (timer)
    {
        furi_timer_stop(timer);
        furi_timer_free(timer);
        timer = nullptr;
    }

    // Clean up viewport if it exists
    if (gui && viewPort)
    {
        gui_remove_view_port(gui, viewPort);
        view_port_free(viewPort);
        viewPort = nullptr;
    }

    // Clean up run
    if (run)
    {
        run.reset();
    }

    // Clean up files
    if (files)
    {
        files.reset();
    }

    // Clean up interval
    if (interval)
    {
        interval.reset();
    }

    // Clean up about
    if (about)
    {
        about.reset();
    }

    // Free the shared radio TX driver (after run is gone so no worker is using it)
    if (subghzTx)
    {
        subghz_tx_free(subghzTx);
        subghzTx = nullptr;
    }

    // Free submenu
    if (submenu)
    {
        view_dispatcher_remove_view(viewDispatcher, SubGhzLooperViewSubmenu);
        submenu_free(submenu);
    }

    // Free view dispatcher
    if (viewDispatcher)
    {
        view_dispatcher_free(viewDispatcher);
    }

    // Close GUI
    if (gui)
    {
        furi_record_close(RECORD_GUI);
    }
}

uint32_t SubGhzLooperApp::callbackExitApp(void *context)
{
    UNUSED(context);
    return VIEW_NONE;
}

void SubGhzLooperApp::callbackSubmenuChoices(uint32_t index)
{
    switch (index)
    {
    case SubGhzLooperSubmenuFiles:
        files = std::make_unique<SubGhzLooperFiles>(this);
        startCustomScreen();
        break;
    case SubGhzLooperSubmenuInterval:
        if (!interval)
        {
            interval = std::make_unique<SubGhzLooperInterval>(&viewDispatcher, this);
        }
        view_dispatcher_switch_to_view(viewDispatcher, SubGhzLooperViewInterval);
        break;
    case SubGhzLooperSubmenuRun:
        run = std::make_unique<SubGhzLooperRun>(this);
        startCustomScreen();
        break;
    case SubGhzLooperSubmenuAbout:
        if (!about)
        {
            about = std::make_unique<SubGhzLooperAbout>(&viewDispatcher);
        }
        view_dispatcher_switch_to_view(viewDispatcher, SubGhzLooperViewAbout);
        break;
    default:
        break;
    }
}

void SubGhzLooperApp::createAppDataPath(const char *appId)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s", appId);
    storage_common_mkdir(storage, directory_path);
    snprintf(directory_path, sizeof(directory_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s/data", appId);
    storage_common_mkdir(storage, directory_path);
    furi_record_close(RECORD_STORAGE);
}

bool SubGhzLooperApp::loadChar(const char *path_name, char *value, size_t value_size, const char *appId)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *file = storage_file_alloc(storage);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s/data/%s.txt", appId, path_name);
    if (!storage_file_open(file, file_path, FSAM_READ, FSOM_OPEN_EXISTING))
    {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    size_t read_count = storage_file_read(file, value, value_size);
    // ensure we don't go out of bounds
    if (read_count > 0 && read_count < value_size)
    {
        value[read_count - 1] = '\0';
    }
    else if (read_count >= value_size && value_size > 0)
    {
        value[value_size - 1] = '\0';
    }
    else
    {
        value[0] = '\0';
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return strlen(value) > 0;
}

SubGhzTx *SubGhzLooperApp::acquireSubGhzTx()
{
    if (!subghzTx)
    {
        subghzTx = subghz_tx_alloc();
    }
    return subghzTx;
}

void SubGhzLooperApp::runDispatcher()
{
    view_dispatcher_run(viewDispatcher);
}

bool SubGhzLooperApp::saveChar(const char *path_name, const char *value, const char *appId, bool overwrite)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *file = storage_file_alloc(storage);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s/data/%s.txt", appId, path_name);
    if (!storage_file_open(file, file_path, FSAM_WRITE, overwrite ? FSOM_CREATE_ALWAYS : FSOM_OPEN_APPEND))
    {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    size_t append = overwrite ? 1 : 0; // add null terminator if overwriting
    size_t data_size = strlen(value) + append;
    if (storage_file_write(file, value, data_size) != data_size)
    {
        FURI_LOG_E(TAG, "Failed to write complete data to file: %s", file_path);
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return true;
}

void SubGhzLooperApp::startCustomScreen()
{
    // make sure any previous viewport/timer are torn down first
    if (timer)
    {
        furi_timer_stop(timer);
        furi_timer_free(timer);
        timer = nullptr;
    }
    if (gui && viewPort)
    {
        gui_remove_view_port(gui, viewPort);
        view_port_free(viewPort);
        viewPort = nullptr;
    }

    viewPort = view_port_alloc();
    view_port_draw_callback_set(viewPort, viewPortDraw, this);
    view_port_input_callback_set(viewPort, viewPortInput, this);
    gui_add_view_port(gui, viewPort, GuiLayerFullscreen);

    timer = furi_timer_alloc(timerCallback, FuriTimerTypePeriodic, this);
    if (timer)
    {
        // 1 Hz is enough: the only thing that changes on its own is the
        // second-granularity countdown (battery is cached, refreshed every 2s).
        // Keypresses repaint immediately via viewPortInput, so interactivity does
        // not depend on this rate. Redrawing faster only burns battery waking the
        // CPU and refreshing the LCD during the (potentially hours-long) idle wait.
        furi_timer_start(timer, 1000);
    }
}

void SubGhzLooperApp::stopCustomScreen()
{
    // Runs on the dispatcher thread (via customEventCallback), where stopping the
    // timer and joining the worker thread (in run.reset()) is safe.
    if (timer)
    {
        furi_timer_stop(timer);
        furi_timer_free(timer);
        timer = nullptr;
    }
    if (gui && viewPort)
    {
        gui_remove_view_port(gui, viewPort);
        view_port_free(viewPort);
        viewPort = nullptr;
    }

    view_dispatcher_switch_to_view(viewDispatcher, SubGhzLooperViewSubmenu);
    files.reset();
    run.reset();
}

bool SubGhzLooperApp::customEventCallback(void *context, uint32_t event)
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(context);
    furi_check(app);
    switch (event)
    {
    case SubGhzLooperCustomEventReturnToMenu:
        app->stopCustomScreen();
        return true;
    default:
        return false;
    }
}

void SubGhzLooperApp::submenuChoicesCallback(void *context, uint32_t index)
{
    SubGhzLooperApp *app = (SubGhzLooperApp *)context;
    app->callbackSubmenuChoices(index);
}

void SubGhzLooperApp::timerCallback(void *context)
{
    // Runs on the FuriTimer service thread: only ever request a redraw here.
    // All teardown happens on the dispatcher thread via SubGhzLooperCustomEventReturnToMenu.
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(context);
    furi_check(app);
    if (app->viewPort)
    {
        view_port_update(app->viewPort);
    }
}

void SubGhzLooperApp::viewPortDraw(Canvas *canvas, void *context)
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(context);
    furi_check(app);
    if (app->files && app->files->isActive())
    {
        app->files->updateDraw(canvas);
    }
    else if (app->run && app->run->isActive())
    {
        app->run->updateDraw(canvas);
    }
}

void SubGhzLooperApp::viewPortInput(InputEvent *event, void *context)
{
    if (event->type != InputTypeShort && event->type != InputTypeLong && event->type != InputTypeRepeat)
    {
        return;
    }
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(context);
    furi_check(app);
    if (app->files && app->files->isActive())
    {
        app->files->updateInput(event);
        if (!app->files->isActive())
        {
            view_dispatcher_send_custom_event(app->viewDispatcher, SubGhzLooperCustomEventReturnToMenu);
        }
        else if (app->viewPort)
        {
            // Repaint now so the keypress feels instant without a fast redraw timer
            view_port_update(app->viewPort);
        }
    }
    else if (app->run && app->run->isActive())
    {
        app->run->updateInput(event);
        if (!app->run->isActive())
        {
            view_dispatcher_send_custom_event(app->viewDispatcher, SubGhzLooperCustomEventReturnToMenu);
        }
        else if (app->viewPort)
        {
            view_port_update(app->viewPort);
        }
    }
}

extern "C"
{
    int32_t subghz_looper_main(void *p)
    {
        // Suppress unused parameter warning
        UNUSED(p);

        // Create the app
        SubGhzLooperApp app;

        // Run the app
        app.runDispatcher();

        // return success
        return 0;
    }
}
