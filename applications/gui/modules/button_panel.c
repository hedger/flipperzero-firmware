#include "button_panel.h"
#include "furi_hal_resources.h"
#include "gui/canvas.h"
#include <m-array.h>
#include <m-i-list.h>
#include <m-list.h>
#include <furi.h>
#include <gui/elements.h>
#include <stdint.h>

typedef struct {
    // uint16_t to support multi-screen, wide button panel
    uint16_t x;
    uint16_t y;
    Font font;
    const char* str;
} LabelElement;

LIST_DEF(LabelList, LabelElement, M_POD_OPLIST)
#define M_OPL_LabelList_t() LIST_OPLIST(LabelList)

typedef struct {
    uint16_t x;
    uint16_t y;
    const Icon* name;
    const Icon* name_selected;
} IconElement;

typedef struct ButtonItem {
    uint32_t index;
    ButtonItemCallback callback;
    IconElement icon;
    void* callback_context;
} ButtonItem;

ARRAY_DEF(ButtonArray, ButtonItem*, M_PTR_OPLIST);
#define M_OPL_ButtonArray_t() ARRAY_OPLIST(ButtonArray, M_PTR_OPLIST)
ARRAY_DEF(ButtonMatrix, ButtonArray_t);
#define M_OPL_ButtonMatrix_t() ARRAY_OPLIST(ButtonMatrix, M_OPL_ButtonArray_t())

struct ButtonPanel {
    View* view;
    ButtonPanelInputCallback input_callback;
    void* input_context;
};

typedef struct {
    ButtonMatrix_t button_matrix;
    LabelList_t labels;
    uint16_t reserve_x;
    uint16_t reserve_y;
    uint16_t selected_item_x;
    uint16_t selected_item_y;
    ButtonPanelDrawCallback draw_callback;
    void* draw_context;
} ButtonPanelModel;

static ButtonItem** button_panel_get_item(ButtonPanelModel* model, size_t x, size_t y);
static void button_panel_process_up(ButtonPanel* button_panel);
static void button_panel_process_down(ButtonPanel* button_panel);
static void button_panel_process_left(ButtonPanel* button_panel);
static void button_panel_process_right(ButtonPanel* button_panel);
static void button_panel_process_ok(ButtonPanel* button_panel);
static void button_panel_view_draw_callback(Canvas* canvas, void* _model);
static bool button_panel_view_input_callback(InputEvent* event, void* context);

ButtonPanel* button_panel_alloc() {
    ButtonPanel* button_panel = furi_alloc(sizeof(ButtonPanel));
    button_panel->view = view_alloc();
    view_set_orientation(button_panel->view, ViewOrientationVertical);
    view_set_context(button_panel->view, button_panel);
    view_allocate_model(button_panel->view, ViewModelTypeLocking, sizeof(ButtonPanelModel));
    view_set_draw_callback(button_panel->view, button_panel_view_draw_callback);
    view_set_input_callback(button_panel->view, button_panel_view_input_callback);
    button_panel->input_callback = NULL;

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            model->reserve_x = 0;
            model->reserve_y = 0;
            model->selected_item_x = 0;
            model->selected_item_y = 0;
            model->draw_callback = NULL;
            ButtonMatrix_init(model->button_matrix);
            LabelList_init(model->labels);
            return true;
        });

    return button_panel;
}

void button_panel_reserve(ButtonPanel* button_panel, size_t reserve_x, size_t reserve_y) {
    furi_check(reserve_x > 0);
    furi_check(reserve_y > 0);

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            model->reserve_x = reserve_x;
            model->reserve_y = reserve_y;
            ButtonMatrix_reserve(model->button_matrix, model->reserve_y);
            for(size_t i = 0; i > model->reserve_y; ++i) {
                ButtonArray_t* array = ButtonMatrix_get(model->button_matrix, i);
                ButtonArray_init(*array);
                ButtonArray_reserve(*array, reserve_x);
                // TODO: do we need to clear allocated memory of ptr-s to ButtonItem ??
            }
            LabelList_init(model->labels);
            return true;
        });
}

