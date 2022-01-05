#include "progress_bar.h"
#include <gui/elements.h>
#include "../../lfrfid/view/container_vm.h"
#include <app-scened-template/text_store.h>

ProgressBarElement::ProgressBarElement() {
    progress_txt = new TextStore(5);
}

ProgressBarElement::~ProgressBarElement() {
    delete progress_txt;
}

void ProgressBarElement::draw(Canvas* canvas) {
    if(pct_text_width == 0) {
        pct_text_width = canvas_string_width(canvas, "%") * 4;
    }

    elements_progress_bar(canvas, x, y, w - pct_text_width, progress, 100);
    elements_multiline_text_aligned(
        canvas, x + w - pct_text_width + 3, y + 1, AlignLeft, AlignTop, progress_txt->text);
}

bool ProgressBarElement::input(InputEvent* event) {
    return false;
}

void ProgressBarElement::setup(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Font font) {
    lock_model();
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->font = font;
    unlock_model(true);
}

void ProgressBarElement::update_progress(uint8_t progress) {
    if(progress > 100) {
        progress = 100;
    }

    lock_model();
    this->progress = progress;
    progress_txt->set("%d%%", progress);
    unlock_model(true);
}

template ProgressBarElement* ContainerVM::add<ProgressBarElement>();