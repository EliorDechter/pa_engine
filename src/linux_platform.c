
#define VK_USE_PLATFORM_XLIB_KHR

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <unistd.h> 
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <alsa/asoundlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/inotify.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define STB_DS_IMPLEMENTATION
#include "externals/stb_ds.h"

#define STB_IMAGE_IMPLEMENTATION
#include "externals/stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h" 

#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/stb_truetype.h"

#include "externals/cglm/cglm.h"

#pragma GCC diagnostic pop

#if 0
const int  g_screen_height = 550; 
const int g_screen_width  = g_screen_height * (16.0f / 9.0f);
#else
const int  g_screen_height = 650; 
const int g_screen_width  = g_screen_height * (16.0f / 9.0f);
#endif

#include "common.c"
#include "math.c"

typedef struct Vertex {
    v3 pos;
    v2 textcoords;
    v4 color;
} Vertex;

typedef struct Mesh {
    Vertex *vertices;
    int *indices;
    int num_vertices, num_indices;
} Mesh;

typedef struct Texture { //NOTE: this is a circualr dependencey between renderer and assts
    char name[standrad_name_length];
    u32 width, height, channels;
    u8 *data;
    u32 size;
} Texture;

typedef struct Transform {
    v3 pos;
    v3 scale;
    v3 rotation;
    
    Transform_data *transform_data;
} Transform;

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <time.h>
//#include "multithreading.c"
//#include "global_data.c"
//#include "parser.c"

#define default_color get_v4(1, 1, 1, 1)

#define default_text_offset 0.01

#define get_sbuff_length arrlen
#define vector_get_length arrlen
#define append_to_sbuff arraddnptr
#define put_in_sbuff arrput
#define vector_add arrput
#define set_sbuff_length arrsetlen
#define vector_set_length arrsetlen
#define vector_add_n_ptr arraddnptr
#define vector_free arrfree

#include <x86intrin.h>

typedef enum Key {
    key_none,
    key_0, key_1, key_2, key_3, key_4, key_5, key_6, key_7, key_8, key_9,
    key_a,
    key_b, 
    key_c,
    key_d,
    key_e,
    key_f,
    key_g,
    key_h,
    key_i,
    key_j,
    key_k,
    key_l,
    key_m,
    key_n,
    key_o,
    key_p,
    key_q,
    key_r,
    key_s,
    key_t_,
    key_u,
    key_w,
    key_x,
    key_y,
    key_z,
    key_right_arrow, key_left_arrow, key_up_arrow, key_down_arrow,  key_escape, key_unknown, key_left_control,
    key_enter, key_backspace,
    num_keyboard_keys } Key;

typedef enum Mouse_button { left_mouse_button, right_mouse_button, scroll_mouse_up, scroll_mouse_down, num_mouse_buttons } Mouse_button;

typedef struct Button {
    bool down, pressed, released;
} Button;

typedef struct Input {
    v2 mouse_pos;
    v2 previous_mouse_pos;
    Button mouse_buttons[num_mouse_buttons];
    Button keys[num_keyboard_keys];
    char current_clicked_key;
} Input;

typedef struct Platform {
    Display *display;
    int screen;
    Window root_window;
    Window window;
    
    Input input;
    
    int screen_width, screen_height;
    
    bool quit;
    char *error;
    
    float delta_time_in_seconds;
    float delta_time_in_miliseconds;
    float time;
} Platform;

Platform g_platform;

static double get_time() {
    struct timespec time_spec;
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    
    return (double)time_spec.tv_sec + (double)time_spec.tv_nsec / 1e9;
}

static void set_button(Button *button, bool down) {
    if (down) {
        button->down = true;
        button->released = false;
        button->pressed = true;
    }
    else {
        button->down = false;
        button->released = true;
        button->pressed = false;
    }
}

