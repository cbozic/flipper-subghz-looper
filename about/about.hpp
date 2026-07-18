#pragma once
#include "easy_flipper/easy_flipper.h"

class SubGhzLooperAbout
{
private:
    Widget *widget;
    ViewDispatcher **viewDispatcherRef;

    static constexpr const uint32_t SubGhzLooperViewSubmenu = 1; // View ID for submenu
    static constexpr const uint32_t SubGhzLooperViewAbout = 2;   // View ID for about

    static uint32_t callbackToSubmenu(void *context);

public:
    SubGhzLooperAbout(ViewDispatcher **viewDispatcher);
    ~SubGhzLooperAbout();
};
