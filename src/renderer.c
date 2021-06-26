
#define default_z_depth -0.2f

#define max_num_fonts 10
#define max_num_font_atlases 20
#define max_num_renderer_textures 50
#define max_num_renderer_commands 3000
#define max_num_objects max_num_renderer_commands

#define default_font g_assets.fonts[0]
#define default_font_size 25

typedef enum Layer {
    layer_default,
    layer_background,
    layer_nevmesh,
    layer_items,
    layer_characters,
    layer_player,
    layer_screen,
} Layer;

typedef struct Renderer_texture_id {
    int id;
} Renderer_texture_id;

typedef struct Renderer_texture {
    Vulkan_texture texture;
    Renderer_texture_id id;
    char name[standrad_name_length];
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
    Layer layer;
} Renderer_command;

typedef struct Font_atlas {
    Renderer_texture_id texture_id;
    u32 scale;
    Font_t font;
} Font_atlas;

typedef struct Renderer {
    Vulkan_buffer vertex_buffer;
    
    Renderer_texture *textures;
    u32 num_textures;
    
    Renderer_command *commands;
    u32 num_commands;
    
    VkPipeline regular_pipeline;
    VkPipeline font_pipeline;
    
    stbtt_fontinfo font;
    stbtt_packedchar packed_characters[128];
    
    Font_atlas *font_atlases;
    u32 num_font_atlases;
    
} Renderer;

Renderer g_renderer;

void renderer_internal_sort_commands() {
    //TODO: remove bubble sort
    bool sorted = false;
    while (!sorted) {
        sorted = true;
        for (int i = 0; i < g_renderer.num_commands; ++i) {
            if (g_renderer.commands[i].layer >  g_renderer.commands[i + 1].layer) {
                Renderer_command temp = g_renderer.commands[i];
                g_renderer.commands[i] = g_renderer.commands[i + 1];
                g_renderer.commands[i + 1] = temp;
                sorted = false;
            }
        }
    }
}

Renderer_texture_id renderer_get_texture_by_name(const char *name) {
    if (!name || !(*name))
        return (Renderer_texture_id){0};
    
    for (int i = 0; i < g_renderer.num_textures; ++i) {
        if (is_literal_string_equal(name, g_renderer.textures[i].name, standrad_name_length)) {
            return g_renderer.textures[i].id;
        }
    }
    
    return (Renderer_texture_id){0};
}

Renderer_texture *renderer_get_texture_from_id(Renderer_texture_id id) {
    return &g_renderer.textures[id.id];
}

void renderer_update_textures() {
    int num_textures = g_renderer.num_textures;
    assert(num_textures > 0);
    Vulkan_texture *textures = (Vulkan_texture *)allocate_frame((num_textures)* sizeof(Vulkan_texture));
    for (int i = 0; i < num_textures; ++i) {
        textures[i] = g_renderer.textures[i].texture;
    }
    vulkan_update_textures(textures, num_textures );
}

#define d_max_num_vertices 10000

Renderer_texture_id renderer_create_texture(Texture texture) {
    
    assert(g_renderer.num_textures < max_num_renderer_textures);
    
    Vulkan_texture vulkan_texture = vulkan_create_texture(texture);
    
    Renderer_texture_id id = (Renderer_texture_id){
        g_renderer.num_textures
    };
    Renderer_texture renderer_texture = {
        .id = id,
        .texture = vulkan_texture,
    };
    
    copy_string_to_buffer(texture.name, renderer_texture.name, standrad_name_length);
    
    g_renderer.textures[g_renderer.num_textures++] = renderer_texture;
    
    return id;
}

Font_atlas renderer_internal_add_font_atlas(Renderer_texture_id texture_id, Font_t font, u32 scale) {
    Font_atlas font_atlas = (Font_atlas) { 
        .texture_id = texture_id,
        .font = font,
        .scale = scale
    };
    assert(g_renderer.num_font_atlases < max_num_font_atlases);
    g_renderer.font_atlases[g_renderer.num_font_atlases++] = font_atlas;
    
    return font_atlas;
}