void button_panel_free(ButtonPanel* button_panel) {
    furi_assert(button_panel);

    button_panel_clean(button_panel);

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            LabelList_clear(model->labels);
            ButtonMatrix_clear(model->button_matrix);
            return true;
        });

    view_free(button_panel->view);
    free(button_panel);
}

void button_panel_clean(ButtonPanel* button_panel) {
    furi_assert(button_panel);

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            for(size_t x = 0; x < model->reserve_x; ++x) {
                for(size_t y = 0; y < model->reserve_y; ++y) {
                    ButtonItem** button_item = button_panel_get_item(model, x, y);
                    free(*button_item);
                    *button_item = NULL;
                }
            }
            model->reserve_x = 0;
            model->reserve_y = 0;
            LabelList_reset(model->labels);
            ButtonMatrix_reset(model->button_matrix);
            return true;
        });
}

static ButtonItem** button_panel_get_item(ButtonPanelModel* model, size_t x, size_t y) {
    furi_assert(model);

    furi_check(x < model->reserve_x);
    furi_check(y < model->reserve_y);
    ButtonArray_t* button_array = ButtonMatrix_safe_get(model->button_matrix, x);
    ButtonItem** button_item = ButtonArray_safe_get(*button_array, y);
    return button_item;
}

void button_panel_add_item(
    ButtonPanel* button_panel,
    uint32_t index,
    uint16_t matrix_place_x,
    uint16_t matrix_place_y,
    uint16_t x,
    uint16_t y,
    const Icon* icon_name,
    const Icon* icon_name_selected,
    ButtonItemCallback callback,
    void* callback_context) {
    furi_assert(button_panel);

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            ButtonItem** button_item_ptr =
                button_panel_get_item(model, matrix_place_x, matrix_place_y);
            furi_check(*button_item_ptr == NULL);
            *button_item_ptr = furi_alloc(sizeof(ButtonItem));
            ButtonItem* button_item = *button_item_ptr;
            button_item->callback = callback;
            button_item->callback_context = callback_context;
            button_item->icon.x = x;
            button_item->icon.y = y;
            button_item->icon.name = icon_name;
            button_item->icon.name_selected = icon_name_selected;
            button_item->index = index;
            return true;
        });
}

View* button_panel_get_view(ButtonPanel* button_panel) {
    furi_assert(button_panel);
    return button_panel->view;
}

static void button_panel_view_draw_callback(Canvas* canvas, void* _model) {
    furi_assert(canvas);
    furi_assert(_model);

    ButtonPanelModel* model = _model;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    for(size_t x = 0; x < model->reserve_x; ++x) {
        for(size_t y = 0; y < model->reserve_y; ++y) {
            ButtonItem* button_item = *button_panel_get_item(model, x, y);
            const Icon* icon_name = button_item->icon.name;
            if((model->selected_item_x == x) && (model->selected_item_y == y)) {
                icon_name = button_item->icon.name_selected;
            }
            canvas_draw_icon(canvas, button_item->icon.x, button_item->icon.y, icon_name);
        }
    }

    for
        M_EACH(label, model->labels, LabelList_t) {
            canvas_set_font(canvas, label->font);
            canvas_draw_str(canvas, label->x, label->y, label->str);
        }

    if(model->draw_callback) model->draw_callback(canvas, model->draw_context);
}

static void button_panel_process_down(ButtonPanel* button_panel) {
    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            size_t new_selected_item_x = model->selected_item_x;
            size_t new_selected_item_y = model->selected_item_y;
            size_t i;

            if(new_selected_item_y >= (model->reserve_y - 1)) return false;

            ++new_selected_item_y;

            for(i = 0; i < model->reserve_x; ++i) {
                new_selected_item_x = (model->selected_item_x + i) % model->reserve_x;
                if(*button_panel_get_item(model, new_selected_item_x, new_selected_item_y)) {
                    break;
                }
            }
            if(i == model->reserve_x) return false;

            model->selected_item_x = new_selected_item_x;
            model->selected_item_y = new_selected_item_y;

            return true;
        });
}

