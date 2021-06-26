
typedef struct Renderer_texture_id {
    int id;
} Renderer_texture_id;

typedef struct Renderer_texture {
    Vulkan_texture texture;
    Renderer_texture_id id;
    const char *name;
} Renderer_texture;

typedef enum Renderer_command_type {
    renderer_command_type_regular,
    renderer_command_type_font
} Renderer_command_type;

typedef struct Renderer_command {
    Renderer_command_type type;
    Mesh mesh;
    Transform_data transform_data;
    Renderer_texture_id texture;
} Renderer_command;

typedef struct Renderer {
    Vulkan_buffer vertex_buffer;
    Renderer_texture *textures_vector;
    VkPipeline regular_pipeline;
    VkPipeline font_pipeline;
    Renderer_command *commands_vector;
} Renderer;

Renderer g_renderer;

Renderer_texture_id renderer_get_texture_by_name(const char *name) {
    for (int i = 0; i < get_sbuff_length(g_renderer.textures_vector); ++i) {
        if (is_literal_string_equal(name, g_renderer.textures_vector[i].name)) {
            return g_renderer.textures_vector[i].id;
        }
    }
    
    assert(0);
}

void renderer_update_textures() {
    int num_textures = vector_get_length(g_renderer.textures_vector);
    assert(num_textures > 0);
    Vulkan_texture *textures = malloc(num_textures * sizeof(Vulkan_texture));
    for (int i = 0; i < num_textures; ++i) {
        textures[i] = g_renderer.textures_vector[i].texture;
    }
    vulkan_set_textures(textures, num_textures);
}

#define d_max_num_vertices 10000

void renderer_init(Platform *platform) {
    g_vulkan.prepared = true;
    init_vulkan(platform);
    prepare_vulkan();
    
    //create dynamic vertex buffer
    g_renderer.vertex_buffer = vulkan_create_buffer(sizeof(Vertex) * d_max_num_vertices, NULL, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    map_buffer(&g_renderer.vertex_buffer);
    
    //create pipelines
    size_t vertex_shader_length;
    const char *vertex_shader_code = read_file("src/vulkan/shaders/vertex_shader.spv",  &vertex_shader_length);
    size_t fragment_shader_length;
    const char *fragment_shader_code = read_file("src/vulkan/shaders/fragment_shader.spv",  &fragment_shader_length);
    size_t font_fragment_shader_length;
    const char *font_fragment_shader_code = read_file("src/vulkan/shaders/fragment_shader.spv",  &font_fragment_shader_length);
    VkShaderModule vertex_shader = vulkan_create_shader_module(vertex_shader_code, vertex_shader_length);
    VkShaderModule fragment_shader = vulkan_create_shader_module(fragment_shader_code, fragment_shader_length);
    VkShaderModule font_fragment_shader = vulkan_create_shader_module(font_fragment_shader_code, font_fragment_shader_length);
    g_renderer.regular_pipeline= vulkan_create_pipeline(vertex_shader, fragment_shader, pipeline_option_enable_blending);
    g_renderer.font_pipeline= vulkan_create_pipeline(vertex_shader, font_fragment_shader, pipeline_option_enable_blending);
}

void renderer_deinit() {
    
}

Mesh renderer_create_mesh_from_rect_with_color(Rect rect, v4 color) {
    Vertex *vertices = malloc(sizeof(Vertex) * 6);
    int *indices = malloc(sizeof(int) * 1);
    
    glm_vec3_copy((vec3){ rect.x0, rect.y0, 0 }, vertices[0].pos);
    glm_vec3_copy((vec3){ rect.x1, rect.y0, 0 }, vertices[1].pos);
    glm_vec3_copy((vec3){ rect.x1, rect.y1, 0 }, vertices[2].pos);
    glm_vec3_copy((vec3){ rect.x1, rect.y1, 0 }, vertices[3].pos);
    glm_vec3_copy((vec3){ rect.x0, rect.y1, 0 }, vertices[4].pos);
    glm_vec3_copy((vec3){ rect.x0, rect.y0, 0 }, vertices[5].pos);
    
    glm_vec2_copy((vec2){ 0, 0 }, vertices[0].textcoords);
    glm_vec2_copy((vec2){ 1, 0 }, vertices[1].textcoords);
    glm_vec2_copy((vec2){ 1, 1 }, vertices[2].textcoords);
    glm_vec2_copy((vec2){ 1, 1 }, vertices[3].textcoords);
    glm_vec2_copy((vec2){ 0, 1 }, vertices[4].textcoords);
    glm_vec2_copy((vec2){ 0, 0 }, vertices[5].textcoords);
    
    for (int i = 0 ; i < 6; ++i) {
        vertices[i].color = color;
    }
    
    Mesh mesh = { .vertices = vertices, .indices = indices, .num_vertices = 6, .num_indices = 1 };
    
    return mesh;
}

Mesh renderer_create_mesh_from_rect(Rect rect) {
    return renderer_create_mesh_from_rect_with_color(rect, default_color);
}

Renderer_texture_id renderer_create_texture(Texture texture) {
    Vulkan_texture vulkan_texture = vulkan_create_texture(texture);
    
    Renderer_texture_id id = (Renderer_texture_id){ vector_get_length(g_renderer.textures_vector) - 1 };
    Renderer_texture renderer_texture = {
        .id = id,
        .texture = vulkan_texture,
        .name = texture.name
    };
    
    vector_add(g_renderer.textures_vector, renderer_texture);
    
    return id;
}

void renderer_begin_frame() {
    g_renderer.commands_vector = null;
}

void renderer_create_commands() {
    int num_commands = vector_get_length(g_renderer.commands_vector);
    Vertex *vertices = null;
    Vulkan_renderer_object *objects = null;
    for (int i = 0; i < num_commands; ++i) {
        Renderer_command command = g_renderer.commands_vector[i];
        int num_vertices = command.mesh.num_vertices;
        assert(num_vertices > 0);
        for (int j = 0; j < num_vertices; ++j) {
            vector_add(vertices, command.mesh.vertices[j]);
        }
        Vulkan_renderer_object object = {
            .transform_data = command.transform_data,
            .num_vertices = num_vertices,
            .push_constant_data = {
                .texture_index = command.texture.id //NOTE: careful might break when changing textures
            } 
        };
        if(command.type == renderer_command_type_regular) {
            object.pipeline = g_renderer.regular_pipeline;
        }
        else if (command.type == renderer_command_type_font) {
            object.pipeline = g_renderer.font_pipeline;
        }
        vector_add(objects, object);
    }
    
    int total_num_vertices = vector_get_length(vertices);
    assert(total_num_vertices <= d_max_num_vertices);
    
#if 1
    vulkan_update_buffer(&g_renderer.vertex_buffer, vertices, total_num_vertices * sizeof(Vertex));
#endif
    
    int num_objects = vector_get_length(objects);
    vulkan_create_command_buffer(g_renderer.vertex_buffer, objects, num_objects, g_renderer.regular_pipeline);
#if 1
    vector_free(vertices);
    vector_free(objects);
    vector_free(g_renderer.commands_vector);
#endif
}

void renderer_submit_frame() {
    vulkan_next_frame();
    //view_changed(&uniform_buffer, &background_ubo_vs);
    if (g_vulkan.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(g_vulkan.device);
    }
}

void renderer_end_frame() {
    renderer_create_commands();
    renderer_submit_frame();
}

void renderer_internal_push_command(Renderer_command command) {
    vector_add(g_renderer.commands_vector, command);
}

void renderer_internal_clean_command_buffer() {
    vector_free(g_renderer.commands_vector);
}

Transform_data get_default_transform_data() {
    Transform_data transform_data;
    glm_mat4_identity(transform_data.model);
    glm_mat4_identity(transform_data.view);
    glm_mat4_identity(transform_data.projection);
    
    return transform_data;
}

void renderer_draw_mesh(Mesh mesh, Transform_data transform_data, Renderer_texture_id texture) {
    assert(mesh.num_vertices > 0);
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = mesh,
        .transform_data = transform_data
    };
    renderer_internal_push_command(command);
}

