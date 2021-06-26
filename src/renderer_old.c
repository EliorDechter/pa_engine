
typedef enum Camera_type { perspective_camera, orthographic_camera } Camera_type;

typedef struct Camera {
    vec3 pos;
    v3 target;
    float aspect;
    
    mat4 view_matrix;
    mat4 projection_matrix;
    Camera_type camera_type;
} Camera;

Camera *g_camera;

static Camera create_camera(Camera_type camera_type) {
    Camera camera = {0};
    glm_mat4_identity(camera.view_matrix);
    glm_mat4_identity(camera.projection_matrix);
    float fov   =  45.0f;
    if (camera_type == perspective_camera) {
        glm_perspective(glm_rad(fov), screen_width / screen_height, 0.1f, 100.0f, camera.projection_matrix);
    }
    else if (camera_type == orthographic_camera) {
        
    }
    
    return camera;
}

#if 0
void move_camera(v3 amount) {
    g_camera->pos =  add_v3(g_camera->pos, amount);
}
#endif

#if 0
typedef enum Command { command_draw } Command;

typedef struct Command_buffer {
    Command *commands;
    int num_commands;
    int max_num_commands;
    void *data;
    void *data_pointer;
    size_t max_data_size;
} Command_buffer;

Command_buffer create_command_buffer() {
    Command_buffer command_buffer = {0};
    command_buffer.max_num_commands = 100;
    command_buffer.max_data_size = convert_gibibytes_to_bytes(0.3);
    command_buffer.commands = (Command *)allocate_frame(g_allocator, sizeof(Command) * command_buffer.max_num_commands);
    command_buffer.data = (void *)allocate_perm(g_allocator, command_buffer.max_data_size);
    
    return command_buffer;
}

typedef struct Texture { //NOTE: this is a circualr dependencey between renderer and assts
    const char *name;
    u32 width, height, channels;
    u8 *data;
    u32 size;
} Texture;

typedef u32 Texture_handle;
Texture textures[100];

typedef struct Draw_command_data {
    int num_vertices;
    float *vertices;
    Texture_handle texture;
} Draw_command_data;

void push_command(Command_buffer *command_buffer, Draw_command_data *draw_command_data) {
    assert(command_buffer->num_commands + 1 <= command_buffer->max_num_commands);
    assert(command_buffer->max_data_size + sizeof(Draw_command_data) <= command_buffer->max_data_size);
    command_buffer->commands[command_buffer->num_commands++] = command_draw;
    memcpy(command_buffer->data_pointer, draw_command_data, sizeof(Draw_command_data));
    command_buffer->data_pointer += sizeof(Draw_command_data);
}

//NOTE: packing gives an improvement of 2x ?!
typedef struct __attribute__((packed)) Vertex  {
    //typedef struct Vertex {
    v3 pos;
    v2 uv;
    v2 normal;
    //v3 texture;
} Vertex;


void change_object_size(Vertex *vertices, u32 num_vertices, int amount) {
    for (u32 i = 0; i < num_vertices; ++i) {
        vertices[i].pos.x *= amount;
        vertices[i].pos.y *= amount;
    }
}

typedef struct Framebuffer{
    u32 width, height;
    unsigned char *color_buffer;
    float *depth_buffer;
} Framebuffer;

static void framebuffer_release(Framebuffer *framebuffer) {
    free(framebuffer->color_buffer);
    free(framebuffer->depth_buffer);
    free(framebuffer);
}

typedef struct Vertex_buffer {
    Vertex *vertices;
    u32 num_vertices;
} Vertex_buffer;

typedef struct Renderer_settings {
    //debug stuff
    bool should_process_vertices;
    bool run_once;
    enum vertices_source { vertices_source_model, vertices_source_simple_image } vertices_source;
} Renderer_settings;

typedef struct Software_renderer_data {
    Renderer_settings *settings;
    Allocator *allocator;
    Worker *main_worker;
    Framebuffer *framebuffer;
    Texture *texture;
    Renderer_settings *renderer_settings;
} Software_renderer_data;

typedef enum Renderer_type { renderer_type_software, renderer_type_vulkan } Renderer_type;

#if 0
typedef struct Renderer {
    Renderer_type type;
    union {
        Software_renderer_data software_renderer_data;
        //Vulkan_renderer_data vulkan_renderer_data;
    };
} Renderer;
#endif

void software_renderer_render(Software_renderer_data *data, Vertex_buffer *vertex_buffer); //FORWARD DECLARATION

Vertex_buffer create_vertex_buffer(float *float_vertices, u32 num_vertices) {
    assert(num_vertices % 3 == 0);
    Vertex *vertices = (Vertex *)allocate_perm(g_allocator, sizeof(Vertex) * num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        vertices[i].pos = get_v3(float_vertices[5 * i + 0], float_vertices[5 * i + 1], float_vertices[5 * i + 2]);
        vertices[i].uv = get_v2(float_vertices[5 * i + 3], float_vertices[5 * i + 4]);
        vertices[i].normal = get_v2(0, 0);
    }
    
    for (int i = 0; i < num_vertices / 3; i += 3) {
        assert(vertices[i].pos.x >= vertices[i + 1].pos.x || vertices[i].pos.y >= vertices[i + 1].pos.y);
        assert(vertices[i + 1].pos.x >= vertices[i + 2].pos.x || vertices[i + 1].pos.y >= vertices[i + 2].pos.y);
    }
    
    Vertex_buffer vertex_buffer = {.vertices = vertices, .num_vertices = num_vertices};
    return vertex_buffer;
}

