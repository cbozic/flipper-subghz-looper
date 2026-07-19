#include "run/run.hpp"
#include "app.hpp"
#include "subghz_tx/subghz_tx.h"

#include <furi_hal_power.h>
#include <notification/notification_messages.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

namespace
{
    constexpr size_t kSelectionBufferSize = 4096;
}

SubGhzLooperRun::SubGhzLooperRun(void *appContext) : appContext(appContext)
{
    loadSelectedFiles();
    loadIntervalSeconds();
    loadLedSetting();

    // Borrow the app-owned radio driver (allocated once, shared across Run sessions)
    tx = static_cast<SubGhzLooperApp *>(appContext)->acquireSubGhzTx();

    if (ledOnBroadcast)
    {
        notifications = static_cast<NotificationApp *>(furi_record_open(RECORD_NOTIFICATION));
    }

    thread = furi_thread_alloc();
    furi_thread_set_name(thread, "SubGhzLooperWorker");
    furi_thread_set_stack_size(thread, 4096);
    furi_thread_set_context(thread, this);
    furi_thread_set_callback(thread, workerThreadTrampoline);
    furi_thread_start(thread);
}

SubGhzLooperRun::~SubGhzLooperRun()
{
    ctlRequestExit = true;
    if (thread)
    {
        furi_thread_join(thread);
        furi_thread_free(thread);
        thread = nullptr;
    }
    // tx is owned by the app; do not free it here.
    tx = nullptr;

    if (notifications)
    {
        furi_record_close(RECORD_NOTIFICATION);
        notifications = nullptr;
    }
}

int32_t SubGhzLooperRun::workerThreadTrampoline(void *context)
{
    SubGhzLooperRun *instance = static_cast<SubGhzLooperRun *>(context);
    instance->workerThreadMain();
    return 0;
}

bool SubGhzLooperRun::shouldAbortCallback(void *context)
{
    SubGhzLooperRun *instance = static_cast<SubGhzLooperRun *>(context);
    return instance->ctlRequestExit || instance->ctlPause;
}

void SubGhzLooperRun::workerThreadMain()
{
    while (!ctlRequestExit)
    {
        // Wait here while paused. Poll slowly: the screen is static while paused,
        // and a resume/exit is still noticed within a fraction of a second.
        while (ctlPause && !ctlRequestExit)
        {
            furi_delay_ms(200);
        }
        if (ctlRequestExit)
        {
            break;
        }

        // Transmit every selected file once
        if (fileCount > 0)
        {
            uint32_t sent = 0;
            uint32_t failed = 0;
            isTransmitting = true;
            if (notifications)
            {
                notification_message(notifications, &sequence_blink_start_red);
            }
            for (size_t i = 0; i < fileCount; i++)
            {
                if (ctlRequestExit || ctlPause)
                {
                    break;
                }
                if (subghz_tx_play_file(tx, filePaths[i], shouldAbortCallback, this))
                {
                    sent++;
                }
                else
                {
                    failed++;
                }
            }
            if (notifications)
            {
                notification_message(notifications, &sequence_blink_stop);
            }
            isTransmitting = false;

            // Only publish the tally for a cycle we actually finished (not one cut short by pause/exit)
            if (!ctlRequestExit && !ctlPause)
            {
                lastCycleSent = sent;
                lastCycleFailed = failed;
                cycleCompleted = true;
            }
        }

        if (ctlRequestExit)
        {
            break;
        }

        // Wait for the configured interval, checking for pause/exit as we go
        // Poll at 250ms: the countdown only displays whole seconds and a redraw
        // happens at 1 Hz, so a coarser poll keeps the counter accurate while
        // waking the CPU far less over a long (minutes/hours) wait. Pause/exit is
        // still noticed within 250ms.
        uint32_t intervalMs = intervalSeconds * 1000u;
        uint32_t waitedMs = 0;
        secondsRemaining = intervalSeconds;
        while (waitedMs < intervalMs && !ctlRequestExit && !ctlPause)
        {
            furi_delay_ms(250);
            waitedMs += 250;
            uint32_t remainingMs = (intervalMs > waitedMs) ? (intervalMs - waitedMs) : 0;
            secondsRemaining = (remainingMs + 999) / 1000;
        }
    }
}

