#include "about/about.hpp"

SubGhzLooperAbout::SubGhzLooperAbout(ViewDispatcher **viewDispatcher) : widget(nullptr), viewDispatcherRef(viewDispatcher)
{
    easy_flipper_set_widget(
        &widget, SubGhzLooperViewAbout,
        "Sub-GHz Looper\n---\nReplays saved Sub-GHz\ncaptures from /ext/subghz\non a repeating timer.\n\nSelect Files, set an\nInterval, then press Run.",
        callbackToSubmenu, viewDispatcherRef);
}

SubGhzLooperAbout::~SubGhzLooperAbout()
{
    if (widget && viewDispatcherRef && *viewDispatcherRef)
    {
        view_dispatcher_remove_view(*viewDispatcherRef, SubGhzLooperViewAbout);
        widget_free(widget);
        widget = nullptr;
    }
}

uint32_t SubGhzLooperAbout::callbackToSubmenu(void *context)
{
    UNUSED(context);
    return SubGhzLooperViewSubmenu;
}