Font_atlas renderer_internal_create_font_atlas(u32 scale, Font_t font, const char *name) {
    
    const int bitmap_width = 512, bitmap_height = 512;
    
    u8 *bitmap = allocate(bitmap_width * bitmap_height, perm_alloc);
    assert(bitmap);
    //u8 *bitmap = malloc(bitmap_width * bitmap_height);
    
    stbtt_pack_context context;
    int result = stbtt_PackBegin(&context, bitmap, bitmap_width, bitmap_height, 0, 1, null);
    assert(result);
    stbtt_PackSetOversampling(&context, 1, 1);
    stbtt_PackFontRange(&context, font.font_info.data, 0, scale, 0, 127, g_renderer.packed_characters);
    stbtt_PackEnd(&context); 
    
    Texture font_atlas_texture = {
        .width = bitmap_width,
        .height  = bitmap_height,
        .channels = 1,
        .data = (u8 *)bitmap,
        .size = bitmap_width * bitmap_height * 1
    };
    
    copy_string_to_buffer(name, font_atlas_texture.name, standrad_name_length); 
    
    Renderer_texture_id id = renderer_create_texture(font_atlas_texture);
    
    Font_atlas atlas = renderer_internal_add_font_atlas(id, font, scale);
    
    return atlas;
}

Font_atlas get_or_create_font_atlas(u32 scale, Font_t font) {
    for (int i = 0; i < g_renderer.num_font_atlases; ++i) {
        if (strcmp(font.name, g_renderer.font_atlases[i].font.name) == 0 && scale == g_renderer.font_atlases[i].scale) {
            return g_renderer.font_atlases[i];
        }
    }
    
    Font_atlas atlas = renderer_internal_create_font_atlas(scale, font, "font_atlas");
    renderer_update_textures();
    
    return atlas;
}