void SubGhzLooperRun::loadSelectedFiles()
{
    fileCount = 0;

    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    std::unique_ptr<char[]> buffer(new char[kSelectionBufferSize]);
    if (!app->loadChar("selected_files", buffer.get(), kSelectionBufferSize))
    {
        return;
    }

    char *cursor = buffer.get();
    while (cursor && *cursor && fileCount < kMaxFiles)
    {
        char *newline = strchr(cursor, '\n');
        if (newline)
        {
            *newline = '\0';
        }
        if (*cursor)
        {
            snprintf(filePaths[fileCount], kMaxPathLength, "/ext/subghz/%s", cursor);
            fileCount++;
        }
        cursor = newline ? newline + 1 : nullptr;
    }
}

void SubGhzLooperRun::loadIntervalSeconds()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    int32_t value = app->intervalValue; // 1..255
    uint8_t unit = app->intervalUnit;   // 0=Seconds, 1=Minutes, 2=Hours

    uint32_t multiplier = 60; // Minutes
    if (unit == 0)
    {
        multiplier = 1; // Seconds
    }
    else if (unit == 2)
    {
        multiplier = 3600; // Hours
    }

    intervalSeconds = static_cast<uint32_t>(value) * multiplier;
    if (intervalSeconds == 0)
    {
        intervalSeconds = 1;
    }
}

void SubGhzLooperRun::loadLedSetting()
{
    SubGhzLooperApp *app = static_cast<SubGhzLooperApp *>(appContext);
    ledOnBroadcast = app->ledOnBroadcast;
}

void SubGhzLooperRun::updateDraw(Canvas *canvas)
{
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Sub-GHz Looper");
    canvas_set_font(canvas, FontSecondary);

    // Battery percentage, top-right (refreshed at most a couple times per second)
    uint32_t now = furi_get_tick();
    if (batteryTick == 0 || now - batteryTick >= 2000)
    {
        batteryPct = furi_hal_power_get_pct();
        batteryTick = now;
    }
    char battery[16];
    snprintf(battery, sizeof(battery), "%u%%", (unsigned)batteryPct);
    canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignBottom, battery);

    if (fileCount == 0)
    {
        canvas_draw_str(canvas, 2, 26, "No files selected.");
        canvas_draw_str(canvas, 2, 38, "Go back and choose");
        canvas_draw_str(canvas, 2, 50, "files first.");
        return;
    }

    char line[64];
    snprintf(line, sizeof(line), "%u file%s selected", (unsigned)fileCount, fileCount == 1 ? "" : "s");
    canvas_draw_str(canvas, 2, 22, line);

    bool paused = ctlPause;
    bool transmitting = isTransmitting;

    canvas_draw_str(canvas, 2, 33, paused ? "Status: Paused" : "Status: Running");

    if (!paused)
    {
        if (transmitting)
        {
            snprintf(line, sizeof(line), "Broadcasting...");
        }
        else
        {
            snprintf(line, sizeof(line), "Next in: %lus", (unsigned long)secondsRemaining);
        }
        canvas_draw_str(canvas, 2, 44, line);
    }

    // Show the outcome of the last completed cycle so silent TX failures are visible
    if (cycleCompleted)
    {
        if (lastCycleFailed > 0)
        {
            snprintf(line, sizeof(line), "Last: %lu sent, %lu FAILED",
                     (unsigned long)lastCycleSent, (unsigned long)lastCycleFailed);
        }
        else
        {
            snprintf(line, sizeof(line), "Last: %lu sent OK", (unsigned long)lastCycleSent);
        }
        canvas_draw_str(canvas, 2, 54, line);
    }

    // Activity indicator, bottom-right: filled while actively broadcasting, hollow otherwise
    if (transmitting)
    {
        canvas_draw_disc(canvas, 121, 59, 4);
    }
    else
    {
        canvas_draw_circle(canvas, 121, 59, 4);
    }

    canvas_draw_str(canvas, 2, 63, paused ? "OK: Play   Back: Exit" : "OK: Pause  Back: Exit");
}

void SubGhzLooperRun::updateInput(InputEvent *event)
{
    if (event->key == InputKeyBack)
    {
        shouldReturnToMenu = true;
        return;
    }
    if (event->key == InputKeyOk && event->type == InputTypeShort)
    {
        ctlPause = !ctlPause;
    }
}
