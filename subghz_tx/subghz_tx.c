#include "subghz_tx/subghz_tx.h"

#include <stdlib.h>
#include <string.h>

#include <furi.h>
#include <storage/storage.h>
#include <flipper_format/flipper_format_i.h>
#include <toolbox/stream/stream.h>

#include <lib/subghz/devices/devices.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>
#include <lib/subghz/devices/preset.h>
#include <lib/subghz/environment.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/subghz_protocol_registry.h>
#include <lib/subghz/protocols/raw.h>

#define TAG "SubGhzTx"

struct SubGhzTx
{
    const SubGhzDevice *device;
    SubGhzEnvironment *environment;
    SubGhzSetting *setting;
};

SubGhzTx *subghz_tx_alloc(void)
{
    SubGhzTx *instance = malloc(sizeof(SubGhzTx));

    subghz_devices_init();
    instance->device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
    subghz_devices_begin(instance->device);

    instance->environment = subghz_environment_alloc();
    subghz_environment_set_protocol_registry(instance->environment, (void *)&subghz_protocol_registry);

    instance->setting = subghz_setting_alloc();
    subghz_setting_load(instance->setting, EXT_PATH("subghz/assets/setting_user"));

    return instance;
}

void subghz_tx_free(SubGhzTx *instance)
{
    if (!instance)
    {
        return;
    }
    subghz_devices_end(instance->device);
    subghz_devices_deinit();
    subghz_environment_free(instance->environment);
    subghz_setting_free(instance->setting);
    free(instance);
}

// Maps a .sub file's long-form "Preset:" string to the short name used by the
// firmware's preset settings table (e.g. "FuriHalSubGhzPresetOok650Async" -> "AM650").
static const char *subghz_tx_short_preset_name(const char *preset)
{
    if (!strcmp(preset, "FuriHalSubGhzPresetOok270Async"))
        return "AM270";
    if (!strcmp(preset, "FuriHalSubGhzPresetOok650Async"))
        return "AM650";
    if (!strcmp(preset, "FuriHalSubGhzPreset2FSKDev238Async"))
        return "FM238";
    if (!strcmp(preset, "FuriHalSubGhzPreset2FSKDev12KAsync"))
        return "FM12K";
    if (!strcmp(preset, "FuriHalSubGhzPreset2FSKDev476Async"))
        return "FM476";
    return "";
}

bool subghz_tx_play_file(
    SubGhzTx *instance,
    const char *file_path,
    SubGhzTxShouldAbort should_abort,
    void *should_abort_context)
{
    if (!instance || !file_path)
    {
        return false;
    }

    Storage *storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat *fff_file = flipper_format_file_alloc(storage);
    FlipperFormat *fff_data = flipper_format_string_alloc();
    FuriString *temp_str = furi_string_alloc();
    FuriString *protocol = furi_string_alloc();
    SubGhzTransmitter *transmitter = NULL;
    bool tx_started = false;
    bool ok = false;

    do
    {
        if (!flipper_format_file_open_existing(fff_file, file_path))
        {
            FURI_LOG_E(TAG, "Failed to open %s", file_path);
            break;
        }

        uint32_t version = 0;
        if (!flipper_format_read_header(fff_file, temp_str, &version))
        {
            FURI_LOG_E(TAG, "Missing header in %s", file_path);
            break;
        }

        uint32_t frequency = 0;
        if (!flipper_format_read_uint32(fff_file, "Frequency", &frequency, 1))
        {
            FURI_LOG_E(TAG, "Missing Frequency in %s", file_path);
            break;
        }
        if (!subghz_devices_is_frequency_valid(instance->device, frequency))
        {
            FURI_LOG_E(TAG, "Invalid frequency %lu in %s", (unsigned long)frequency, file_path);
            break;
        }

        if (!flipper_format_read_string(fff_file, "Preset", temp_str))
        {
            FURI_LOG_E(TAG, "Missing Preset in %s", file_path);
            break;
        }
        const char *preset_name = subghz_tx_short_preset_name(furi_string_get_cstr(temp_str));
        if (preset_name[0] == '\0')
        {
            FURI_LOG_E(TAG, "Unsupported preset in %s", file_path);
            break;
        }
        uint8_t *preset_data = subghz_setting_get_preset_data_by_name(instance->setting, preset_name);
        if (!preset_data)
        {
            FURI_LOG_E(TAG, "Unknown preset data for %s", preset_name);
            break;
        }

        if (!flipper_format_read_string(fff_file, "Protocol", protocol))
        {
            FURI_LOG_E(TAG, "Missing Protocol in %s", file_path);
            break;
        }

        if (!strcmp(furi_string_get_cstr(protocol), "RAW"))
        {
            subghz_protocol_raw_gen_fff_data(fff_data, file_path, subghz_devices_get_name(instance->device));
        }
        else
        {
            stream_copy_full(flipper_format_get_raw_stream(fff_file), flipper_format_get_raw_stream(fff_data));
        }

        // fff_file is no longer needed once the data is in fff_data / regenerated for RAW;
        // release it before the (potentially multi-second) transmission.
        flipper_format_free(fff_file);
        fff_file = NULL;

        transmitter = subghz_transmitter_alloc_init(instance->environment, furi_string_get_cstr(protocol));
        if (!transmitter)
        {
            FURI_LOG_E(TAG, "Unknown protocol %s", furi_string_get_cstr(protocol));
            break;
        }
        if (subghz_transmitter_deserialize(transmitter, fff_data) != SubGhzProtocolStatusOk)
        {
            FURI_LOG_E(TAG, "Failed to deserialize %s", file_path);
            break;
        }

        subghz_devices_reset(instance->device);
        subghz_devices_idle(instance->device);
        subghz_devices_load_preset(instance->device, FuriHalSubGhzPresetCustom, preset_data);
        subghz_devices_set_frequency(instance->device, frequency);

        if (!subghz_devices_set_tx(instance->device))
        {
            FURI_LOG_E(TAG, "Only RX supported at %lu", (unsigned long)frequency);
            break;
        }

        subghz_devices_start_async_tx(instance->device, subghz_transmitter_yield, transmitter);
        tx_started = true;
        while (!subghz_devices_is_async_complete_tx(instance->device))
        {
            if (should_abort && should_abort(should_abort_context))
            {
                break;
            }
            furi_delay_ms(20);
        }
        subghz_devices_stop_async_tx(instance->device);
        subghz_devices_idle(instance->device);

        ok = true;
    } while (false);

    if (transmitter)
    {
        if (tx_started)
        {
            subghz_transmitter_stop(transmitter);
        }
        subghz_transmitter_free(transmitter);
    }

    if (fff_file)
    {
        flipper_format_free(fff_file);
    }
    flipper_format_free(fff_data);
    furi_string_free(temp_str);
    furi_string_free(protocol);
    furi_record_close(RECORD_STORAGE);

    return ok;
}
