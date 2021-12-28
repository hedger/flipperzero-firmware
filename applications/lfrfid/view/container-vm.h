#pragma once
#include <view-modules/generic-view-module.h>
#include <list>

class GenericElement;

class ContainerVMData {
public:
    ContainerVMData(){};
    ~ContainerVMData();

    std::list<GenericElement*> elements;

    template <typename T> T add(const T element, View* view) {
        elements.push_back(element);
        element->set_parent_view(view);
        return element;
    }

    void clean();
};


struct ContainerVMModel {
    ContainerVMData* data;
};

class ContainerVM : public GenericViewModule {
public:
    ContainerVM();
    ~ContainerVM() final;
    View* get_view() final;
    void clean() final;

    template <typename T> T* add() {
        T* element = new T();

        with_view_model_cpp(view, ContainerVMModel, model, {
            model->data->add(element, view);
            return true;
        });

        return element;
    }

private:
    View* view;
    static void view_draw_callback(Canvas* canvas, void* model);
    static bool view_input_callback(InputEvent* event, void* context);
};