void renderer_init(Platform *platform) {
    g_renderer.textures = (Renderer_texture *)allocate_perm(sizeof(Renderer_texture) * max_num_renderer_textures);
    g_renderer.commands = (Renderer_command *)allocate_perm(sizeof(Renderer_command) * max_num_renderer_commands);
    
    g_vulkan.prepared = true;
    init_vulkan(platform);
    prepare_vulkan();
    
    //create dynamic vertex buffer
    g_renderer.vertex_buffer = vulkan_create_buffer(sizeof(Vertex) * d_max_num_vertices, NULL, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    map_buffer(&g_renderer.vertex_buffer);
    
    //create dynamic uniform buffer
    vulkan_create_dynamic_uniform_buffer(max_num_objects);
    
    //create pipelines
    size_t vertex_shader_length;
    const char *vertex_shader_code = read_file("src/vulkan/shaders/vertex_shader.spv",  &vertex_shader_length);
    VkShaderModule vertex_shader = vulkan_create_shader_module(vertex_shader_code, vertex_shader_length);
    
    size_t fragment_shader_length;
    const char *fragment_shader_code = read_file("src/vulkan/shaders/fragment_shader.spv",  &fragment_shader_length);
    VkShaderModule fragment_shader = vulkan_create_shader_module(fragment_shader_code, fragment_shader_length);
    g_renderer.regular_pipeline= vulkan_create_pipeline(vertex_shader, fragment_shader, pipeline_option_enable_blending);
    
    size_t font_fragment_shader_length;
    const char *font_fragment_shader_code = read_file("src/vulkan/shaders/font_fragment_shader.spv",  &font_fragment_shader_length);
    VkShaderModule font_fragment_shader = vulkan_create_shader_module(font_fragment_shader_code, font_fragment_shader_length);
    g_renderer.font_pipeline= vulkan_create_pipeline(vertex_shader, font_fragment_shader, pipeline_option_enable_blending);
    
    Texture null_texture = load_texture("textures/null.jpeg", "null");
    assert(null_texture.data);
    renderer_create_texture(null_texture);
    
    g_renderer.font_atlases = (Font_atlas *)allocate_perm(sizeof(Font_atlas) * max_num_font_atlases);
    Font_t font = get_font("open_sans_bold");
    renderer_internal_create_font_atlas(25, font, "font_atlas");
    
    //update textures
    renderer_update_textures();
}

void renderer_deinit() {
    
}

Mesh renderer_create_mesh_from_pos_and_uvs_rect_left_corner(Rect pos_rect, Rect uv_rect, Allocation_type allocation_type) {
    Vertex *vertices = (Vertex *)allocate(sizeof(Vertex) * 6, allocation_type);
    int *indices = (int *)allocate(sizeof(int) * 1, allocation_type);
    
#if 0
    vertices[0].pos = get_v3(pos_rect.x0, pos_rect.y0, 0);
    vertices[1].pos = get_v3(pos_rect.x1, pos_rect.y0, 0);
    vertices[2].pos = get_v3(pos_rect.x1, pos_rect.y1, 0);
    vertices[3].pos = get_v3(pos_rect.x1, pos_rect.y1, 0);
    vertices[4].pos = get_v3(pos_rect.x0, pos_rect.y1, 0);
    vertices[5].pos = get_v3(pos_rect.x0, pos_rect.y0, 0);
#else
    vertices[0].pos = get_v3(pos_rect.x0, pos_rect.y0, 0);
    vertices[1].pos = get_v3(pos_rect.x1, pos_rect.y0, 0);
    vertices[2].pos = get_v3(pos_rect.x1, pos_rect.y1, 0);
    vertices[3].pos = get_v3(pos_rect.x1, pos_rect.y1, 0);
    vertices[4].pos = get_v3(pos_rect.x0, pos_rect.y1, 0);
    vertices[5].pos = get_v3(pos_rect.x0, pos_rect.y0, 0);
#endif
    
    vertices[0].textcoords = get_v2(uv_rect.u0, uv_rect.v0);
    vertices[1].textcoords = get_v2(uv_rect.u1, uv_rect.v0);
    vertices[2].textcoords = get_v2(uv_rect.u1, uv_rect.v1);
    vertices[3].textcoords = get_v2(uv_rect.u1, uv_rect.v1);
    vertices[4].textcoords = get_v2(uv_rect.u0, uv_rect.v1);
    vertices[5].textcoords = get_v2(uv_rect.u0, uv_rect.v0);
    
    for (int i = 0 ; i < 6; ++i) {
        vertices[i].color = default_color;
    }
    
    Mesh mesh = { .vertices = vertices, .indices = indices, .num_vertices = 6, .num_indices = 1 };
    
    return mesh;
}

Mesh renderer_create_mesh_from_rect_with_color_ui(Rect rect, v4 color, Allocation_type allocation_type) {
    //TODO: Fix the permalloc asap
    Vertex *vertices = (Vertex *)allocate(sizeof(Vertex) * 6, allocation_type);
    int *indices = (int *)allocate(sizeof(int) * 1, allocation_type);
    
    vertices[0].pos = get_v3(rect.x0, rect.y0, 0);
    vertices[1].pos = get_v3(rect.x1, rect.y0, 0);
    vertices[2].pos = get_v3(rect.x1, rect.y1, 0);
    vertices[3].pos = get_v3(rect.x1, rect.y1, 0);
    vertices[4].pos = get_v3(rect.x0, rect.y1, 0);
    vertices[5].pos = get_v3(rect.x0, rect.y0, 0);
    
#if 0
    vertices[0].textcoords = get_v2(0 ,0);
    vertices[1].textcoords = get_v2(1 ,0);
    vertices[2].textcoords = get_v2(1 ,1);
    vertices[3].textcoords = get_v2(1 ,1);
    vertices[4].textcoords = get_v2(0 ,1);
    vertices[5].textcoords = get_v2(0 ,0);
#else
    vertices[0].textcoords = get_v2(0 ,1);
    vertices[1].textcoords = get_v2(1 ,1);
    vertices[2].textcoords = get_v2(1 ,0);
    vertices[3].textcoords = get_v2(1 ,0);
    vertices[4].textcoords = get_v2(0 ,0);
    vertices[5].textcoords = get_v2(0 ,1);
#endif
    
    for (int i = 0 ; i < 6; ++i) {
        vertices[i].color = color;
    }
    
    Mesh mesh = { .vertices = vertices, .indices = indices, .num_vertices = 6, .num_indices = 1 };
    
    return mesh;
}

Mesh renderer_create_mesh_from_rect_with_color(Rect rect, v4 color, Allocation_type allocation_type) {
    //TODO: Fix the permalloc asap
    Vertex *vertices = (Vertex *)allocate(sizeof(Vertex) * 6, allocation_type);
    int *indices = (int *)allocate(sizeof(int) * 1, allocation_type);
    
#if 1
    vertices[0].pos = get_v3(rect.x0, rect.y0, 0);
    vertices[1].pos = get_v3(rect.x1, rect.y0, 0);
    vertices[2].pos = get_v3(rect.x1, rect.y1, 0);
    vertices[3].pos = get_v3(rect.x1, rect.y1, 0);
    vertices[4].pos = get_v3(rect.x0, rect.y1, 0);
    vertices[5].pos = get_v3(rect.x0, rect.y0, 0);
#else
    vertices[0].pos = get_v3(rect.x0, rect.y0, default_z_depth);
    vertices[1].pos = get_v3(rect.x1, rect.y0, default_z_depth);
    vertices[2].pos = get_v3(rect.x1, rect.y1, default_z_depth);
    vertices[3].pos = get_v3(rect.x1, rect.y1, default_z_depth);
    vertices[4].pos = get_v3(rect.x0, rect.y1, default_z_depth);
    vertices[5].pos = get_v3(rect.x0, rect.y0, default_z_depth);
#endif
    
#if 0
    vertices[0].textcoords = get_v2(0 ,0);
    vertices[1].textcoords = get_v2(1 ,0);
    vertices[2].textcoords = get_v2(1 ,1);
    vertices[3].textcoords = get_v2(1 ,1);
    vertices[4].textcoords = get_v2(0 ,1);
    vertices[5].textcoords = get_v2(0 ,0);
#else
    vertices[0].textcoords = get_v2(0 ,1);
    vertices[1].textcoords = get_v2(1 ,1);
    vertices[2].textcoords = get_v2(1 ,0);
    vertices[3].textcoords = get_v2(1 ,0);
    vertices[4].textcoords = get_v2(0 ,0);
    vertices[5].textcoords = get_v2(0 ,1);
#endif
    
    for (int i = 0 ; i < 6; ++i) {
        vertices[i].color = color;
    }
    
    Mesh mesh = { .vertices = vertices, .indices = indices, .num_vertices = 6, .num_indices = 1 };
    
    return mesh;
}

Mesh renderer_create_mesh_from_rect(Rect rect, Allocation_type allocation_type) {
    return renderer_create_mesh_from_rect_with_color(rect, default_color, allocation_type);
}


void renderer_begin_frame() {
    
}

void renderer_create_commands() {
    Vertex *vertices = null;
    Transform_data *transforms = null;
    Vulkan_renderer_object *objects = null;
    
    renderer_internal_sort_commands();
    
    for (int i = 0; i < g_renderer.num_commands; ++i) {
        Renderer_command command = g_renderer.commands[i];
        int num_vertices = command.mesh.num_vertices;
        if (!num_vertices)
            continue;
        for (int j = 0; j < num_vertices; ++j) {
            vector_add(vertices, command.mesh.vertices[j]);
        }
        Vulkan_renderer_object object = {
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
        vector_add(transforms, command.transform_data);
    }
    
    int total_num_vertices = vector_get_length(vertices);
    assert(total_num_vertices <= d_max_num_vertices);
    
    vulkan_update_buffer(&g_renderer.vertex_buffer, vertices, total_num_vertices * sizeof(Vertex));
    
    int num_objects = vector_get_length(objects);
    
    char *aligned_transforms = (char *)allocate(g_vulkan.uniform_buffer_dynamic_alignment * num_objects, frame_alloc);
    
    for (int i = 0; i < num_objects; ++i) {
        memcpy(&aligned_transforms[i * g_vulkan.uniform_buffer_dynamic_alignment], &transforms[i], sizeof(Transform_data));
    }
    
    vulkan_update_buffer(&g_vulkan.uniform_buffer, aligned_transforms, g_vulkan.uniform_buffer_dynamic_alignment * num_objects);
    
    vulkan_create_command_buffer(g_renderer.vertex_buffer, objects, num_objects, g_renderer.regular_pipeline);
    
    g_renderer.num_commands = 0;
    
    vector_free(vertices);
    vector_free(objects);
    vector_free(transforms);
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
    assert(g_renderer.num_commands < max_num_renderer_commands);
    g_renderer.commands[g_renderer.num_commands++] = command;
}

Transform_data get_default_transform_data() {
    Transform_data transform_data;
    
    transform_data.model = math_get_identity_matrix();
    transform_data.view = math_get_identity_matrix();
    transform_data.projection = math_get_identity_matrix();
    
    return transform_data;
}

void renderer_draw_mesh(Mesh mesh, Transform_data transform_data, Renderer_texture_id texture, Layer layer) {
    assert(mesh.num_vertices > 0);
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = mesh,
        .transform_data = transform_data,
        .layer = layer
    };
    renderer_internal_push_command(command);
}

void renderer_draw_rect_with_color(Rect rect, Transform_data transform_data, Renderer_texture_id texture, v4 color, Layer layer) {
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = renderer_create_mesh_from_rect_with_color(rect, color, frame_alloc),
        .transform_data = transform_data,
        .layer = layer
    };
    renderer_internal_push_command(command);
}

