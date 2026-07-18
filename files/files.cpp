#include "files/files.hpp"
#include "app.hpp"

#include <storage/storage.h>
#include <string.h>
#include <stdio.h>
#include <memory>

#define SUBGHZ_DIR "/ext/subghz"

namespace
{
    constexpr size_t kVisibleRows = 3;
    constexpr int kRowHeight = 13;
    constexpr int kListTop = 22;
    constexpr size_t kSelectionBufferSize = 4096;
}

SubGhzLooperFiles::SubGhzLooperFiles(void *appContext) : appContext(appContext)
{
    scanFiles();
    loadSelection();
}

SubGhzLooperFiles::~SubGhzLooperFiles()
{
    saveSelection();
}

void SubGhzLooperFiles::scanFiles()
{
    fileCount = 0;

    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *dir = storage_file_alloc(storage);

    if (storage_dir_open(dir, SUBGHZ_DIR))
    {
        FileInfo fileInfo;
        char name[kMaxNameLength];
        while (fileCount < kMaxFiles && storage_dir_read(dir, &fileInfo, name, sizeof(name)))
        {
            if (file_info_is_dir(&fileInfo))
            {
                continue;
            }
            size_t len = strlen(name);
            if (len > 4 && strcmp(name + len - 4, ".sub") == 0)
            {
                strncpy(fileNames[fileCount], name, kMaxNameLength - 1);
                fileNames[fileCount][kMaxNameLength - 1] = '\0';
                fileChecked[fileCount] = false;
                fileCount++;
            }
        }
    }

    storage_dir_close(dir);
    storage_file_free(dir);
    furi_record_close(RECORD_STORAGE);
}

void SubGhzLooperFiles::loadSelection()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    std::unique_ptr<char[]> buffer(new char[kSelectionBufferSize]);
    if (!app->loadChar("selected_files", buffer.get(), kSelectionBufferSize))
    {
        return;
    }

    char *cursor = buffer.get();
    while (cursor && *cursor)
    {
        char *newline = strchr(cursor, '\n');
        if (newline)
        {
            *newline = '\0';
        }
        if (*cursor)
        {
            for (size_t i = 0; i < fileCount; i++)
            {
                if (strcmp(fileNames[i], cursor) == 0)
                {
                    fileChecked[i] = true;
                    break;
                }
            }
        }
        cursor = newline ? newline + 1 : nullptr;
    }
}

void SubGhzLooperFiles::saveSelection()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    std::unique_ptr<char[]> buffer(new char[kSelectionBufferSize]);
    buffer[0] = '\0';
    size_t offset = 0;

    for (size_t i = 0; i < fileCount; i++)
    {
        if (!fileChecked[i])
        {
            continue;
        }
        size_t nameLen = strlen(fileNames[i]);
        if (offset + nameLen + 2 >= kSelectionBufferSize)
        {
            break;
        }
        if (offset > 0)
        {
            buffer[offset++] = '\n';
        }
        memcpy(buffer.get() + offset, fileNames[i], nameLen);
        offset += nameLen;
        buffer[offset] = '\0';
    }

    app->saveChar("selected_files", buffer.get());
}

void SubGhzLooperFiles::updateDraw(Canvas *canvas)
{
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Select Files");
    canvas_set_font(canvas, FontSecondary);

    if (fileCount == 0)
    {
        canvas_draw_str(canvas, 2, 30, "No .sub files found");
        canvas_draw_str(canvas, 2, 42, "in /ext/subghz");
        return;
    }

    char counter[16];
    snprintf(counter, sizeof(counter), "%u/%u", (unsigned)(focusIndex + 1), (unsigned)fileCount);
    canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignBottom, counter);

    for (size_t row = 0; row < kVisibleRows && (scrollOffset + row) < fileCount; row++)
    {
        size_t idx = scrollOffset + row;
        int y = kListTop + static_cast<int>(row) * kRowHeight;
        bool focused = (idx == focusIndex);

        if (focused)
        {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, y - 9, 128, kRowHeight);
            canvas_set_color(canvas, ColorWhite);
        }

        char line[96];
        snprintf(line, sizeof(line), "%s %s", fileChecked[idx] ? "[x]" : "[ ]", fileNames[idx]);
        canvas_draw_str(canvas, 4, y, line);

        if (focused)
        {
            canvas_set_color(canvas, ColorBlack);
        }
    }

    canvas_draw_str(canvas, 2, 61, "OK: toggle   Back: done");
}

void SubGhzLooperFiles::updateInput(InputEvent *event)
{
    if (event->key == InputKeyBack)
    {
        shouldReturnToMenu = true;
        return;
    }

    if (fileCount == 0)
    {
        return;
    }

    if (event->type != InputTypeShort && event->type != InputTypeRepeat)
    {
        return;
    }

    switch (event->key)
    {
    case InputKeyUp:
        if (focusIndex > 0)
        {
            focusIndex--;
            if (focusIndex < scrollOffset)
            {
                scrollOffset = focusIndex;
            }
        }
        break;
    case InputKeyDown:
        if (focusIndex + 1 < fileCount)
        {
            focusIndex++;
            if (focusIndex >= scrollOffset + kVisibleRows)
            {
                scrollOffset = focusIndex - kVisibleRows + 1;
            }
        }
        break;
    case InputKeyOk:
        if (event->type == InputTypeShort)
        {
            fileChecked[focusIndex] = !fileChecked[focusIndex];
        }
        break;
    default:
        break;
    }
}
