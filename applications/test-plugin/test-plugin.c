#include <furi.h>
#include <furi-hal.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <notification/notification-messages.h>
#include <assets_icons.h>

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} TestPluginEvent;

typedef struct {
    int num_x;
    int num_y;
} TestPluginNum;


static void test_plugin_draw_callback(Canvas* canvas, void* ctx) {
    TestPluginNum* plugin = ctx;

    // elements_button_left(canvas, "Hello");
    // if (plugin->num_y == 0) {
    //     canvas_set_font(canvas, FontPrimary);
    //     elements_multiline_text_aligned(canvas, 2, 2, AlignLeft, AlignTop, "Product: Flipper Zero\nModel: FZ.1");
    //     canvas_set_font(canvas, FontSecondary);
    //     elements_multiline_text_aligned(canvas, 2, 30, AlignLeft, AlignTop, "FCC ID: 2A2V6-FZ\nIC ID: 27624-FZ");
    // } else if (plugin->num_y == 1) {
    //     canvas_set_font(canvas, FontSecondary);
    //     elements_multiline_text_aligned(canvas, 2, 2, AlignLeft, AlignTop, "Flipper Devices Inc,\nSuite B #551,\n2803 Philadelphia Pike,\nClaymont, DE, USA 19703");
    //     //For all compliance certificates\nplease visit\nwww.flipp.dev/compliance
    // } else if (plugin->num_y == 2) {
    //     canvas_draw_icon(canvas, plugin->num_x, plugin->num_y, &I_Test_Labels_1_128x64);
    // } else if (plugin->num_y == 3) {
    //     canvas_set_font(canvas, FontSecondary);
    //     elements_multiline_text_aligned(canvas, 2, 2, AlignLeft, AlignTop, "For all compliance\ncertificatesplease visit");
    //     canvas_set_font(canvas, FontPrimary);
    //     canvas_draw_str(canvas, 2, 30, "www.flipp.dev/compliance");
    // }

    canvas_set_font(canvas, FontPrimary);
    // canvas_draw_str(canvas, 2, 10, "Hello world motherfucka!");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_icon(canvas, plugin->num_x, plugin->num_y, &I_Test_BLE_128x64);

    // char buffer_x[10];
    // char buffer_y[10];
    // snprintf(buffer_x,sizeof(buffer_x),"%d", plugin->num_x);
    // snprintf(buffer_y,sizeof(buffer_y),"%d", plugin->num_y);
    
    // strcat("X:", buffer_x);
    // strcat("Y:", buffer_y);

    // canvas_draw_str(canvas, 2, 10, buffer_x);
    // canvas_draw_str(canvas, 20, 10, buffer_y);
}

static void test_plugin_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    osMessageQueueId_t event_queue = ctx;

    TestPluginEvent event = {.type = EventTypeKey, .input = *input_event};
    osMessageQueuePut(event_queue, &event, 0, 0);
}

int32_t test_plugin_app(void* p){
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(TestPluginEvent), NULL);
    TestPluginNum plugin = {.num_x = 0, .num_y = 0};

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    furi_check(view_port);
    view_port_draw_callback_set(view_port, test_plugin_draw_callback, &plugin);
    view_port_input_callback_set(view_port, test_plugin_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    TestPluginEvent event;

    while(1) {
        furi_check(osMessageQueueGet(event_queue, &event, NULL, osWaitForever) == osOK);
        if(event.type == EventTypeKey) {
            if (event.input.type == InputTypeShort) {
                if (event.input.key == InputKeyBack) {
                    view_port_enabled_set(view_port, false);
                    gui_remove_view_port(gui, view_port);
                    view_port_free(view_port);
                    osMessageQueueDelete(event_queue);
                    return 0;
                } else {
                    if (event.input.key == InputKeyUp && plugin.num_y != 0) {
                        plugin.num_y--;
                    } else if (event.input.key == InputKeyDown && plugin.num_y != 3) {
                        plugin.num_y++;
                    } else if (event.input.key == InputKeyLeft) {
                        plugin.num_x--;
                    } else if (event.input.key == InputKeyRight) {
                        plugin.num_x++;
                    }
                    view_port_update(view_port);
                }
            }
        }
    }
    
    return 0;
}