void renderer_draw_outline_with_color(Rect rect, Transform_data transform_data, v4 color, float weight, Layer layer) {
    Rect rect0 = (Rect){ rect.x0, rect.y0, rect.x1, rect.y0 + weight };
    Rect rect1 = (Rect){ rect.x0, rect.y0, rect.x0 + weight, rect.y1 };
    Rect rect2 = (Rect){ rect.x0, rect.y1 - weight, rect.x1, rect.y1 };
    Rect rect3 = (Rect){ rect.x1 - weight, rect.y0, rect.x1, rect.y1 };
    
    renderer_draw_rect_with_color(rect0, transform_data, renderer_get_texture_by_name("white_texture"), color, layer);
    renderer_draw_rect_with_color(rect1, transform_data, renderer_get_texture_by_name("white_texture"), color, layer);
    renderer_draw_rect_with_color(rect2, transform_data, renderer_get_texture_by_name("white_texture"), color, layer);
    renderer_draw_rect_with_color(rect3, transform_data, renderer_get_texture_by_name("white_texture"), color, layer);
}

void renderer_draw_rect(Rect rect, Transform_data transform_data, Renderer_texture_id texture, Layer layer) {
    renderer_draw_rect_with_color(rect, transform_data, texture, default_color, layer);
}

void renderer_draw_rect_to_screen_with_color(Rect rect, Renderer_texture_id texture, v4 color) {
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = renderer_create_mesh_from_rect_with_color_ui(rect, color, frame_alloc),
        .transform_data = get_default_transform_data(),
        .layer = layer_screen
    };
    renderer_internal_push_command(command);
}