void renderer_draw_rect_with_color(Rect rect, Transform_data transform_data, Renderer_texture_id texture, v4 color) {
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = renderer_create_mesh_from_rect_with_color(rect, color),
        .transform_data = transform_data
    };
    renderer_internal_push_command(command);
}

void renderer_draw_rect(Rect rect, Transform_data transform_data, Renderer_texture_id texture) {
    renderer_draw_rect_with_color(rect, transform_data, texture, default_color);
}

void renderer_draw_rect_to_screen_with_color(Rect rect, Renderer_texture_id texture, v4 color) {
    
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = renderer_create_mesh_from_rect_with_color(rect, color),
        .transform_data = get_default_transform_data()
    };
    renderer_internal_push_command(command);
}

void renderer_draw_rect_to_screen(Rect rect, Renderer_texture_id texture) {
    renderer_draw_rect_to_screen_with_color(rect, texture, default_color);
}

void renderer_draw_text_to_screen(const char *text, Rect rect, v4 color) {
    float x0 = rect.x0;
    float y0 = rect.y0;
    
    for (int i = 0; i < strlen(text); ++i) {
        //Texture *regular_texture = &g_char_textures['a'];
        char character = text[i];
        if (character == ' ') {
            x0 += 0.005;
            continue;
        }
        Font_texture font_texture = g_char_textures[character];
        Texture regular_texture = font_texture.texture;
        //textures[i] = create_vulkan_texture(&regular_texture);
        
        float width = regular_texture.width / (float)screen_width;
        float height = regular_texture.height  / (float)screen_height;
        //float width = get_rect_width(rect);
        //float height =   get_rect_height(rect);
        float y_offset = (font_texture.y_offset) / (float)screen_height;
        float x_offset = (font_texture.x_offset) / (float)screen_width;
        float advance = font_texture.advance / (float)screen_width;
        //float advance  = 0;
        
        //y1 += height;
        
        Rect drawing_rect = (Rect){
            .x0 = x0,
            .x1 = x0 + width ,
            .y0 = y0 + y_offset + 0.08,
            .y1 = y0 + height + y_offset + 0.08
        };
        
        x0 += width - x_offset + advance;
        
        char char_texture_name[20];
        sprintf(char_texture_name, "%c_char_texture", character);
        
        Renderer_command command = {
            .type = renderer_command_type_font,
            .texture = renderer_get_texture_by_name(char_texture_name),
            .mesh = renderer_create_mesh_from_rect(rect)
        };
        renderer_internal_push_command(command);
    }
}