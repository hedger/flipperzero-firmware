#include "../storage_settings.h"

#define BENCH_DATA_SIZE 4096 * 16
#define BENCH_COUNT 6
#define BENCH_REPEATS 4
#define BENCH_FILE "/ext/rwfiletest.bin"

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static uint16_t bench_size_default[BENCH_COUNT] = {1, 8, 32, 256, 512, 1024};
static uint16_t bench_size_large[BENCH_COUNT] = {512, 1024, 2048, 4096, 8192, 16384};

static bool storage_settings_scene_bench_write(
    Storage* api,
    uint16_t size,
    const uint8_t* data,
    uint32_t* speed) {
    File* file = storage_file_alloc(api);
    bool result = true;
    if(storage_file_open(file, BENCH_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        uint32_t ticks, bench_iterations;
        bench_iterations = max(min(BENCH_DATA_SIZE / size, 512), 16);
        ticks = osKernelGetTickCount();

        for(size_t repeat = 0; repeat < BENCH_REPEATS; repeat++) {
            for(size_t i = 0; i < bench_iterations; i++) {
                if(storage_file_write(file, (data + i * size), size) != size) {
                    result = false;
                    break;
                }
            }
        }

        ticks = osKernelGetTickCount() - ticks;
        *speed = bench_iterations * size * osKernelGetTickFreq() * BENCH_REPEATS;
        *speed /= ticks;
        *speed /= 1024;
    }
    storage_file_close(file);
    storage_file_free(file);
    return result;
}

static bool
    storage_settings_scene_bench_read(Storage* api, uint16_t size, uint8_t* data, uint32_t* speed) {
    File* file = storage_file_alloc(api);
    bool result = true;
    *speed = -1;

    if(storage_file_open(file, BENCH_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint32_t ticks, bench_iterations;
        bench_iterations = max(min(BENCH_DATA_SIZE / size, 512), 16);
        ticks = osKernelGetTickCount();

        for(size_t repeat = 0; repeat < BENCH_REPEATS; repeat++) {
            for(size_t i = 0; i < bench_iterations; i++) {
                if(storage_file_read(file, (data + i * size), size) != size) {
                    result = false;
                    break;
                }
            }
        }

        ticks = osKernelGetTickCount() - ticks;
        *speed = bench_iterations * size * osKernelGetTickFreq() * BENCH_REPEATS;
        *speed /= ticks;
        *speed /= 1024;
    }
    storage_file_close(file);
    storage_file_free(file);
    return result;
};

static bool running = false;

static void storage_settings_scene_benchmark(StorageSettings* app, uint16_t* bench_size) {
    if(running) return;
    running = true;

    string_reset(app->text_string);
    DialogEx* dialog_ex = app->dialog_ex;
    uint8_t* bench_data;
    dialog_ex_set_header(dialog_ex, "Preparing data...", 64, 32, AlignCenter, AlignCenter);

    bench_data = malloc(BENCH_DATA_SIZE);
    for(size_t i = 0; i < BENCH_DATA_SIZE; i++) {
        bench_data[i] = (uint8_t)i;
    }

    uint32_t bench_w_speed[BENCH_COUNT] = {0, 0, 0, 0, 0, 0};
    uint32_t bench_r_speed[BENCH_COUNT] = {0, 0, 0, 0, 0, 0};

    dialog_ex_set_header(dialog_ex, "Benchmarking...", 64, 32, AlignCenter, AlignCenter);
    for(size_t i = 0; i < BENCH_COUNT; i++) {
        if(!storage_settings_scene_bench_write(
               app->fs_api, bench_size[i], bench_data, &bench_w_speed[i]))
            break;

        if(i > 0) string_cat_printf(app->text_string, "\n");
        string_cat_printf(app->text_string, "%ub : W %luK ", bench_size[i], bench_w_speed[i]);
        dialog_ex_set_header(dialog_ex, NULL, 0, 0, AlignCenter, AlignCenter);
        dialog_ex_set_text(
            dialog_ex, string_get_cstr(app->text_string), 0, 32, AlignLeft, AlignCenter);

        if(!storage_settings_scene_bench_read(
               app->fs_api, bench_size[i], bench_data, &bench_r_speed[i]))
            break;

        string_cat_printf(app->text_string, "R %luK", bench_r_speed[i]);
        dialog_ex_set_text(
            dialog_ex, string_get_cstr(app->text_string), 0, 32, AlignLeft, AlignCenter);
    }

    free(bench_data);
    running = false;
}

static void
    storage_settings_scene_benchmark_dialog_callback(DialogExResult result, void* context) {
    StorageSettings* app = context;

    view_dispatcher_send_custom_event(app->view_dispatcher, result);
    if(result == DialogExResultRight) {
        storage_settings_scene_benchmark(app, bench_size_large);
    }
};

void storage_settings_scene_benchmark_on_enter(void* context) {
    StorageSettings* app = context;
    DialogEx* dialog_ex = app->dialog_ex;

    dialog_ex_set_context(dialog_ex, app);
    dialog_ex_set_result_callback(dialog_ex, storage_settings_scene_benchmark_dialog_callback);
    view_dispatcher_switch_to_view(app->view_dispatcher, StorageSettingsViewDialogEx);

    if(storage_sd_status(app->fs_api) != FSE_OK) {
        dialog_ex_set_header(dialog_ex, "SD card not mounted", 64, 10, AlignCenter, AlignCenter);
        dialog_ex_set_text(
            dialog_ex,
            "If an SD card is inserted,\r\npull it out and reinsert it",
            64,
            32,
            AlignCenter,
            AlignCenter);
        dialog_ex_set_left_button_text(dialog_ex, "Back");
    } else {
        dialog_ex_set_right_button_text(dialog_ex, "...");
        storage_settings_scene_benchmark(app, bench_size_default); //bench_size_large);
        notification_message(app->notification, &sequence_blink_green_100);
    }
}

bool storage_settings_scene_benchmark_on_event(void* context, SceneManagerEvent event) {
    StorageSettings* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DialogExResultLeft:
            consumed = scene_manager_previous_scene(app->scene_manager);
            break;
        }
    }
    return consumed;
}

void storage_settings_scene_benchmark_on_exit(void* context) {
    StorageSettings* app = context;
    DialogEx* dialog_ex = app->dialog_ex;

    dialog_ex_set_header(dialog_ex, NULL, 0, 0, AlignCenter, AlignCenter);
    dialog_ex_set_text(dialog_ex, NULL, 0, 0, AlignCenter, AlignTop);
    dialog_ex_set_icon(dialog_ex, 0, 0, NULL);
    dialog_ex_set_left_button_text(dialog_ex, NULL);
    dialog_ex_set_right_button_text(dialog_ex, NULL);
    dialog_ex_set_result_callback(dialog_ex, NULL);
    dialog_ex_set_context(dialog_ex, NULL);

    string_reset(app->text_string);
}