void renderer_draw_rect_to_screen_with_color_and_layer(Rect rect, Renderer_texture_id texture, v4 color, Layer layer) {
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = texture,
        .mesh = renderer_create_mesh_from_rect_with_color_ui(rect, color, frame_alloc),
        .transform_data = get_default_transform_data(),
        .layer = layer
    };
    renderer_internal_push_command(command);
}

void renderer_draw_rect_to_screen(Rect rect, Renderer_texture_id texture) {
    renderer_draw_rect_to_screen_with_color(rect, texture, default_color);
}

typedef enum Rect_alignment {
    rect_alignment_left_top,
    rect_alignment_left_center,
    rect_alignment_center,
} Rect_alignment;

void renderer_draw_text_to_screen(const char *text, Font_t font, u32 font_scale, Rect rect, v4 color, Rect_alignment text_alignment, float text_offset) {
    Font_atlas font_atlas = get_or_create_font_atlas(font_scale, font);
    float scale = stbtt_ScaleForPixelHeight(&font.font_info, font_scale);
    int ascent_, descent_, line_gap_;
    stbtt_GetFontVMetrics(&font.font_info, &ascent_, &descent_, &line_gap_);
    float ascent, descent, line_gap;
    ascent = ascent_ * scale;
    descent = descent_ * scale;
    line_gap = line_gap_ * scale;
    
    float x = rect.x0 + text_offset;
    float y = rect.y0;
    convert_from_ndc_to_screen_coords(&x, &y, g_screen_width, g_screen_height);
    y = convert_opengl_screenspace_to_vulkan_screenspace(get_v2(x, y), g_screen_height).y;
    float x_line_start = x;
    
    Rect *pos_rects_vector = null;
    Rect *uvs_rects_vector = null;
    
#if 0
    switch(text_alignment) {
        case rect_alignment_left_top: {
            y -= ascent;
            break;
        }
        case rect_alignment_left_center: case rect_alignment_center: {
            float rect_y0 = rect.y0;
            float rect_y1 = rect.y1;
            convert_from_ndc_to_screen_coords(null, &rect_y0, g_screen_width, g_screen_height);
            convert_from_ndc_to_screen_coords(null, &rect_y1, g_screen_width, g_screen_height);
            float rect_height = rect_y1 - rect_y0;
            float text_height = ascent - descent;
            float offset = (rect_height - text_height) / 2;
            y  = y - (ascent + offset);
            break;
        }
    }
#endif
    
    const char *text_ptr = text;
    while (*text_ptr) {
        if (*text_ptr == '\n') {
            float line_offset = line_gap + ascent - descent;
            y += line_offset;
            x = x_line_start;
            text_ptr++;
            continue;
        }
        
        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(g_renderer.packed_characters, 512, 512, *text_ptr++, &x, &y, &quad, 0);
        
        Rect pos_rect = {
            .vec0 = get_v2(quad.x0, quad.y0),
            .vec1 = get_v2(quad.x1, quad.y1),
        };
        
        pos_rect.vec0 = convert_vulkan_screenspace_to_opengl_screenspace(pos_rect.vec0, g_screen_height);
        pos_rect.vec1 = convert_vulkan_screenspace_to_opengl_screenspace(pos_rect.vec1, g_screen_height);
        
        pos_rect.vec0 = convert_from_screen_coords_to_ndc(pos_rect.vec0, g_screen_width, g_screen_height);
        pos_rect.vec1 = convert_from_screen_coords_to_ndc(pos_rect.vec1, g_screen_width, g_screen_height);
        
        Rect uvs_rect = {
            .vec0 = get_v2(quad.s0, quad.t0),
            .vec1 = get_v2(quad.s1, quad.t1),
        };
        
        vector_add(uvs_rects_vector, uvs_rect);
        vector_add(pos_rects_vector, pos_rect);
    }
    
    if (text_alignment == rect_alignment_center) {
        float text_width = pos_rects_vector[vector_get_length(pos_rects_vector) - 1].x1 - pos_rects_vector[0].x0;
        float rect_x0 = rect.x0;
        float rect_x1 = rect.x1;
        float offset = ((rect_x1 - rect_x0) - text_width) / 2;
        for (int i = 0; i < vector_get_length(pos_rects_vector); ++i) {
            pos_rects_vector[i] = move_rect(pos_rects_vector[i], get_v2(offset, 0));
        }
    }
    
    for (int i = 0; i < vector_get_length(pos_rects_vector); ++i) {
        Renderer_command command = {
            .type = renderer_command_type_font,
            //.texture = renderer_get_texture_by_name("font_atlas"),
            .texture = font_atlas.texture_id,
            .mesh = renderer_create_mesh_from_pos_and_uvs_rect_left_corner(pos_rects_vector[i], uvs_rects_vector[i], perm_alloc),
            .transform_data = get_default_transform_data(),
            .layer = layer_screen
        };
        renderer_internal_push_command(command);
    }
    
    vector_free(pos_rects_vector);
    vector_free(uvs_rects_vector);
}

