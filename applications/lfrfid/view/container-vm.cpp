#include "container-vm.h"
#include "elements/generic-element.h"
#include "elements/string-element.h"
#include "elements/icon-element.h"
#include "elements/button-element.h"
#include <list>

ContainerVMData::~ContainerVMData() {
    for(auto& it : elements) delete it;
};

void ContainerVMData::clean() {
    for(auto& it : elements) delete it;
    elements.clear();
}

ContainerVM::ContainerVM() {
    view = view_alloc();
    view_set_context(view, this);
    view_allocate_model(view, ViewModelTypeLocking, sizeof(ContainerVMModel));

    with_view_model_cpp(view, ContainerVMModel, model, {
        model->data = new ContainerVMData();
        return true;
    });

    view_set_draw_callback(view, view_draw_callback);
    view_set_input_callback(view, view_input_callback);
}

ContainerVM::~ContainerVM() {
    with_view_model_cpp(view, ContainerVMModel, model, {
        delete model->data;
        model->data = NULL;
        return false;
    });

    view_free(view);
}

View* ContainerVM::get_view() {
    return view;
}

void ContainerVM::clean() {
    with_view_model_cpp(view, ContainerVMModel, model, {
        model->data->clean();
        return true;
    });
}

void ContainerVM::view_draw_callback(Canvas* canvas, void* model) {
    ContainerVMData* data = static_cast<ContainerVMModel*>(model)->data;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);

    for(const auto& element : data->elements) {
        if(!element->is_enabled()) {
            continue;
        }

        element->draw(canvas);
    }
}

bool ContainerVM::view_input_callback(InputEvent* event, void* context) {
    bool consumed = false;
    View* view = static_cast<ContainerVM*>(context)->view;

    with_view_model_cpp(view, ContainerVMModel, model, {
        for(const auto& element : model->data->elements) {
            if(!element->is_enabled()) {
                continue;
            }

            if(element->input(event)) {
                consumed = true;
            }

            if(consumed) {
                break;
            }
        }

        return consumed;
    });

    return consumed;
}