#if 0
void execute_commands(Command_buffer *command_buffer, Renderer *renderer, Vertex_buffer *out_vertex_buffer) {
    int num_vertices = 0;
    float *float_vertex_buffer = (float *)get_frame_allocator_pointer();
    for (int i = 0; i < command_buffer->num_commands; ++i) {
        Command command = command_buffer->commands[i];
        void *current_command_data = command_buffer->data;
        switch(command) {
            case command_draw: {
                Draw_command_data *data = (Draw_command_data *)current_command_data;
                current_command_data += sizeof(Draw_command_data);
                float *vertices = (float *)allocate_frame(g_allocator, sizeof(data->num_vertices));
                memcpy(vertices, data->vertices, data->num_vertices * sizeof(float));
                num_vertices += data->num_vertices;
                break;
            }
            default: {
                assert(0);
            }
        }
    }
    
    Vertex_buffer vertex_buffer = create_vertex_buffer(float_vertex_buffer, num_vertices); //this doesnt actually create soa. consider using a union or casting for the same operation
    *out_vertex_buffer = vertex_buffer;
}
#endif
void triangulate_rect(Rect rect, float *vertices) {
    float float_rect[] = {
        rect.x_max, rect.y_min, 0, 1, 0,
        rect.x_min, rect.y_min, 0, 0, 0,
        rect.x_min, rect.y_max, 0, 0, 1,
        rect.x_min, rect.y_max, 0, 0, 1,
        rect.x_max, rect.y_max, 0, 1, 1,
        rect.x_max, rect.y_min, 0, 1, 0
    };
    
    memcpy(vertices, float_rect, sizeof(float_rect));
}

bool compare_c_strings(const char *a, const char *b) {
    if (strcmp(a, b) == 0) 
        return true;
    return false;
}


Texture_handle get_texture(const char *name) {
    for (int i = 0; i < array_count(textures); ++i) {
        assert(compare_c_strings(textures[i].name, name));
        return i;
    }
}

void draw_rect(Command_buffer *command_buffer, Rect rect) {
    float *vertices = (float *)allocate_frame(g_allocator, sizeof(float) * 6);
    triangulate_rect(rect, vertices);
    Draw_command_data command = {
        .num_vertices = 6,
        .vertices = vertices,
        .texture = get_texture("index.jpeg")
    };
    push_command(command_buffer, &command);
}

//SOFTWARE_RENDERER

typedef struct Image{
    u32 width, height, channels;
    u8 *buffer;
} Image;

static Image create_image(Allocator *allocator, u32 width, u32 height, u32 channels) {
    u32 num_elements = width * height * channels;
    
    assert(width > 0 && height > 0 && channels >= 1 && channels <= 4);
    
    Image image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.buffer = NULL;
    
    u32 buffer_size = (u32)sizeof(unsigned char) * num_elements;
    image.buffer = allocate_perm(allocator, buffer_size);
    memset(image.buffer, 0, buffer_size);
    
    return image;
}

static void blit_image(Framebuffer *framebuffer, Image *image) {
    u32 tile_width = 2;
    u32 tile_height = 2;
    u32 width_in_tiles = (framebuffer->width + tile_width - 1) / tile_width;
    
    for (u32 y = 0; y < framebuffer->height; ++y) {
        for (u32 x = 0; x < framebuffer->width; ++x) {
            
            u32 tile_x = x / tile_width;
            u32 tile_y = y / tile_height;
            u32 in_tile_x = x % tile_width;
            u32 in_tile_y = y % tile_height;
            u32 src_index = (tile_y * width_in_tiles + tile_x) * (tile_width * tile_height) + in_tile_y * tile_width + in_tile_x;
            u32 dst_index = y * framebuffer->width + x;
            
            u32 *src_pixel = (u32 *)framebuffer->color_buffer + src_index;
            u32 *dst_pixel = (u32 *)image->buffer + dst_index;
            
            u8 *src_pixel_elements = (u8 *)src_pixel;
            u8 *dst_pixel_elements = (u8 *)dst_pixel;
            
            dst_pixel_elements[0] = src_pixel_elements[2];  /* blue */
            dst_pixel_elements[1] = src_pixel_elements[1];  /* green */
            dst_pixel_elements[2] = src_pixel_elements[0];  /* red */
        }
    }
}

typedef struct Software_renderer_platform {
    Framebuffer framebuffer;
    Image surface;
    XImage *ximage;
} Software_renderer_platform;


void deinit_software_renderer_platform(Software_renderer_platform *platform) {
    XDestroyImage(platform->ximage);
}

void begin_software_renderer_frame() {
    
}