void renderer_draw_text_to_screen_formatted(Font_t font, u32 scale, Rect rect, v4 color, Rect_alignment text_alignment, float text_offset, const char *text, ...) {
    va_list args;
    va_start(args, text);
    char formatted_text[200];
    vsprintf(formatted_text, text, args);
    renderer_draw_text_to_screen(formatted_text, font, scale, rect, color, text_alignment, text_offset);
}

void renderer_draw_line_to_screen(v2 start, v2 end, float weight, v4 color) {
    
    Vertex *vertices = (Vertex *)allocate(sizeof(Vertex) * 6, frame_alloc);
    int *indices = (int *)allocate(sizeof(int) * 1, frame_alloc);
    
    vertices[0].pos = get_v3(start.x, start.y, 0);
    vertices[1].pos = get_v3(end.x, end.y - weight, 0);
    vertices[2].pos = get_v3(end.x, end.y, 0);
    
    vertices[3].pos = get_v3(end.x, end.y, 0);
    vertices[4].pos = get_v3(start.x, start.y + weight, 0);
    vertices[5].pos = get_v3(start.x, start.y, 0);
    
    vertices[0].textcoords = get_v2(0 ,1);
    vertices[1].textcoords = get_v2(1 ,1);
    vertices[2].textcoords = get_v2(1 ,0);
    
    vertices[3].textcoords = get_v2(1 ,0);
    vertices[4].textcoords = get_v2(0 ,0);
    vertices[5].textcoords = get_v2(0 ,1);
    
    for (int i = 0 ; i < 6; ++i) {
        vertices[i].color = color;
    }
    
    Mesh mesh = { .vertices = vertices, .indices = indices, .num_vertices = 6, .num_indices = 1 };
    
    Renderer_command command = {
        .type = renderer_command_type_regular,
        .texture = renderer_get_texture_by_name("white_texture"),
        .mesh = mesh,
        .transform_data = get_default_transform_data(),
        .layer = layer_screen
    };
    renderer_internal_push_command(command);
    
}