static void handle_key_event(int first_key_code, bool down) {
    int keysyms_per_keycode_return;
    KeySym *key_sym;
    
    key_sym = XGetKeyboardMapping(g_platform.display, first_key_code, 1, &keysyms_per_keycode_return);
    
    Key key = 0;
    switch (key_sym[0]) {
        case XK_a:     key = key_a;     break;
        case XK_b:    key = key_b;     break;
        case XK_c:      key = key_c;    break;
        case XK_d:      key = key_d;    break;
        case XK_e:      key = key_e;     break;
        case XK_f:      key = key_f;     break;
        case XK_g:      key = key_g;     break;
        case XK_h:      key = key_h;    break;
        case XK_i:      key = key_i;    break;
        case XK_j:      key = key_j;    break;
        case XK_k:      key = key_k;     break;
        case XK_l:      key = key_l;     break;
        case XK_m:      key = key_m;     break;
        case XK_n:    key = key_n;     break;
        case XK_o:      key = key_o;    break;
        case XK_p:      key = key_p;    break;
        case XK_q:      key = key_q;     break;
        case XK_r:      key = key_r;     break;
        case XK_s:      key = key_s;     break;
        case XK_t:      key = key_t_;    break;
        case XK_u:      key = key_u;    break;
        case XK_w:      key = key_w;    break;
        case XK_x:      key = key_x;     break;
        case XK_y:      key = key_y;     break;
        case XK_z:      key = key_z;     break;
        case XK_Control_L:      key = key_left_control;     break;
        case XK_Right:      key = key_right_arrow;     break;
        case XK_Left:     key = key_left_arrow;     break;
        case XK_Up:      key = key_up_arrow;     break;
        case XK_Down:     key = key_down_arrow;     break;
        case XK_1:      key = key_1;     break;
        case XK_2:      key = key_2;     break;
        case XK_3:      key = key_3;     break;
        case XK_4:      key = key_4;     break;
        case XK_5:      key = key_5;     break;
        case XK_6:      key = key_6;     break;
        case XK_7:      key = key_7;     break;
        case XK_8:      key = key_8;     break;
        case XK_9:      key = key_9;     break;
        case XK_0:      key = key_0;     break;
        case XK_Escape:  key = key_escape; break;
        case XK_Return:  key = key_enter; break;
        case XK_BackSpace:  key = key_backspace; break;
        default:       key = key_none;   break;
    }
    
    if (down)
        g_platform.input.current_clicked_key = (char)key_sym[0];
    
    set_button(&g_platform.input.keys[key], down);
    
    XFree(key_sym);
}

static void handle_mouse_button_event(int xbutton, bool down) {
    Mouse_button button;
    switch(xbutton) {
        case Button1: button = left_mouse_button; break;
        case Button3: button = right_mouse_button; break;
    }
    
    set_button(&g_platform.input.mouse_buttons[button], down);
}

static v2 get_mouse_position() {
    Window root, child;
    int root_x, root_y, window_x, window_y;
    u32 mask;
    XQueryPointer(g_platform.display, g_platform.window, &root, &child, &root_x, &root_y, &window_x, &window_y, &mask);
    
    v2 pos = get_v2(window_x, window_y);
    pos = convert_vulkan_screenspace_to_opengl_screenspace(pos, g_screen_height);
    
    return pos;
}

static void handle_client_event(XClientMessageEvent *event) {
    static Atom protocols = None;
    static Atom delete_window = None;
    if (protocols == None) {
        protocols = XInternAtom(g_platform.display, "WM_PROTOCOLS", True);
        delete_window = XInternAtom(g_platform.display, "WM_DELETE_WINDOW", True);
        assert(protocols != None);
        assert(delete_window != None);
    }
    if (event->message_type == protocols) {
        Atom protocol = event->data.l[0];
        if (protocol == delete_window) {
            g_platform.quit = true;
        }
    }
}

static void handle_events() {
    g_platform.input.current_clicked_key = 0;
    int num_pending_events = XEventsQueued(g_platform.display, QueuedAfterReading);
    int i = 0;
    for (int i = 0; i < num_keyboard_keys; ++i) {
        g_platform.input.keys[i].down = g_platform.input.keys[i].released = false;
    }
    for (int i = 0; i < num_mouse_buttons; ++i) {
        g_platform.input.mouse_buttons[i].down = g_platform.input.mouse_buttons[i].released = false;
    }
    
    while(XPending(g_platform.display)) {
        ++i;
        XEvent event;
        XNextEvent(g_platform.display, &event);
        
        if (event.type == Expose) {
            
        }
        else if (event.type == ClientMessage) {
            handle_client_event(&event.xclient);
        } 
        
        if (event.type == KeyPress) {
            handle_key_event(event.xkey.keycode, true);
        }
        else if (event.type == KeyRelease) {
            bool is_released = true;
            if (XEventsQueued(g_platform.display, QueuedAfterReading)) {
                XEvent next_event;
                XNextEvent(g_platform.display, &next_event);
                
                if (next_event.type == KeyPress && next_event.xkey.time == event.xkey.time && next_event.xkey.keycode == next_event.xkey.keycode) {
                    is_released = false;
                }
            }
            if (is_released) {
                handle_key_event(event.xkey.keycode, false);
            }
        }
        
        if (event.type == ButtonPress) {
            handle_mouse_button_event(event.xbutton.button, true);
        } 
        else if (event.type == ButtonRelease) {
            handle_mouse_button_event(event.xbutton.button, false);
        }
        else {
            
        }
    }
    
    v2 mouse_pos = get_mouse_position();
    g_platform.input.previous_mouse_pos = g_platform.input.mouse_pos;
    g_platform.input.mouse_pos = mouse_pos;
    
    XFlush(g_platform.display); 
}