static void button_panel_process_up(ButtonPanel* button_panel) {
    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            size_t new_selected_item_x = model->selected_item_x;
            size_t new_selected_item_y = model->selected_item_y;
            size_t i;

            if(new_selected_item_y <= 0) return false;

            --new_selected_item_y;

            for(i = 0; i < model->reserve_x; ++i) {
                new_selected_item_x = (model->selected_item_x + i) % model->reserve_x;
                if(*button_panel_get_item(model, new_selected_item_x, new_selected_item_y)) {
                    break;
                }
            }
            if(i == model->reserve_x) return false;

            model->selected_item_x = new_selected_item_x;
            model->selected_item_y = new_selected_item_y;
            return true;
        });
}

static void button_panel_process_left(ButtonPanel* button_panel) {
    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            size_t new_selected_item_x = model->selected_item_x;
            size_t new_selected_item_y = model->selected_item_y;
            size_t i;

            if(new_selected_item_x <= 0) return false;

            --new_selected_item_x;

            for(i = 0; i < model->reserve_y; ++i) {
                new_selected_item_y = (model->selected_item_y + i) % model->reserve_y;
                if(*button_panel_get_item(model, new_selected_item_x, new_selected_item_y)) {
                    break;
                }
            }
            if(i == model->reserve_y) return false;

            model->selected_item_x = new_selected_item_x;
            model->selected_item_y = new_selected_item_y;
            return true;
        });
}

static void button_panel_process_right(ButtonPanel* button_panel) {
    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            size_t new_selected_item_x = model->selected_item_x;
            size_t new_selected_item_y = model->selected_item_y;
            size_t i;

            if(new_selected_item_x >= (model->reserve_x - 1)) return false;

            ++new_selected_item_x;

            for(i = 0; i < model->reserve_y; ++i) {
                new_selected_item_y = (model->selected_item_y + i) % model->reserve_y;
                if(*button_panel_get_item(model, new_selected_item_x, new_selected_item_y)) {
                    break;
                }
            }
            if(i == model->reserve_y) return false;

            model->selected_item_x = new_selected_item_x;
            model->selected_item_y = new_selected_item_y;
            return true;
        });
}

void button_panel_process_ok(ButtonPanel* button_panel) {
    ButtonItem* button_item = NULL;

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            button_item =
                *button_panel_get_item(model, model->selected_item_x, model->selected_item_y);
            return true;
        });

    if(button_item && button_item->callback) {
        button_item->callback(button_item->callback_context, button_item->index);
    }
}

static bool button_panel_view_input_callback(InputEvent* event, void* context) {
    ButtonPanel* button_panel = context;
    furi_assert(button_panel);
    bool consumed = false;

    if(button_panel->input_callback) {
        consumed = button_panel->input_callback(event, button_panel->input_context);
    } else if(event->type == InputTypeShort) {
        switch(event->key) {
        case InputKeyUp:
            consumed = true;
            button_panel_process_up(button_panel);
            break;
        case InputKeyDown:
            consumed = true;
            button_panel_process_down(button_panel);
            break;
        case InputKeyLeft:
            consumed = true;
            button_panel_process_left(button_panel);
            break;
        case InputKeyRight:
            consumed = true;
            button_panel_process_right(button_panel);
            break;
        case InputKeyOk:
            consumed = true;
            button_panel_process_ok(button_panel);
            break;
        default:
            break;
        }
    }

    return consumed;
}

void button_panel_add_label(
    ButtonPanel* button_panel,
    uint16_t x,
    uint16_t y,
    Font font,
    const char* label_str) {
    furi_assert(button_panel);

    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            LabelElement* label = LabelList_push_raw(model->labels);
            label->x = x;
            label->y = y;
            label->font = font;
            label->str = label_str;
            return true;
        });
}

void button_panel_set_popup_draw_callback(
    ButtonPanel* button_panel,
    ButtonPanelDrawCallback callback,
    void* context) {
    with_view_model(
        button_panel->view, (ButtonPanelModel * model) {
            model->draw_callback = callback;
            model->draw_context = context;
            return true;
        });
}

void button_panel_set_popup_input_callback(
    ButtonPanel* button_panel,
    ButtonPanelInputCallback callback,
    void* context) {
    button_panel->input_context = context;
    button_panel->input_callback = callback;
}
