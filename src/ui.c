#include "common.c"

typedef enum Ui_interaction_type {
    ui_interaction_none,
    ui_interaction_nop,
    ui_interaction_immediate_button
} Ui_interaction_type;

typedef enum Widget_status { widget_present, widget_not_present } Widget_status;

int g_widget_id_generator = 0;

typedef struct Widget {
    bool valid;
    int id;
    Ui_interaction_type type;
} Widget;

typedef struct Ui_layout {
    v3 pos_left_corner;
    float spacing;
    Rect_alignment text_alignment;
    float text_offset;
} Ui_layout;

typedef struct Ui_context {
    Widget half_clicked_widget, hovered_widget, fully_clicked_widget;
    Ui_layout current_layout;
} Ui_context;

Ui_context g_ui_context;

typedef struct Rect_triangles {
    v3 positions[6];
} Rect_triangles;

Rect_triangles create_rect_triangles(Rect rect) {
    Rect_triangles rect_triangles;
    rect_triangles.positions[0] = get_v3(rect.x_max, rect.y_min, 0);
    rect_triangles.positions[1] = get_v3(rect.x_min, rect.y_min, 0);
    rect_triangles.positions[2] = get_v3(rect.x_min, rect.y_max, 0);
    rect_triangles.positions[3] = get_v3(rect.x_min, rect.y_max, 0);
    rect_triangles.positions[4] = get_v3(rect.x_max, rect.y_max, 0);
    rect_triangles.positions[5] = get_v3(rect.x_max, rect.y_min, 0);
    
    return rect_triangles;
}

bool did_mouse_go_up() {
    bool result = g_platform.input.mouse_buttons[left_mouse_button].released;
    
    return result;
}

bool did_mouse_go_down() {
    return g_platform.input.mouse_buttons[left_mouse_button].down;
}

bool check_if_mouse_hovering_over_widget(Widget widget, Rect rect) {
    bool is_hovering = is_point_inside_rect(rect, g_platform.input.mouse_pos);
    if (is_hovering) {
        //printf("hovering\n");
        g_ui_context.hovered_widget = widget;
        g_ui_context.hovered_widget.valid = true;
    }
    
    return is_hovering;
}

void begin_ui_frame() {
    
}

void ui_begin_layout(Ui_layout layout) {
    g_ui_context.current_layout = layout;
}

void ui_end_layout() {
    memset(&g_ui_context.current_layout, 0, sizeof(Ui_layout));
}

void end_ui_frame() {
    g_widget_id_generator = 0;
}

bool is_widget_equal(Widget a, Widget b) {
    if (a.id == b.id && a.valid && b.valid) 
        return true;
    return false;
}

Widget create_widget() {
    Widget widget = {0};
    widget.valid = true;
    widget.id = g_widget_id_generator++;
    
    return widget;
}

//NOTE: all these functions assume vulkan coordinates
Rect add_rects(Rect rect0, Rect rect1) {
    Rect rect;
    rect.x_min = rect0.x_min + rect1.x_min;
    rect.x_max = rect0.x_max + rect1.x_max;
    rect.y_min = rect0.y_min + rect1.y_min;
    rect.y_max = rect0.y_max + rect1.y_max;
    
    return rect;
}

Rect divide_rect(Rect rect, float value) {
    Rect new_rect = rect;
    new_rect.x_min /= value;
    new_rect.y_min /= value;
    new_rect.x_max /= value;
    new_rect.y_max /= value;
    
    return new_rect;
}

Rect convert_rect_ndc_to_screen_coords(Rect rect) {
    Rect new_rect = add_rects(rect, (Rect){1, 1, 1, 1});
    new_rect = divide_rect(new_rect, 2);
    new_rect.x_min *= g_screen_width;
    new_rect.y_min *= g_screen_height;
    new_rect.x_max *= g_screen_width;
    new_rect.y_max *= g_screen_height;
    
    return new_rect;
}

Rect scale_rect(Rect rect, float value) {
    Rect new_rect = {0};
}

v2 get_rect_center(Rect rect) {
    float rect_center_x = rect.x_min + (rect.x_max - rect.x_min) / 2;
    float rect_center_y = rect.y_min + (rect.y_max - rect.y_min) / 2;
    
    return get_v2(rect_center_x, rect_center_y);
}

Rect center_rect(Rect parent_rect, float width, float height) {
    v2 center = get_rect_center(parent_rect);
    float x_offset = width / 2;
    float y_offset = height / 2;
    return (Rect) {
        .x_min = center.x - x_offset,
        .x_max = center.x + x_offset,
        .y_min = center.y - y_offset,
        .y_max = center.y + y_offset
    };
}

v4 change_color(v4 v, float scalar) {
    v4 result = get_v4(v.x * scalar, v.y * scalar, v.z * scalar, v.a);
    
    return result;
}

v4 change_color_alpha(v4 v, float new_alpha) {
    v4 result = get_v4(v.x ,v.y, v.z, new_alpha);
    
    return result;
}

bool button_image_text(bool enabled, Rect rect, const char *text, v4 panel_color, v4 text_color, Renderer_texture_id texture, Rect_alignment text_alignment, float text_offset) {
    Widget widget = create_widget(); 
    Rect screen_coords_rect = convert_rect_ndc_to_screen_coords(rect);
    bool is_hovering = check_if_mouse_hovering_over_widget(widget, screen_coords_rect);
    bool result = false;
    
    if (is_widget_equal(g_ui_context.fully_clicked_widget, widget)) {
        g_ui_context.fully_clicked_widget.valid = false;
        result = true;
    }
    
    if (is_widget_equal(g_ui_context.half_clicked_widget, widget)) {
        renderer_draw_rect_to_screen_with_color(rect, texture, change_color_alpha(panel_color, panel_color.a * 0.5));
    }
    else {
        if (is_hovering) {
            renderer_draw_rect_to_screen_with_color(rect, texture, change_color_alpha(panel_color, panel_color.a * 0.8));
        }
        else {
            renderer_draw_rect_to_screen_with_color(rect, texture, panel_color);
        }
    }
    
    renderer_draw_text_to_screen(text, default_font, default_font_size, rect, text_color, text_alignment, text_offset);
    
    return result;
}

bool panel(bool enabled, Rect rect, Renderer_texture_id texture_id, v4 color) {
    Widget widget = create_widget(); 
    
    renderer_draw_rect_to_screen_with_color(rect, texture_id, color);
}

void ui_text(bool enabled, Rect rect, const char *text, v4 text_color, Rect_alignment text_alignment, float text_offset) {
    renderer_draw_text_to_screen(text, default_font, default_font_size, rect, text_color, text_alignment, text_offset);
}

void interact_with_ui() {
    if (g_ui_context.half_clicked_widget.valid == true) {
        if (is_widget_equal(g_ui_context.half_clicked_widget, g_ui_context.hovered_widget)) {
            if (did_mouse_go_up()) {
                g_ui_context.fully_clicked_widget = g_ui_context.half_clicked_widget;
                g_ui_context.fully_clicked_widget.valid = true;
                g_ui_context.half_clicked_widget.valid = false;
            }
        }
        else {
            if (did_mouse_go_up()) {
                g_ui_context.fully_clicked_widget.valid = false;
                g_ui_context.half_clicked_widget.valid = false;
            }
        }
    }
    else {
        if(g_ui_context.hovered_widget.valid == true) {
            if(did_mouse_go_down()) {
                g_ui_context.half_clicked_widget = g_ui_context.hovered_widget;
                g_ui_context.half_clicked_widget.valid = true;
            }
        }
    }
    
    g_ui_context.hovered_widget.valid = false;
}

void ui_init() {
}