static void  init_platform(int screen_width, int screen_height, const char *title) {
    Display *display = XOpenDisplay(NULL);
    assert(display);
    
    int screen = XDefaultScreen(display);
    unsigned long border = XWhitePixel(display, screen);
    unsigned long background = XBlackPixel(display, screen);
    Window root_window = XRootWindow(display, screen);
    
    Window window = XCreateSimpleWindow(display, root_window, 0, 0, screen_width, screen_height, 0,
                                        border, background);
    
    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize;
    size_hints->min_width = screen_width;
    size_hints->max_width = screen_width;
    size_hints->min_height = screen_height;
    size_hints->max_height = screen_height;
    XSetWMNormalHints(display, window, size_hints);
    XFree(size_hints);
    
    XClassHint *class_hint = XAllocClassHint();
    class_hint->res_name = (char*)title;
    class_hint->res_class = (char*)title ;
    XSetClassHint(display, window, class_hint);
    XFree(class_hint);
    
    long mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;
    XSelectInput(display, window, mask);
    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window, &delete_window, 1);
    
    XMapWindow(display, window);
    
    bool supported_rtrn;
    
    Platform platform = {0};
    platform.display = display;
    platform.screen = screen;
    platform.root_window = root_window;
    platform.window = window;
    platform.screen_width = screen_width;
    platform.screen_height = screen_height;
    platform.quit = false;
    platform.delta_time_in_seconds = 0;
    platform.time = get_time();
    platform.error = null;
    
    g_platform = platform;
}

static void deinit_platform() {
    XCloseDisplay(g_platform.display);
    XDestroyWindow(g_platform.display, g_platform.window);
}

static bool pull_platform() {
    double time = get_time();
    g_platform.delta_time_in_seconds = time - g_platform.time;
    g_platform.delta_time_in_miliseconds = g_platform.delta_time_in_seconds / 1000;
    g_platform.time = time;
    handle_events();
    
    return !g_platform.quit;
}

#define linux_assert(error) (assert(error >= 0))

#include "spine/spine.h"
#include "assets.c"
#include "vulkan/vulkan_renderer.c"
#include "renderer.c"
#include "spine.c" 
#include "audio.c"
#include "ui.c"
#include "game.c"

bool g_are_assets_loaded = false;
bool g_is_loading_assets = false;

static void *load_assets_thread(void *data) {
    Asset_database_item *asset_items = (Asset_database_item *)data;
    
#if 1
    load_all_assets(asset_items, vector_get_length(asset_items));
#else
    load_from_asset_file("asset_file");
    g_assets.spine_assets = load_spine_assets("spine_assets/export/spineboy.atlas", "spine_assets/export/spineboy-ess.json");
#endif
    
    g_are_assets_loaded = true;
}

Asset_database_item *create_asset_database() {
    
    Asset_database_item *items_vector = null;
    vector_add(items_vector, ((Asset_database_item){"background_texture", "textures/background.jpg", asset_type_texture}));
    vector_add(items_vector, ((Asset_database_item){"door_texture", "textures/door.jpg", asset_type_texture}));
    vector_add(items_vector, ((Asset_database_item){"background_2_texture", "textures/background_2.jpeg", asset_type_texture}));
    vector_add(items_vector, ((Asset_database_item){"button_texture", "textures/button_background.jpeg", asset_type_texture}));
    vector_add(items_vector, ((Asset_database_item){"white_texture", "textures/white.jpeg", asset_type_texture}));
    vector_add(items_vector, ((Asset_database_item){"spine_texture", "spine_assets/export/spineboy.png", asset_type_texture}));
    vector_add(items_vector, ((Asset_database_item){"cubism_texture", "textures/cubism.jpeg", asset_type_texture}));
    
    
#if 1
    vector_add(items_vector, ((Asset_database_item){"cannonball", "audio/02 Cannonball.flac", asset_type_flac}));
    vector_add(items_vector, ((Asset_database_item){"footstep", "audio/Fotstep_Carpet_Right_3.mp3", asset_type_mp3}));
#endif
    
    return items_vector;
    
}

