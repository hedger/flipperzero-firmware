#include "subghz_frequency_analyzer.h"
#include "../subghz_i.h"

#include <math.h>
#include <furi.h>
#include <furi_hal.h>
#include <input/input.h>
#include <notification/notification_messages.h>
#include <lib/subghz/protocols/subghz_protocol_princeton.h>
#include "../helpers/subghz_frequency_analyzer_worker.h"

#include <assets_icons.h>

typedef enum {
    SubghzFrequencyAnalyzerStatusIDLE,
} SubghzFrequencyAnalyzerStatus;

struct SubghzFrequencyAnalyzer {
    View* view;
    SubGhzFrequencyAnalyzerWorker* worker;
    SubghzFrequencyAnalyzerCallback callback;
    void* context;
};

typedef struct {
    uint32_t frequency;
    float rssi;
} SubghzFrequencyAnalyzerModel;

void subghz_frequency_analyzer_set_callback(
    SubghzFrequencyAnalyzer* subghz_frequency_analyzer,
    SubghzFrequencyAnalyzerCallback callback,
    void* context) {
    furi_assert(subghz_frequency_analyzer);
    furi_assert(callback);
    subghz_frequency_analyzer->callback = callback;
    subghz_frequency_analyzer->context = context;
}

void subghz_frequency_analyzer_draw_rssi(Canvas* canvas, float rssi) {
    uint8_t x = 48;
    uint8_t y = 56;
    uint8_t column_number = 0;
    if(rssi) {
        rssi = (rssi + 90) / 3;
        for(size_t i = 1; i < (uint8_t)rssi; i++) {
            if(i > 20) break;
            if(i % 4) {
                column_number++;
                canvas_draw_box(canvas, x + 2 * i, y - column_number, 2, 4 + column_number);
            }
        }
    }
}

void subghz_frequency_analyzer_draw(Canvas* canvas, SubghzFrequencyAnalyzerModel* model) {
    char buffer[64];

    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 20, 8, "Frequency Analyzer");

    canvas_draw_str(canvas, 28, 60, "RSSI");
    subghz_frequency_analyzer_draw_rssi(canvas, model->rssi);

    //Frequency
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(
        buffer,
        sizeof(buffer),
        "%03ld.%03ld",
        model->frequency / 1000000 % 1000,
        model->frequency / 1000 % 1000);
    canvas_draw_str(canvas, 8, 35, buffer);
    canvas_draw_icon(canvas, 96, 24, &I_MHz_25x11);
}

bool subghz_frequency_analyzer_input(InputEvent* event, void* context) {
    furi_assert(context);
    //SubghzFrequencyAnalyzer* instance = context;

    if(event->key == InputKeyBack) {
        return false;
    }

    return true;
}

void subghz_frequency_analyzer_pair_callback(void* context, uint32_t frequency, float rssi) {
    SubghzFrequencyAnalyzer* instance = context;
    with_view_model(
        instance->view, (SubghzFrequencyAnalyzerModel * model) {
            model->rssi = rssi;
            model->frequency = frequency;
            return true;
        });
}

void subghz_frequency_analyzer_enter(void* context) {
    furi_assert(context);
    SubghzFrequencyAnalyzer* instance = context;

    //Start worker
    instance->worker = subghz_frequency_analyzer_worker_alloc();

    subghz_frequency_analyzer_worker_set_pair_callback(
        instance->worker,
        (SubGhzFrequencyAnalyzerWorkerPairCallback)subghz_frequency_analyzer_pair_callback,
        instance);

    subghz_frequency_analyzer_worker_start(instance->worker);

    with_view_model(
        instance->view, (SubghzFrequencyAnalyzerModel * model) {
            model->rssi = 0;
            model->frequency = 0;
            return true;
        });
}

void subghz_frequency_analyzer_exit(void* context) {
    furi_assert(context);
    SubghzFrequencyAnalyzer* instance = context;

    //Stop worker
    if(subghz_frequency_analyzer_worker_is_running(instance->worker)) {
        subghz_frequency_analyzer_worker_stop(instance->worker);
    }
    subghz_frequency_analyzer_worker_free(instance->worker);

    with_view_model(
        instance->view, (SubghzFrequencyAnalyzerModel * model) {
            model->rssi = 0;
            return true;
        });
}

SubghzFrequencyAnalyzer* subghz_frequency_analyzer_alloc() {
    SubghzFrequencyAnalyzer* instance = furi_alloc(sizeof(SubghzFrequencyAnalyzer));

    // View allocation and configuration
    instance->view = view_alloc();
    view_allocate_model(
        instance->view, ViewModelTypeLocking, sizeof(SubghzFrequencyAnalyzerModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, (ViewDrawCallback)subghz_frequency_analyzer_draw);
    view_set_input_callback(instance->view, subghz_frequency_analyzer_input);
    view_set_enter_callback(instance->view, subghz_frequency_analyzer_enter);
    view_set_exit_callback(instance->view, subghz_frequency_analyzer_exit);

    with_view_model(
        instance->view, (SubghzFrequencyAnalyzerModel * model) {
            model->rssi = 0;
            return true;
        });

    return instance;
}

void subghz_frequency_analyzer_free(SubghzFrequencyAnalyzer* instance) {
    furi_assert(instance);

    view_free(instance->view);
    free(instance);
}

View* subghz_frequency_analyzer_get_view(SubghzFrequencyAnalyzer* instance) {
    furi_assert(instance);
    return instance->view;
}