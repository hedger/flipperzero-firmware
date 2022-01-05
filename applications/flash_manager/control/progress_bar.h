#pragma once

#include "../../lfrfid/view/elements/generic_element.h"

class TextStore;

class ProgressBarElement : public GenericElement {
public:
    ProgressBarElement();
    ~ProgressBarElement() final;
    void draw(Canvas* canvas) final;
    bool input(InputEvent* event) final;

    void setup(
        uint8_t x = 0,
        uint8_t y = 0,
        uint8_t w = 0,
        uint8_t h = 0,
        Font font = FontPrimary);
    void update_progress(uint8_t progress);

private:
    TextStore* progress_txt;
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t w = 0;
    uint8_t h = 0;
    uint8_t progress = 0;
    uint16_t pct_text_width = 0;
    Font font = FontPrimary;
};