int main() {
    
    init_allocator(1, 1);
    init_platform(g_screen_width , g_screen_height, "PA");
    assets_init();
    renderer_init(&g_platform);
    audio_init();
    //audio_start();
    ui_init();
    
    Asset_database_item *items_vector = create_asset_database();
    bool is_game_init = false;
    pthread_t io_thread = {0};
    Playing_sound playing_sound = {0};
    
    const char *asset_file_name = "asset_file";
    //create_asset_file(asset_file_name, items_vector, vector_get_length(items_vector));
    
    while(pull_platform()) {
        clear_frame_memory();
        if (g_game_info.input_actions[input_action_exit]) {
            if (!save_game("save_file")) {
                fprintf(stderr, "failed to save game\n");
            }
            g_platform.quit = true;
        }
        
        if (g_game_info.input_actions[input_action_save]) {
            if (!save_game("save_file")) {
                fprintf(stderr, "failed to save game\n");
            }
        }
        
        if (g_game_info.input_actions[input_action_load]) {
            if (!load_game("save_file")) {
                fprintf(stderr, "failed to load game\n");
            }
        }
        
        if (g_game_info.input_actions[input_action_destory_all_entities]) {
            destroy_all_entities_and_scenes();
        }
        
        renderer_begin_frame();
        {
            begin_ui_frame();
            
            //TODO: add synchronous loading path
            if (!g_are_assets_loaded && !g_is_loading_assets) {
                pthread_create(&io_thread, null, load_assets_thread, (void *)items_vector);
                g_is_loading_assets = true;
            }
            if (!g_are_assets_loaded && g_is_loading_assets) {
                Rect text_rect = {-1, -1, 1, -0.7};
                renderer_draw_text_to_screen("loading...", default_font, default_font_size, text_rect, get_v4(1, 1, 1, 1), rect_alignment_left_top, default_text_offset);
            }
            if (g_are_assets_loaded && g_is_loading_assets) {
                for (int i = 0; i < vector_get_length(g_assets.textures_vector); ++i) {
                    renderer_create_texture(g_assets.textures_vector[i]);
                }
                renderer_update_textures();
                g_is_loading_assets = false;
            }
            
            if (!is_game_init && g_are_assets_loaded) {
                bool use_save_file = false;
                init_game(use_save_file);
                is_game_init = true;
                audio_play_sound(assets_get_sound_by_name("cannonball"), 1);
            }
            
            if (is_game_init) {
                if (g_game_info.game_state == game_state_main_menu) {
                    run_main_menu();
                }
                else {
                    run_game();
                }
                
                output_to_debug_view("ms: %.1f\n", g_platform.delta_time_in_seconds * 1000);
                
                draw_debug_string();
                
                
                //Sound *sound = assets_get_sound_by_name("footstep");
                //audio_play_sound(sound, 1);
                //Sound *sound = 0;
                //sound = assets_get_sound_by_name("");
                
                audio_mix_sounds(audio_max_num_frames_to_play);
                //int num_frames_played = linux_push_to_sound_buffer(g_audio.samples, audio_max_num_frames_to_play);
                int num_frames_played = 0;
                audio_update_playing_sounds(num_frames_played);
            }
            
            interact_with_ui();
            end_ui_frame();
        }
        renderer_end_frame();
        
        flush_debug_view();
        
        if (g_platform.delta_time_in_seconds < 1 / 60) {
            //sleep(1 / 60 - g_platform.delta_time_in_seconds);
        }
    }
    
    pthread_join(io_thread, null);
    
    audio_deinit();
    free_allocator();
    renderer_deinit();
    deinit_platform();
    
    vector_free(items_vector);
    
    return 0;
}

