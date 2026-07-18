#pragma once
#include "easy_flipper/easy_flipper.h"

class SubGhzLooperApp;

class SubGhzLooperFiles
{
public:
    static constexpr size_t kMaxFiles = 64;
    static constexpr size_t kMaxNameLength = 128;

private:
    void *appContext;                  // reference to the app context
    bool shouldReturnToMenu = false;   // flag to signal return to menu

    char fileNames[kMaxFiles][kMaxNameLength]; // bare filenames found directly under /ext/subghz
    bool fileChecked[kMaxFiles] = {};
    size_t fileCount = 0;
    size_t focusIndex = 0;
    size_t scrollOffset = 0;

    void scanFiles();     // scan /ext/subghz for .sub files
    void loadSelection();  // mark previously-selected files as checked
    void saveSelection();  // persist the checked files

public:
    SubGhzLooperFiles(void *appContext);
    ~SubGhzLooperFiles();

    bool isActive() const { return !shouldReturnToMenu; } // Check if the screen is active
    void updateDraw(Canvas *canvas);                       // update and draw the screen
    void updateInput(InputEvent *event);                   // update input for the screen
};
