typedef enum Dragging_state { none_state, drag_state } Dragging_state;
typedef enum Game_mode { game_mode_normal, game_mode_dialogue } Game_mode;

#include "camera.c"


typedef struct Spine_animation_id {
    int id;
} Spine_animation_id;

typedef enum Game_state { game_state_playing, game_state_main_menu } Game_state;

typedef struct Entity_id {
    int id;
} Entity_id;

typedef struct Entity_animation {
    int id;
} Entity_animation;

typedef enum Entity_type {
    entity_type_static,
    entity_type_player,
    entity_type_npc,
} Entity_type;

typedef enum Animation_type { walk_animation, idle_animation } Animation_type;

typedef struct Spine_animation {
    Animation_type state;
    struct Spine_animation_data data;
} Spine_animation;

typedef enum Entity_rendering_type { none_rendering, regular_rendering, spine_rendering } Entity_rendering_type;

typedef enum Facing_direction { facing_right_direction, facing_left_direction } Facing_direction;

typedef enum Collision_type { collision_type_none,
    collision_type_collider,
    collision_type_collidable
} Collision_type;

typedef enum Collision_effect_type {
    collision_effect_type_none,
    collision_effect_type_change_scene 
} Collision_effect_type;

typedef struct Scene_id {
    int id;
} Scene_id;

typedef struct Dialogue_id {
    int id;
} Dialogue_id;

typedef struct Dialoguer {
    bool can_start_dialogue;
    Dialogue_id dialogue_id;
} Dialoguer;

typedef struct Collider {
    bool is_collidable;
    Collision_effect_type collision_effect_type;
    union {
        Scene_id next_scene;
    } effect;
} Collider;

typedef enum Click_effect { click_effect_none, click_effect_start_dialogue } Click_effect;

typedef struct Clicker {
    bool is_clickable;
    Click_effect click_effect;
} Clicker;

typedef struct Path {
    v2 *nodes;
    u32 num_nodes;
} Path;

typedef struct Path_mover {
    u32 current_node;
    bool is_moving_to_next_node;
} Path_mover;

typedef struct Entity {
    char name[standrad_name_length];
    Entity_type type;
    Entity_id id;
    //char texture_name[standrad_name_length];
    Entity_rendering_type rendering_type;
    Renderer_texture_id texture_id;
    Mesh mesh;
    Spine_animation spine_animation;
    v3 pos, scale;
    Bounding_box bounding_box;
    Dragging_state dragging_state;
    
    Facing_direction facing_direction;
    Transform transform;
    
    Collider collider;
    Dialoguer dialoguer;
    Clicker clicker;
    
    bool active;
    
    bool is_being_dragged;
    
    Layer layer;
    
    v3 target;
    bool is_moving;
    
    Path_mover path_mover;
    
} Entity;

typedef struct Scene {
    Entity_id *entity_vector;
    u32 num_entities;
    char name[standrad_name_length];
} Scene;

#define debug_string_length 400

typedef enum Input_action {
    input_action_focus_camera_on_player,
    input_action_free_camera,
    input_action_move_player_left,
    input_action_move_player_right,
    input_action_move_player_up,
    input_action_move_player_down,
    input_action_move_camera_right,
    input_action_move_camera_left,
    input_action_move_camera_up,
    input_action_move_camera_down,
    input_action_move_camera_in,
    input_action_move_camera_out,
    input_action_left_button_clicked,
    input_action_left_button_pressed,
    input_action_left_button_released,
    input_action_right_button_clicked,
    input_action_edit_mode,
    input_action_game_mode,
    input_action_enter_command,
    input_action_save,
    input_action_load,
    input_action_destory_all_entities,
    input_action_exit,
    input_action_num
} Input_action;

typedef struct String_128 {
    char str[128];
    u32 len;
} String_128;

#include "Shell.c"

#define max_num_polygon_clicks 100

typedef struct Grid {
    Rect rect;
    float tile_width, tile_height;
    u32 num_rows, num_columns, num_cells;
    bool *obstacles;
} Grid;

typedef struct Game_info {
    Camera camera;
    Camera editor_camera;
    Game_state game_state;
    Game_mode game_mode;
    Entity_id player_entity;
    Entity *entity_vector;
    Scene *scene_vector;
    Scene_id current_scene;
    
    bool init;
    
    bool is_in_edit_mode;
    
    char debug_string[debug_string_length];
    
    bool input_actions[input_action_num];
    
    struct {
        bool is_entering_command;
        String_128 command_text;
        
        v2 polygon_clicks[max_num_polygon_clicks];
        u32 num_polygon_clicks;
        bool is_drawing_polygon;
        Mesh nevmesh;
    } editor;
    
    Grid grid;
    Path path;
    
} Game_info;

Game_info g_game_info;

typedef struct Rect_mesh {
    Vertex vertices[6];
} Rect_mesh;

typedef struct Entity_save_data {
    char name[standrad_name_length];
    Entity_type type;
    char texture_name[standrad_name_length];
    char animation_name[standrad_name_length];
    Rect_mesh mesh;
    //Transform transform;
    Entity_rendering_type rendering_type;
    v3 pos;
    v3 scale;
    Collider collider;
    Layer layer;
} Entity_save_data;

Entity *get_entity(Entity_id entity_id) {
    Entity *entity = &g_game_info.entity_vector[entity_id.id];
    
    return entity;
}

Entity_id get_entity_id(const char *name) {
    for (int i = 1; i < vector_get_length(g_game_info.entity_vector); ++i) {
        if (is_literal_string_equal(g_game_info.entity_vector[i].name, name, standrad_name_length)) {
            return g_game_info.entity_vector[i].id;
        }
    }
    
    return (Entity_id){0};
}

void render_entity(Entity_id entity_id, Camera camera) {
    Entity *entity = get_entity(entity_id);
    entity->transform.transform_data->view = calculate_view_matrix_from_camera(camera);
    entity->transform.transform_data->projection = get_projection_matrix_from_camera(camera);
    if (entity->rendering_type == regular_rendering) {
        renderer_draw_mesh(entity->mesh, *entity->transform.transform_data, entity->texture_id, entity->layer);
    }
    else if (entity->rendering_type == spine_rendering) {
        spine_render(*entity->transform.transform_data, entity->texture_id, &entity->bounding_box, &entity->spine_animation.data, entity->layer);
    }
}

void output_to_debug_view(const char *format, ...) {
    char text[debug_string_length]; //NOTE: buggggyyyy
    va_list args;
    va_start(args, format);
    vsprintf(text, format, args);
    
    assert(strlen(text) + strlen(g_game_info.debug_string) < debug_string_length);
    
    strcat(g_game_info.debug_string, text);
    va_end(args);
}

void flush_debug_view() {
    *g_game_info.debug_string = 0;
}

Scene_id create_scene(const char *name) {
    Scene scene = {0};
    copy_string_to_buffer(name, scene.name, standrad_name_length);
    
    vector_add(g_game_info.scene_vector, scene);
    
    Scene_id scene_id = {
        .id = vector_get_length(g_game_info.scene_vector) - 1
    };
    
    return scene_id;
}

Scene *get_scene(Scene_id scene_id) {
    Scene *scene = &g_game_info.scene_vector[scene_id.id];
    
    return scene;
}

void add_entity_to_scene(Scene_id scene_id, Entity_id entity_id) {
    Scene *scene = get_scene(scene_id);
    vector_add(scene->entity_vector, entity_id);
}

void add_entities_to_scene(Entity_id *entity_ids, int num, Scene_id scene_id) {
    for (int i = 0; i < num; ++i) {
        add_entity_to_scene(scene_id, entity_ids[i]);
    }
}

void move_entity(Entity *entity, v3 amount) {
    //entity->transform.transform_data->model = math_translate(entity->transform.transform_data->model, amount);
    entity->pos = add_v3(entity->pos, amount);
}

void scale_entity(Entity *entity, v3 scale) {
    //entity->transform.transform_data->model = math_scale(entity->transform.transform_data->model, scale);
    entity->scale = mul_v3_hadmard(scale, entity->scale); //NOTE: potential bug?
}

void set_entity_pos(Entity *entity, v3 pos) {
    move_entity(entity, sub_v3(pos, entity->pos));
}

void set_entity_scale(Entity *entity, v3 scale) {
    scale_entity(entity, mul_v3_hadmard(scale, entity->scale));
}

Transform_data *create_transform_data() {
    Transform_data *transform_data = aligned_alloc(32, sizeof(Transform_data));
    
    transform_data->model = math_get_identity_matrix();
    transform_data->view = math_get_identity_matrix();
    transform_data->projection = math_get_identity_matrix();
    
    return transform_data;
}

Bounding_box get_bounding_box_from_mesh(Mesh mesh) {
    Bounding_box bounding_box = {
        .x0 = mesh.vertices[0].pos.x,
        .y0 = mesh.vertices[0].pos.y,
        .z0 = mesh.vertices[0].pos.z,
        .x1 = mesh.vertices[0].pos.x,
        .y1 = mesh.vertices[0].pos.y,
        .z1 = mesh.vertices[0].pos.z
    };
    
    for (int i = 0; i < mesh.num_vertices; ++i) {
        bounding_box.x0 = fmin(bounding_box.x0, mesh.vertices[i].pos.x);
        bounding_box.y0 = fmin(bounding_box.y0, mesh.vertices[i].pos.y);
        bounding_box.z0 = fmin(bounding_box.z0, mesh.vertices[i].pos.z);
        bounding_box.x1 = fmax(bounding_box.x1, mesh.vertices[i].pos.x);
        bounding_box.y1 = fmax(bounding_box.y1, mesh.vertices[i].pos.y);
        bounding_box.z1 = fmax(bounding_box.z1, mesh.vertices[i].pos.z);
    }
    
    return bounding_box;
}


Entity_id create_entity_with_texture(char *name, Mesh mesh, v3 pos, v3 scale, Renderer_texture_id texture_id,
                                     Entity_type type) {
    //NOTE: the pos argument isnt used
    Entity entity = {0};
    entity.scale = get_v3(1, 1, 1);
    entity.transform.transform_data = create_transform_data();
    entity.mesh = mesh;
    entity.texture_id = texture_id;
    copy_string_to_buffer(name, entity.name, standrad_name_length);
    entity.type = type;
    if (mesh.num_vertices > 0)
        entity.bounding_box = get_bounding_box_from_mesh(mesh);
    entity.dragging_state = none_state;
    entity.active = true;
    entity.target = pos;
    
    //set_entity_scale(&entity, scale);
    //set_entity_pos(&entity, pos);
    scale_entity(&entity, scale);
    move_entity(&entity, pos);
    
    Entity_id entity_id = {
        .id = vector_get_length(g_game_info.entity_vector)
    };
    entity.id = entity_id;
    
    vector_add(g_game_info.entity_vector, entity);
    
    return entity_id;
}

Entity_id create_entity(char *name, Mesh mesh, v3 pos, v3 scale, Renderer_texture_id texture_id, Entity_type type, Layer  layer, Collider collider) {
    Entity_id entity_id = create_entity_with_texture(name, mesh, pos, scale, texture_id, type);
    Entity *entity = get_entity(entity_id);
    entity->rendering_type = regular_rendering;
    entity->layer = layer;
    entity->collider = collider;
    
    return entity_id;
}

Entity_id create_animated_entity(char *name, v3 pos, v3 scale, Spine_animation_data spine_animation_data, Renderer_texture_id texture_id, Entity_type type, Layer layer, Collider collider) {
    Entity_id entity_id = create_entity_with_texture(name, (Mesh){0}, pos, scale, texture_id, type);
    Entity *entity = get_entity(entity_id);
    entity->spine_animation = (Spine_animation){0};
    entity->spine_animation.data = spine_animation_data;
    entity->rendering_type = spine_rendering;
    entity->layer = layer;
    entity->collider = collider;
    
    return entity_id;
}

m4 get_entity_model_matrix(Entity entity) {
    m4 result = math_scale(math_get_identity_matrix(), entity.scale);
    //TODO:rotate
    result = math_translate(result, entity.pos);
    
    return result;
}

v3 convert_vector_from_model_to_world_space(m4 model_matrix, v3 v) {
    v3 result = get_v3_from_v4(mul_m4_by_v4(model_matrix, get_v4(v.x, v.y, v.z, 1)));
    
    return result;
}


v3 convert_vector_from_world_to_model_space(m4 model_matrix, v3 v) {
    v3 result = get_v3_from_v4(mul_m4_by_v4(inverse_matrix(model_matrix), get_v4(v.x, v.y, v.z, 1)));
    
    return result;
}

void update_entity_model_matrix(Entity *entity) {
    entity->transform.transform_data->model = get_entity_model_matrix(*entity);
}

void render_scene(Scene_id scene_id, Camera camera) {
    Scene *scene = get_scene(scene_id);
    for (int i = 0; i < vector_get_length(scene->entity_vector); ++i) {
        update_entity_model_matrix(get_entity(scene->entity_vector[i]));
        render_entity(scene->entity_vector[i], camera);
    }
}

Mesh get_mesh_from_rect_mesh() {
    
}

Entity *get_entity_from_id(Entity_id id) {
    return &g_game_info.entity_vector[id.id];
}

void create_entity_from_entity_save_data(Entity_save_data data) {
    //TODO: add proper depth index
    
    Mesh mesh = {0};
    if (data.rendering_type == regular_rendering) {
        mesh.vertices = (Vertex *)allocate_perm(sizeof(Vertex) * 6);
        mesh.num_vertices = 6;
        memcpy(mesh.vertices, data.mesh.vertices, 6 * sizeof(Vertex));
    }
    
    Entity_id entity;
    
    Renderer_texture_id texture_id = renderer_get_texture_by_name(data.texture_name);
    if (data.rendering_type == regular_rendering) {
        entity = create_entity(data.name, mesh, data.pos, data.scale, texture_id, data.type, data.layer, data.collider);
    }
    else if (data.rendering_type == spine_rendering) {
        Spine_animation_data spine_animation_data = create_spine_animation(&g_assets.spine_assets);
        entity = create_animated_entity(data.name, data.pos, data.scale, spine_animation_data, texture_id, data.type, data.layer, data.collider);
    }
    
}

Entity_save_data get_entity_save_data_from_entity(Entity entity) {
    Rect_mesh mesh = {0};
    if (entity.rendering_type == regular_rendering) {
        if (entity.mesh.vertices) {
            memcpy(mesh.vertices, entity.mesh.vertices, 6 * sizeof(Vertex));
        }
    }
    
    Entity_save_data result = {
        .pos = entity.pos,
        .scale = entity.scale,
        .type = entity.type,
        .rendering_type = entity.rendering_type,
        .mesh = mesh,
        .collider = entity.collider,
        .layer = entity.layer
    };
    
    copy_string_to_buffer(entity.name, result.name, standrad_name_length);
    copy_string_to_buffer(renderer_get_texture_from_id(entity.texture_id)->name, result.texture_name, standrad_name_length);
    
    return result;
}

void start_dialogue(Game_info *game_info) {
    game_info->game_mode = game_mode_dialogue;
}

void change_entity_animation(Entity_id entity_id, Animation_type type) {
    Entity *entity = get_entity(entity_id);
    entity->spine_animation.state = type;
    const char *animation_name;
    switch (type) {
        case walk_animation: {
            animation_name = "walk";
            break;
        }
        case idle_animation: {
            animation_name = "idle";
            break;
        }
    }
    spAnimationState_setAnimationByName(entity->spine_animation.data.animation_state, 0, animation_name, 1);
}

void change_entity_facing_direction(Entity *entity, Facing_direction facing_direction) {
    entity->facing_direction = facing_direction;
    int flip_x = -1;
    if (facing_direction == facing_left_direction)
        flip_x = -1;
    //scale_entity(entity, get_v3(flip_x, 1, 1));
}

v3 get_center_of_bbox(Bbox bbox) {
    v3 result = {
        .x = bbox.min_x + (bbox.max_x - bbox.min_x) / 2,
        .y = bbox.min_y + (bbox.max_y - bbox.min_y) / 2,
        .z = bbox.min_z +  (bbox.max_z - bbox.min_z) / 2
    };
    
    return result;
}

void set_entity_animation(Entity *entity, Animation_type state) {
    entity->spine_animation.state = state;
    switch(state) {
        case walk_animation: {
            spAnimationState_setAnimationByName(entity->spine_animation.data.animation_state, 0, "walk", 1);
            break;
        }
        case idle_animation: {
            spAnimationState_setAnimationByName(entity->spine_animation.data.animation_state, 0, "idle", 1);
            break;
        }
    }
}

Bbox get_entity_bbox(Entity *entity) {
    Bbox bbox = get_bounding_box_world_coords(entity->bounding_box, entity->transform.transform_data->model);
    
    return bbox;
}

#define max_dialogue_line_length 200

typedef enum Dialogue_block_type { dialogue_block_regular, dialogue_block_choices } Dialogue_block_type;

typedef struct Dialogue_line {
    char str[max_dialogue_line_length];
    int next_block_tag;
} Dialogue_line;

typedef struct Dialogue_block {
    Dialogue_line *dialogue_line_vector;
    int tag;
    int *next_tags_vector;
    Dialogue_block_type type;
} Dialogue_block;

typedef struct Dialogue {
    const char *name;
    Dialogue_id dialogue_id;
    Dialogue_block *dialogue_block_vector;
    int current_block;
    bool active;
} Dialogue;

void add_dialogue_block(Dialogue *dialogue, Dialogue_block block) {
    put_in_sbuff(dialogue->dialogue_block_vector, block);
}

Dialogue_block create_dialogue_block(Dialogue_line *dialogue_line_vector) {
    Dialogue_block block = {
        .type = dialogue_block_regular,
        .dialogue_line_vector = dialogue_line_vector
    };
    
    return block;
}

Dialogue_block create_dialogue_block_with_tag(Dialogue_line *dialogue_line_vector, int tag) {
    Dialogue_block block = {
        .type = dialogue_block_regular,
        .dialogue_line_vector = dialogue_line_vector,
        .tag = tag
    };
    
    return block;
}

Dialogue_block create_dialogue_block_choices(Dialogue_line *dialogue_line_vector) {
    Dialogue_block block = {
        .type = dialogue_block_choices,
        .dialogue_line_vector = dialogue_line_vector,
        
    };
    
    return block;
}

Dialogue_line create_dialogue_line(const char *str) {
    Dialogue_line line = {0};
    strcpy(line.str, str);
    
    return line;
}

Dialogue_line create_dialogue_line_with_next_tag(const char *str, int tag) {
    Dialogue_line line = {
        .next_block_tag = tag
    };
    strcpy(line.str, str);
    
    return line;
}

typedef struct Dialogue_system {
    Dialogue *dialogue_vector;
    Dialogue *current_dialogue;
    bool is_dialogue_active;
} Dialogue_system;

Dialogue_system g_dialogue_system;

Dialogue *get_dialogue_by_name(const char *name) {
    for (int i = 0; i < get_sbuff_length(g_dialogue_system.dialogue_vector); ++i) {
        if (is_literal_string_equal(g_dialogue_system.dialogue_vector[i].name, name, standrad_name_length)) {
            return &g_dialogue_system.dialogue_vector[i];
        }
    }
    assert(0);
}

int get_dialogue_block_index_by_tag(Dialogue *dialogue, int tag) {
    for (int i = 0; i < get_sbuff_length(dialogue->dialogue_block_vector); ++i) {
        Dialogue_block dialogue_block = dialogue->dialogue_block_vector[i];
        if (dialogue_block.tag == tag)
            return i;
    }
    assert(0);
}

Dialogue *get_dialogue(Dialogue_id id) {
    return &g_dialogue_system.dialogue_vector[id.id];
}

void begin_dialogue(Dialogue_id dialogue_id) {
    g_dialogue_system.current_dialogue = get_dialogue(dialogue_id);
    g_dialogue_system.is_dialogue_active = true;
}

void end_dialogue() {
    g_dialogue_system.is_dialogue_active = false;
}

bool is_dialogue_active(Dialogue *dialogue) {
    return dialogue->active;
}

void reset_dialogue(Dialogue *dialogue) {
    dialogue->current_block = 0;
}

void move_to_next_dialogue_block() {
    Dialogue *dialogue = g_dialogue_system.current_dialogue;
    if (dialogue->current_block < get_sbuff_length(dialogue->dialogue_block_vector) - 1) {
        dialogue->current_block = dialogue->current_block + 1;
    }
    else {
        end_dialogue();
        reset_dialogue(dialogue);
    }
}

void move_to_next_dialogue_block_by_tag(Dialogue *dialogue, int tag) {
    dialogue->current_block = get_dialogue_block_index_by_tag(dialogue, tag);
}

Dialogue_block get_current_dialogue_block(Dialogue *dialogue) {
    Dialogue_block result;
    
    result = dialogue->dialogue_block_vector[dialogue->current_block];
    
    return result;
}

void run_dialogue(Game_info *game_info,  bool is_panel_clicked) {
    
    Dialogue *dialogue = g_dialogue_system.current_dialogue;
    
    float length = 0.1;
    float space = 0.01;
    float y0 = -0.8;
    
#if 0
    for (int i = 0; i < num_dialogue_lines; ++i) {
        v3 color;
        if (i == current_dialogue)
            color = get_v3(1, 1, 1);
        else
            color = get_v3(0.5, 0.5, 0.5);
    }
#endif
    
    v4 text_color = get_v4(1, 1, 1, 1);
    
    Dialogue_block dialogue_block = get_current_dialogue_block(dialogue);
    
    for (int i = 0; i < get_sbuff_length(dialogue_block.dialogue_line_vector); ++i) {
        Rect text_rect = {
            .x0 = -0.25,
            .y0 = y0 ,
            .x1 = 0.5,
            .y1 = y0 + length
        };
        y0 = y0 + length + space;
        Dialogue_line line = dialogue_block.dialogue_line_vector[i];
        if (dialogue_block.type == dialogue_block_regular) {
            if (is_panel_clicked) {
                if (line.next_block_tag > 0) {
                    move_to_next_dialogue_block_by_tag(dialogue, line.next_block_tag);
                }
                else {
                    move_to_next_dialogue_block();
                }
            }
            
            renderer_draw_text_to_screen(line.str, default_font, default_font_size, text_rect, text_color, 0, default_text_offset);
        }
        else if (dialogue_block.type == dialogue_block_choices){
            if(button_image_text(true, text_rect, line.str, get_v4(0, 0, 0, 0.5), text_color, renderer_get_texture_by_name("white_texture"), rect_alignment_left_center, default_text_offset)) {
                move_to_next_dialogue_block_by_tag(dialogue, line.next_block_tag);
            }
        }
    }
}

#if 0
Entity *get_entity_from_scene(Scene *scene, const char *name) {
    for (int i = 0; i < scene->num_entities; ++i) {
        if (is_literal_string_equal(scene->entities[i], name, standrad_name_length)) {
            return get_entity(scene->entities[i]);
        }
    }
    
    return null;
}
#endif

bool change_scene(Scene_id scene) {
    g_game_info.current_scene = scene;
}

typedef struct Fader {
    bool is_fading_out;
    bool is_fading_in;
    bool has_finished_fading;
    float alpha;
} Fader;

Fader g_fader;
Scene_id g_next_scene;

void begin_fading_out() {
    g_fader.is_fading_out = true;
}

float g_fade_rate = 0.3;

void fade_out() {
    if (g_fader.is_fading_out) {
        if (g_fader.alpha > 0.95) {
            g_fader.alpha = 1;
            g_fader.has_finished_fading = true;
        }
        else {
            g_fader.alpha = lerp(g_fader.alpha, 1, g_fade_rate);
        }
    }
}

void end_fading_out() {
    g_fader.is_fading_out = false;
    g_fader.has_finished_fading = false;
}

void begin_fading_in() {
    g_fader.is_fading_in = true;
}

void fade_in() {
    if (g_fader.is_fading_in) {
        if (g_fader.alpha < 0.05) {
            g_fader.alpha = 0;
            g_fader.has_finished_fading = true;
        }
        else {
            g_fader.alpha = lerp(g_fader.alpha, 0, g_fade_rate - 0.1);
        }
    }
}

void end_fading_in() {
    g_fader.is_fading_in = false;
    g_fader.has_finished_fading = false;
}

void draw_black_rect() {
    renderer_draw_rect_to_screen_with_color((Rect){-1, -1, 1, 1}, renderer_get_texture_by_name("white_texture"), get_v4(0, 0, 0, g_fader.alpha));
}

float g_player_side = 1;

void run_main_menu() {
    Game_info *game_info = &g_game_info;
    Rect_alignment alignment = rect_alignment_center;
    
    v4 text_color = get_v4(1, 1, 1, 1);
    
    Rect button_rect = create_rect(-0.95, -0.93, -0.3, -0.65);
    
    if(button_image_text(true, button_rect, "PLAY", get_v4(0, 0, 0, 0.5), text_color, renderer_get_texture_by_name("white_texture"), alignment, 0)) {
        game_info->game_state = game_state_playing;
    }
    
    button_rect = move_rect(button_rect, get_v2(0, 0.3));
    
    if(button_image_text(true, button_rect, "LOAD", get_v4(0, 0, 0, 0.5), text_color, renderer_get_texture_by_name("white_texture"), alignment, 0)) {
        
    }
    
    button_rect = move_rect(button_rect, get_v2(0, 0.3));
    
    if(button_image_text(true, button_rect, "QUIT", get_v4(0, 0, 0, 0.5), text_color, renderer_get_texture_by_name("white_texture"), alignment, 0)) {
        g_platform.quit = true;
    }
}

#pragma pack(push, 1)

typedef struct Save_file_header {
    u32 magic_value;
    u32 num_entities;
    u32 num_scenes;
    Scene_id current_scene;
} Save_file_header;

#pragma pack(pop)

typedef struct Game_info_save_data {
    Camera camera;
    Game_state game_state;
    const char *current_scene;
} Game_info_save_data;

#define max_num_scenes 10

typedef struct Scene_save_data {
    char name[standrad_name_length];
    char entities_names[max_num_scenes][standrad_name_length];
    int num_entities;
} Scene_save_data;

void destroy_all_entities_and_scenes() {
    g_game_info.current_scene = (Scene_id){0};
    if (g_game_info.entity_vector && g_game_info.scene_vector) {
        vector_free(g_game_info.entity_vector);
        vector_free(g_game_info.scene_vector);
        g_game_info.entity_vector =  null;
        g_game_info.scene_vector = null;
        vector_add(g_game_info.entity_vector, (Entity){0});
        vector_add(g_game_info.scene_vector, (Scene){0});
        g_game_info.player_entity = (Entity_id){0};
    }
}

Scene_save_data get_scene_save_data_from_scene(Scene scene) {
    u32 num_entities = vector_get_length(scene.entity_vector);
    Scene_save_data result = {
        .num_entities = num_entities
    };
    
    copy_string_to_buffer(scene.name, result.name, standrad_name_length);
    
    for (int i = 0; i < num_entities; ++i) {
        copy_string_to_buffer(get_entity_from_id(scene.entity_vector[i])->name, result.entities_names[i], standrad_name_length);
    }
    
    return result;
}

void create_scene_from_scene_save_data(Scene_save_data data) {
    
}

Entity *get_entity_by_name(const char *name) {
    if (!name)
        return &g_game_info.entity_vector[0];
    
    for (int i = 1; i < vector_get_length(g_game_info.entity_vector); ++i) {
        if (is_literal_string_equal(name, g_game_info.entity_vector[i].name, standrad_name_length))
            return &g_game_info.entity_vector[i];
    }
    
    return &g_game_info.entity_vector[0];
}

bool save_game(const char *save_file_name) {
    FILE *file = fopen(save_file_name, "wb+");
    if (!file)
        return false;
    
    u32 num_entities = vector_get_length(g_game_info.entity_vector) - 1;
    u32 num_scenes = vector_get_length(g_game_info.scene_vector) - 1;
    
    Save_file_header save_file_header = {
        .magic_value = save_file_magic_value,
        .num_entities = num_entities,
        .num_scenes = num_scenes,
        .current_scene = g_game_info.current_scene
    };
    
    Entity_save_data *entity_save_data_array = (Entity_save_data *)allocate_frame(sizeof(Entity_save_data) * num_entities);
    
    Scene_save_data *scene_save_data_array = (Scene_save_data *)allocate_frame(sizeof(Scene_save_data) * num_scenes);
    
    for (int i = 1; i < vector_get_length(g_game_info.entity_vector); ++i) {
        entity_save_data_array[i - 1] = get_entity_save_data_from_entity(g_game_info.entity_vector[i]);
    }
    
    for (int i = 1; i <  vector_get_length(g_game_info.scene_vector); ++i) {
        scene_save_data_array[i - 1] = get_scene_save_data_from_scene(g_game_info.scene_vector[i]);
    }
    
    fwrite(&save_file_header, 1, sizeof(Save_file_header), file);
    fwrite(entity_save_data_array, sizeof(Entity_save_data), num_entities, file);
    fwrite(scene_save_data_array, sizeof(Scene_save_data), num_scenes, file);
    
    fclose(file);
    
    return true;
}

bool load_game(const char *save_file_name) {
    FILE *file = fopen(save_file_name, "rb");
    if (!file)
        return false;
    
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (!length)
        return false;
    
    Save_file_header save_file_header = {0};
    fread(&save_file_header, 1, sizeof(Save_file_header), file);
    assert(save_file_header.magic_value == save_file_magic_value);
    
    Entity_save_data *entity_save_data_array = (Entity_save_data *)allocate_frame(sizeof(Entity_save_data) * save_file_header.num_entities);
    Scene_save_data *scene_save_data_array = (Scene_save_data *)allocate_frame(sizeof(Scene_save_data) * save_file_header.num_scenes);
    
    fread(entity_save_data_array, sizeof(Entity_save_data), save_file_header.num_entities, file);
    fread(scene_save_data_array, sizeof(Scene_save_data), save_file_header.num_scenes, file);
    
    destroy_all_entities_and_scenes();
    g_game_info.current_scene = save_file_header.current_scene;
    
    for (int i = 0; i < save_file_header.num_entities; ++i) {
        create_entity_from_entity_save_data(entity_save_data_array[i]);
    }
    
    for (int i = 0; i < save_file_header.num_scenes; ++i) {
        Scene_id scene_id = create_scene(scene_save_data_array[i].name);
        for (int j = 0; j < scene_save_data_array[i].num_entities; ++j) {
            add_entity_to_scene(scene_id, get_entity_by_name(scene_save_data_array[i].entities_names[j])->id);
        }
    }
    
    g_game_info.player_entity = get_entity_by_name("player")->id;
    
    return true;
}

Facing_direction get_entity_facing_direction(Entity *entity) {
    
    return entity->facing_direction;
}

Animation_type get_entity_animation_state(Entity *entity) {
    
    return entity->spine_animation.state;
}

Entity_id *get_scene_entities(Scene_id scene_id) {
    Entity_id *entities = get_scene(scene_id)->entity_vector;
    
    return entities;
}

bool do_entities_collide(Entity e0, Entity e1) {
    if (check_aabb_collision(get_bounding_box_world_coords(e0.bounding_box, e0.transform.transform_data->model),
                             get_bounding_box_world_coords(e1.bounding_box, e1.transform.transform_data->model))) {
        return true;
    }
    
    return false;
}

void begin_changing_scene(Scene_id next_scene) {
    begin_fading_out();
    g_next_scene = next_scene;
}

Dialogue_id create_dialogue(const char *name) {
    Dialogue dialogue = {
        .name = name
    };
    vector_add(g_dialogue_system.dialogue_vector, dialogue);
    
    Dialogue_id id = {
        .id = vector_get_length(g_dialogue_system.dialogue_vector) - 1
    };
    
    dialogue.dialogue_id = id;
    
    return id;
}

Dialogue_id get_dialogue_id_by_name(const char *name) {
    for (int i = 0; i < vector_get_length(g_dialogue_system.dialogue_vector); ++i) {
        if (is_literal_string_equal(name, g_dialogue_system.dialogue_vector[i].name, standrad_name_length)) {
            return g_dialogue_system.dialogue_vector[i].dialogue_id;
        }
    }
    
    assert(0);
}

void setup_dialogues() {
    Dialogue_id dialogue_id = create_dialogue("dialogue0");
    Dialogue *dialogue = get_dialogue(dialogue_id);
    
    Dialogue_line *dialogue_line_vector0 = null;
    put_in_sbuff(dialogue_line_vector0, create_dialogue_line("0"));
    
    Dialogue_line *dialogue_line_vector1 = null;
    put_in_sbuff(dialogue_line_vector1, create_dialogue_line_with_next_tag("1.0", 1));
    put_in_sbuff(dialogue_line_vector1, create_dialogue_line_with_next_tag("1.1", 2));
    
    Dialogue_line *dialogue_line_vector2 = null;
    put_in_sbuff(dialogue_line_vector2, create_dialogue_line_with_next_tag("2.1", 3));
    
    Dialogue_line *dialogue_line_vector3 = null;
    put_in_sbuff(dialogue_line_vector3, create_dialogue_line_with_next_tag("2.2", 3));
    
    add_dialogue_block(dialogue, create_dialogue_block(dialogue_line_vector0));
    add_dialogue_block(dialogue, create_dialogue_block_choices(dialogue_line_vector1));
    add_dialogue_block(dialogue, create_dialogue_block_with_tag(dialogue_line_vector2, 1));
    add_dialogue_block(dialogue, create_dialogue_block_with_tag(dialogue_line_vector3, 2));
    
    Dialogue_line *last_dialogue_line_vector = null;
    put_in_sbuff(last_dialogue_line_vector, create_dialogue_line("finished"));
    add_dialogue_block(dialogue, create_dialogue_block_with_tag(last_dialogue_line_vector, 3));
}

void init_entities_and_scenes() {
    Scene_id scene0 = create_scene("scene0");
    Scene_id scene1 = create_scene("scene1");
    
#if 0
    Entity_id background_entity = create_entity("background",
                                                renderer_create_mesh_from_rect((Rect){-0.3, 0 ,0.3 ,0.23}),
                                                (v3){0, 0, default_z_depth}, (v3){1, 1, 1}, renderer_get_texture_by_name("background_texture"), entity_type_static, layer_background, (Collider){0});
#endif
    Entity_id door_entity = create_entity("door",
                                          renderer_create_mesh_from_rect((Rect){0, 0, 0.1, 0.06}, perm_alloc),
                                          (v3){0, 0, default_z_depth}, (v3){1, 1, 1},
                                          renderer_get_texture_by_name("door_texture"),
                                          entity_type_static,
                                          layer_items,
                                          (Collider){.is_collidable = true,
                                              .collision_effect_type = collision_effect_type_change_scene,
                                              .effect.next_scene = scene1});
    
    //set_entity_pos(get_entity(door_entity), get_v3(0, 0, default_z_depth));
    
    Entity_id background_2_entity = create_entity("background_2", renderer_create_mesh_from_rect((Rect){-0.3, 0 ,0.3 ,0.23}, perm_alloc), (v3){0, -0.5, default_z_depth}, (v3){1, 1, 1}, renderer_get_texture_by_name("background_2_texture"), entity_type_static, layer_background, (Collider){0});
    //scale_entity(get_entity(background_2_entity), get_v3(0.003, 0.003, 0.003));
    //move_entity(get_entity(background_2_entity), (v3){ 1000, 500, 0});
    
    Spine_animation_data spine_animation0 = create_spine_animation(&g_assets.spine_assets);
    g_game_info.player_entity = create_animated_entity("player", (v3){0, 0, default_z_depth}, (v3){0.00009, 0.00009, 1}, spine_animation0, renderer_get_texture_by_name("spine_texture"), entity_type_player, layer_player, (Collider){.is_collidable = true});
    Entity *player_ptr = get_entity(g_game_info.player_entity);
    player_ptr->collider.is_collidable = true;
    //scale_entity(get_entity(g_game_info.player_entity), get_v3(0.00009, 0.00009, 0));
    //move_entity(get_entity(g_game_info.player_entity), (v3){0, 200, 0});
    bool flip_skeleton = false;
    
    Spine_animation_data spine_animation1 = create_spine_animation(&g_assets.spine_assets);
    Entity_id npc0_entity = create_animated_entity("npc0", (v3){-500, 0, default_z_depth}, (v3){0.00009, 0.00009, 1}, spine_animation1, renderer_get_texture_by_name("spine_texture"), entity_type_npc, layer_characters, (Collider){0});
    Entity *npc0 = get_entity(npc0_entity);
    //npc0->entity_type = entity_type_npc;
    npc0->clicker = (Clicker) {
        .is_clickable = true,
        .click_effect = click_effect_start_dialogue
    };
    npc0->dialoguer = (Dialoguer) {
        .can_start_dialogue = true,
        .dialogue_id = get_dialogue_id_by_name("dialogue0")
    };
    
    //add_entity_to_scene(get_scene(&g_scene_holder, "scene_0"), "npc0");
    
    Entity_id scene0_entities[] = { npc0_entity, g_game_info.player_entity, door_entity };
    //Entity_id scene0_entities[] = { door_entity };
    add_entities_to_scene(scene0_entities, array_count(scene0_entities), scene0);
    
    Entity_id scene1_entities[] = { background_2_entity, g_game_info.player_entity };
    add_entities_to_scene(scene1_entities, array_count(scene1_entities), scene1);
    
    g_game_info.current_scene = scene0;
}

Bbox get_entity_screen_space_bbox(Entity entity) {
    //TODO: THE COORDINATES ARE REVEREED. FIX ASAP
    v3 v0 =  project(*entity.transform.transform_data, get_v3(entity.bounding_box.x0, entity.bounding_box.y0, 0));
    v3 v1 =  project(*entity.transform.transform_data, get_v3(entity.bounding_box.x1, entity.bounding_box.y1, 0));
    
    //Bounding_box bounding_box = get_bounding_box(v0.x, v1.y, v1.x, v0.y);
    Bounding_box bounding_box = get_bounding_box(v0.x, v0.y, v1.x, v1.y);
    
    return bounding_box;
}

bool is_entity_clicked(Entity entity, Input *input) {
    Bounding_box bounding_box = get_entity_screen_space_bbox(entity);
    bool is_inside = is_point_inside_bounding_box(bounding_box, input->mouse_pos);
    bool is_clicked = g_game_info.input_actions[input_action_left_button_clicked];
    bool is_mouse_up = g_game_info.input_actions[input_action_left_button_released];
    
    //print_bbox(bounding_box);
    //print_v2(input->mouse_pos);
    //fprintf(stderr, "\r");
    
    if (is_inside && is_clicked) {
        return true;
    }
    else if (is_mouse_up) {
        
    }
    
    return false;
}

m4 get_model_matrix_from_entity(Entity entity) {
    m4 result = mul_m4_by_m4(math_get_translation_matrix(entity.pos), math_get_scale_matrix(entity.scale));
    
    return result;
}


void null_terminate_string_128(String_128 *str) {
    str->str[str->len] = 0;
}

void display_command_line() {
    null_terminate_string_128(&g_game_info.editor.command_text);
    output_to_debug_view(g_game_info.editor.command_text.str);
    output_to_debug_view("\n");
}

char convert_key_to_ascii(Key key) {
    return key - 1;
}

void stop_reading_command() {
    g_game_info.editor.is_entering_command = false;
    g_game_info.editor.command_text.len = 0;
}

void read_command() {
    //if (!g_platform.input.current_clicked_key)
    //return;
    char c = g_platform.input.current_clicked_key;
    if (c >= 32 && c <= 126) {
        if (g_game_info.editor.command_text.len < 128)
            g_game_info.editor.command_text.str[g_game_info.editor.command_text.len++] = c; 
    }
    else if (g_platform.input.keys[key_enter].down) {
        
    }
    else if (g_platform.input.keys[key_backspace].down) {
        if (g_game_info.editor.command_text.len > 0)
            g_game_info.editor.command_text.len--;
    }
    
}

bool is_entity_equal(Entity_id a, Entity_id b) {
    if (a.id == b.id)
        return true;
    return false;
}

bool is_entity_in_scene(Entity_id entity_id) {
    
    Entity_id *entities = get_scene(g_game_info.current_scene)->entity_vector;
    for (int i = 0; i < vector_get_length(entities); ++i) {
        if (is_entity_equal(entity_id, entities[i])) 
            return true;
    }
    
    return false;
    
}

void draw_entity_bounding_box_outline(Entity_id entity_id) {
    if (!entity_id.id || !is_entity_in_scene(entity_id)) {
        return;
    }
    Entity *entity = get_entity(entity_id);
    v4 color = get_v4(1, 1, 1, 1);
    Bounding_box bounding_box = entity->bounding_box;
    int weight = 7;
    Rect rect0 = (Rect){ bounding_box.min_x, bounding_box.min_y, bounding_box.max_x, bounding_box.min_y + weight };
    Rect rect1 = (Rect){ bounding_box.min_x, bounding_box.min_y, bounding_box.min_x + weight, bounding_box.max_y };
    Rect rect2 = (Rect){ bounding_box.min_x, bounding_box.max_y - weight, bounding_box.max_x, bounding_box.max_y };
    Rect rect3 = (Rect){ bounding_box.max_x - weight, bounding_box.min_y, bounding_box.max_x, bounding_box.max_y };
    
    renderer_draw_outline_with_color(get_rect_from_bbox(entity->bounding_box), *entity->transform.transform_data,  color, 7, entity->layer);
}

v3 get_vector_from_screen_coords_to_world_space(v2 pos) {
    m4 view = calculate_view_matrix_from_camera(g_game_info.camera);
    m4 projection = get_projection_matrix_from_camera(g_game_info.camera);
    v3 ray = raycast(pos, view, projection);
    
    return ray;
}

v3 get_pos_in_world_coords_from_viewport(v2 pos) {
    v3 vector = get_vector_from_screen_coords_to_world_space(pos);
    v3 lerped_pos = lerp_v3(g_game_info.camera.pos, vector, (-g_game_info.camera.pos.z + default_z_depth) / vector.z);
    
    return lerped_pos;
}

v3 get_pos_in_world_coords_from_ndc(v2 pos) {
    convert_from_ndc_to_screen_coords(&pos.x, &pos.y, g_screen_width, g_screen_height);
    v2 pos_in_viewport_coords = get_v2(pos.x, pos.y);
    
    v3 result = get_pos_in_world_coords_from_viewport(pos_in_viewport_coords);
    
    return result;
}


void update_entities() {
    Bbox player_limits = { -5, -5, 5, 5 };
    
    Scene_id scene_id = g_game_info.current_scene;
    Entity_id *entity_vector = get_scene_entities(scene_id);
    int num_entities = vector_get_length(entity_vector);
    
    for (int i = 0 ; i < num_entities; ++i) {
        Entity *e0 = get_entity(entity_vector[i]);
        Entity *entity = e0;
        if (!entity->active) {
            continue;
        }
        
#if 0
        m4 translation = math_get_translation_matrix(entity->pos);
        entity->transform.transform_data->model = translation;
#endif
        
        const float scale_factor = 0.991;
        
#if 1
        if (entity->type == entity_type_player) {
            Entity *player_entity = entity;
            Facing_direction player_facing_direction = get_entity_facing_direction(player_entity);
            Animation_type player_animation_state = get_entity_animation_state(player_entity);
            bool is_walking = false;
            if (g_game_info.input_actions[input_action_move_player_right]) {
                if ( player_facing_direction == facing_left_direction) {
                    change_entity_facing_direction(player_entity, facing_right_direction);
                }
                if ( player_animation_state == idle_animation) {
                    set_entity_animation(player_entity, walk_animation);
                }
                //move_entity_within_bbox(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0), player_limits);
                //move_entity(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0));
                move_entity(player_entity, get_v3(10, 0, 0));
                is_walking = true;
            }
            if (g_game_info.input_actions[input_action_move_player_left]) {
                if ( player_facing_direction == facing_right_direction) {
                    //change_entity_facing_direction(player_entity, facing_left_direction);
                }
                if ( player_animation_state == idle_animation) {
#if 0
                    spAnimationState_setAnimationByName(player_entity->spine_animation.data.animation_state, 0, "walk", 1);
                    player_entity->spine_animation.state = walk_animation;
#else
                    set_entity_animation(player_entity, walk_animation);
#endif
                    
                }
                //move_entity_within_bbox(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0), player_limits);
                //move_entity(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0));
                move_entity(player_entity, get_v3(-10, 0, 0));
                is_walking = true;
            }
            if (g_game_info.input_actions[input_action_move_player_up]) {
                if ( player_facing_direction == facing_right_direction) {
                    //change_entity_facing_direction(player_entity, facing_left_direction);
                }
                if ( player_animation_state == idle_animation) {
#if 0
                    spAnimationState_setAnimationByName(player_entity->spine_animation.data.animation_state, 0, "walk", 1);
                    player_entity->spine_animation.state = walk_animation;
#else
                    set_entity_animation(player_entity, walk_animation);
#endif
                }
                //move_entity_within_bbox(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0), player_limits);
                //move_entity(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0));
                //move_entity(player_entity, get_v3(0, 10, 0));
                //scale_entity(player_entity, get_v3(scale_factor, scale_factor, 0));
                //scale_entity(player_entity, convert_vector_from_model_to_world_space(get_entity_model_matrix(*entity), get_v3(scale_factor, scale_factor, 0)));
                
                //scale_entity(player_entity, div_v3(get_v3(0.1, 0.1, 0), player_entity->scale));
                move_entity(player_entity, get_v3(0, 10, 0));
                //scale_entity(player_entity, get_v3(scale_factor, scale_factor, 0));
                //move_entity(player_entity, mul_v3_hadmard(get_v3(0, 10, 0), player_entity->scale));
                is_walking = true;
            }
            if (g_game_info.input_actions[input_action_move_player_down]) {
                if ( player_facing_direction == facing_right_direction) {
                    //change_entity_facing_direction(player_entity, facing_left_direction);
                }
                if ( player_animation_state == idle_animation) {
#if 0
                    spAnimationState_setAnimationByName(player_entity->spine_animation.data.animation_state, 0, "walk", 1);
                    player_entity->spine_animation.state = walk_animation;
#else
                    set_entity_animation(player_entity, walk_animation);
#endif
                }
                //move_entity_within_bbox(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0), player_limits);
                //move_entity(player_entity, get_v3(1000 * g_platform.delta_time_in_seconds, 0, 0));
                //move_entity(player_entity, get_v3(0, -10, 0));
                //scale_entity(player_entity, get_v3(1 / scale_factor, 1 / scale_factor, 0));
                //scale_entity(player_entity, convert_vector_from_model_to_world_space(get_entity_model_matrix(*entity), get_v3(1 / scale_factor, 1 / scale_factor, 0)));
                //move_entity(player_entity, get_v3(0, 0, 0));
                //scale_entity(player_entity, div_v3(get_v3(1, 1, 0), player_entity->scale));
                //scale_entity(player_entity, get_v3(1 / scale_factor, 1 / scale_factor, 0));
                //move_entity(player_entity, mul_v3_hadmard(get_v3(0, -10, 0), player_entity->scale));
                move_entity(player_entity, (v3){.y = -10});
                
                is_walking = true;
            }
            if (!is_walking){
                if ( player_animation_state == walk_animation) {
                    set_entity_animation(player_entity, idle_animation);
                }
            }
            
            //check for collisions
            for (int j = 0 ; j < num_entities; ++j) {
                if (i == j)
                    continue;
                Entity *e1 = get_entity(entity_vector[j]);
                if (e0->collider.is_collidable && e1->collider.is_collidable) {
                    if (do_entities_collide(*e0, *e1)) {
                        if (e0->type == entity_type_player) {
                            if (e1->collider.collision_effect_type == collision_effect_type_change_scene) {
                                if (g_platform.input.keys[key_up_arrow].down) {
                                    begin_changing_scene(e1->collider.effect.next_scene);
                                }
                            }
                        }
                    }
                }
            }
        }
#endif
        
        //check if entity clicked
        if (entity->clicker.is_clickable) {
            if (is_entity_clicked(*entity, &g_platform.input)) {
                if (!g_game_info.is_in_edit_mode) {
                    if (entity->clicker.click_effect == click_effect_start_dialogue) {
                        assert(entity->dialoguer.can_start_dialogue);
                        begin_dialogue(entity->dialoguer.dialogue_id);
                    }
                }
            }
        }
        
        //check if entity is being dragged
        if (g_game_info.is_in_edit_mode) {
            Bbox bbox = get_entity_bbox(entity);
            Rect rect = get_rect(bbox.x0, bbox.y0, bbox.x1, bbox.y1);
            float distance = 0;
#if 0
            v2 mouse_pos = get_mouse_position();
            m4 view = calculate_view_matrix_from_camera(g_game_info.camera);
            m4 projection = get_projection_matrix_from_camera(g_game_info.camera);
            v3 ray = raycast(mouse_pos, view, projection);
            v3 previous_ray = raycast(g_platform.input.previous_mouse_pos, view, projection);
            v3 lerped_mouse_pos = lerp_v3(g_game_info.camera.pos, ray, (-g_game_info.camera.pos.z + default_z_depth) / ray.z);
            v3 previous_lerped_mouse_pos = lerp_v3(g_game_info.camera.pos, previous_ray, (-g_game_info.camera.pos.z + default_z_depth) / previous_ray.z);
#else
            v3 lerped_mouse_pos = get_pos_in_world_coords_from_viewport(get_mouse_position());
            v3 previous_lerped_mouse_pos = get_pos_in_world_coords_from_viewport(g_platform.input.previous_mouse_pos);
#endif
            if (entity->is_being_dragged) {
                if (g_game_info.input_actions[input_action_left_button_pressed]) {
                    v3 bbox_center = get_center_of_bbox(entity->bounding_box);
                    //bbox_center.z = default_z_depth;
                    v3 entity_world_coords_center = get_v3_from_v4(mul_m4_by_v4(entity->transform.transform_data->model, get_v4(bbox_center.x, bbox_center.y, bbox_center.z, 1.0f)));
                    
                    //v3 offset = sub_v3(lerped_mouse_pos, entity_world_coords_center);
                    v3 offset = sub_v3(lerped_mouse_pos, previous_lerped_mouse_pos);
                    //move_entity(entity, mul_v3_hadmard(offset, get_v3(1/entity->scale.x, 1/entity->scale.y, 1/entity->scale.z)));
                    //output_to_debug_view("mouse_pos: (%f %f %f)\n", lerped_mouse_pos.x, lerped_mouse_pos.y, lerped_mouse_pos.z);
                    //v3 offset = 
                    //move_entity(entity, offset);
                    
                }
                else if (g_game_info.input_actions[input_action_left_button_released]) {
                    entity->is_being_dragged = false;
                }
            }
        }
        
        //draw outline
        if (g_game_info.is_in_edit_mode && (entity->type == entity_type_player || entity->type == entity_type_npc)) {
            draw_entity_bounding_box_outline(entity_vector[i]);
        }
    }
    
    //set entity to be dragged
    for (int i = num_entities - 1 ; i >= 0 ; --i) {
        Entity *entity = get_entity(entity_vector[i]);
        Bbox bbox = get_entity_bbox(entity);
        Rect rect = get_rect(bbox.x0, bbox.y0, bbox.x1, bbox.y1);
        float distance = 0;
#if 0
        v2 mouse_pos = get_mouse_position();
        m4 view = calculate_view_matrix_from_camera(g_game_info.camera);
        m4 projection = get_projection_matrix_from_camera(g_game_info.camera);
        v3 ray = raycast(mouse_pos, view, projection);
#else
        v3 ray = get_vector_from_screen_coords_to_world_space(get_mouse_position());
#endif
        if (check_collision_between_vector_and_plane(ray, rect, default_z_depth, g_game_info.camera.pos, &distance) && g_platform.input.mouse_buttons[left_mouse_button].down) {
            if (!entity->is_being_dragged) {
                entity->is_being_dragged = true;
            }
            break;
        }
    }
    
}

void draw_debug_string() {
    Rect text_rect = {-1, 0.90, 1, 1};
    renderer_draw_text_to_screen(g_game_info.debug_string, default_font, default_font_size, text_rect, get_v4(1, 1, 1, 1), rect_alignment_left_top, default_text_offset);
}

void move_entity_within_bbox(Entity *entity, v3 amount, Bbox bbox) {
    m4 mat = math_translate(entity->transform.transform_data->model, amount);
    Bbox world_coords_bbox = get_bounding_box_world_coords(entity->bounding_box, mat);
    bool is_inside =
        world_coords_bbox.min_x >= bbox.min_x &&
        world_coords_bbox.min_y >= bbox.min_y &&
        world_coords_bbox.max_x <= bbox.max_x &&
        world_coords_bbox.max_y <= bbox.max_y;
    
    if (is_inside) {
        entity->transform.transform_data->model = mat;
    }
}

void handle_fading() {
    draw_black_rect();
    
    if (g_fader.is_fading_out && g_fader.has_finished_fading) {
        change_scene(g_next_scene);
        end_fading_out();
        begin_fading_in();
    }
    else if(g_fader.is_fading_in && g_fader.has_finished_fading) {
        end_fading_in();
    }
    
    if (g_fader.is_fading_out)
        fade_out();
    if (g_fader.is_fading_in)
        fade_in();
    
}

Bbox unproject_bbox(Transform_data transform_data, Bbox bbox) {
    Bbox result  = {
        .v0 = unproject(transform_data, bbox.v0),
        .v1 = unproject(transform_data, bbox.v1)
    };
    
    return result;
}

Bbox mul_bbox_by_matrix(m4 m, Bbox bbox) {
    Bbox result  = {
        .v0 = get_v3_from_v4(mul_m4_by_v4(m, get_v4(bbox.x0, bbox.y0, bbox.z0, 1))),
        .v1 = get_v3_from_v4(mul_m4_by_v4(m, get_v4(bbox.x1, bbox.y1, bbox.z1, 1)))
    };
    
    return result;
}

void draw_camera_rect(Camera camera) {
    float weight = 0.001;
    v3 camera_pos = camera_default_pos;
    //v3 camera_pos = g_game_info.camera.pos;
    m4 default_view = math_lookat(camera_default_pos, add_v3(camera_default_pos, camera_default_front), camera_default_up);
    //m4 default_view = calculate_view_matrix_from_camera(g_game_info.camera);
    m4 projection = get_projection_matrix_from_camera(camera);
    v3 ray0 = raycast((v2){0, 0}, default_view, projection);
    v3 ray1 = raycast((v2){g_screen_width, g_screen_height}, default_view, projection);
    v3 left_bottom_corner = lerp_v3( camera_pos, ray0, (-camera_pos.z + default_z_depth) / ray0.z);
    v3 right_top_corner = lerp_v3( camera_pos, ray1, (-camera_pos.z + default_z_depth) / ray1.z);
    
    m4 view = calculate_view_matrix_from_camera(camera);
    m4 model = math_get_identity_matrix();
    model = math_translate(model, get_v3(0, 0, default_z_depth));
    renderer_draw_outline_with_color((Rect){ left_bottom_corner.x, left_bottom_corner.y, right_top_corner.x, right_top_corner.y },
                                     (Transform_data){ .model = model, .view = view, .projection = projection },
                                     default_color, weight, layer_screen);
}

void handle_input() {
    memset(g_game_info.input_actions, 0, sizeof(bool) * input_action_num);
    
    if (g_platform.input.keys[key_1].down) {
        g_game_info.input_actions[input_action_free_camera] = true;
    }
    if (g_platform.input.keys[key_2].down) {
        g_game_info.input_actions[input_action_focus_camera_on_player] = true;
    }
    
    if (g_platform.input.keys[key_left_control].pressed) {
        if (g_platform.input.keys[key_s].down) {
            g_game_info.input_actions[input_action_save] = true;
        }
        else if (g_platform.input.keys[key_l].down) {
            g_game_info.input_actions[input_action_load] = true;
        }
        else if (g_platform.input.keys[key_d].down) {
            g_game_info.input_actions[input_action_destory_all_entities] = true;
        }
        else if (g_platform.input.keys[key_e].down) {
            g_game_info.input_actions[input_action_edit_mode] = true;
        }
        else if (g_platform.input.keys[key_g].down) {
            g_game_info.input_actions[input_action_game_mode] = true;
        }
        else if (g_platform.input.keys[key_c].down) {
            g_game_info.input_actions[input_action_enter_command] = true;
        }
    }
    else {
        if (g_platform.input.keys[key_w].pressed) {
            g_game_info.input_actions[input_action_move_camera_up] = true;
        }
        else if (g_platform.input.keys[key_s].pressed) {
            g_game_info.input_actions[input_action_move_camera_down] = true;
        }
        else if (g_platform.input.keys[key_a].pressed) {
            g_game_info.input_actions[input_action_move_camera_left] = true;
        }
        else if (g_platform.input.keys[key_d].pressed) {
            g_game_info.input_actions[input_action_move_camera_right] = true;
        }
        else if (g_platform.input.keys[key_r].pressed) {
            g_game_info.input_actions[input_action_move_camera_in] = true;
        }
        else if (g_platform.input.keys[key_f].pressed) {
            g_game_info.input_actions[input_action_move_camera_out] = true;
        }
    }
    
    if (g_platform.input.keys[key_right_arrow].pressed) {
        g_game_info.input_actions[input_action_move_player_right] = true;
    }
    if (g_platform.input.keys[key_left_arrow].pressed) {
        g_game_info.input_actions[input_action_move_player_left] = true;
    }
    if (g_platform.input.keys[key_up_arrow].pressed) {
        g_game_info.input_actions[input_action_move_player_up] = true;
    }
    if (g_platform.input.keys[key_down_arrow].pressed) {
        g_game_info.input_actions[input_action_move_player_down] = true;
    }
    
    if (g_platform.input.mouse_buttons[left_mouse_button].down) {
        g_game_info.input_actions[input_action_left_button_clicked] = true;
    }
    if (g_platform.input.mouse_buttons[left_mouse_button].pressed) {
        g_game_info.input_actions[input_action_left_button_pressed] = true;
    }
    if (g_platform.input.mouse_buttons[left_mouse_button].released) {
        g_game_info.input_actions[input_action_left_button_released] = true;
    }
    if (g_platform.input.mouse_buttons[right_mouse_button].down) {
        g_game_info.input_actions[input_action_right_button_clicked] = true;
    }
    
    if (g_platform.input.keys[key_escape].down) {
        g_game_info.input_actions[input_action_exit] = true;
    }
    
}

void game_set_current_scene_background(Texture_id texture) {
    
    Rect rect = { 1, -0.4, -1, 1 };
    renderer_draw_rect_to_screen_with_color_and_layer(rect, renderer_get_texture_by_name("background_texture"), get_v4(1, 1, 1, 1), layer_background);
}

typedef struct Polygon {
    v2 *coords;
    u32 num_coords;
} Polygon;

bool is_even(u32 num) {
    if (num % 2 == 0)
        return true;
    return false;
}

typedef struct Line {
    v2 a, b;
} Line;

bool is_point_on_line(Line line, v2 p) {
    v2 ab = sub_v2(line.b, line.a);
    v2 ac = sub_v2(p, line.a);
    v3 v = cross_v3(get_v3_from_v2(ac), get_v3_from_v2(ab));
    
    //TODO: this function is completely fucked up, something to do with floating points
    // add a single zero to the 0.000...1 number and see how it fucks ups
    
    if (v.x <= 0.000001 && v.y <= 0.000001 && v.z <= 0.000001) {
        float k_ac = dot_v2(ab, ac);
        float k_ab = dot_v2(ab, ab);
        if (k_ac >= 0 && (k_ac  - k_ab) < 0.000001) 
            return true;
    }
    
    return false;
}

bool is_intersecting(Line line_a, Line line_b) {
    float x1 = line_a.a.x;
    float x2 = line_a.b.x;
    float x3 = line_b.a.x;
    float x4 = line_b.b.x;
    float y1 = line_a.a.y;
    float y2 = line_a.b.y;
    float y3 = line_b.a.y;
    float y4 = line_b.b.y;
    
    float denominator  = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    float x_nominator = (x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4);
    float y_nominator = (x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4);
    
    float p_x = x_nominator / denominator;
    float p_y = y_nominator / denominator;
    
    v2 p = get_v2(p_x, p_y);
    
    if (is_point_on_line(line_a, p) && is_point_on_line(line_b, p))
        return true;
    
    return false;
}


bool is_point_inside_polygon(Polygon polygon, v2 p) {
    float minimum_x = polygon.coords[0].x;
    for (int i = 0; i < polygon.num_coords; ++i) {
        if (polygon.coords[i].x < minimum_x)
            minimum_x = polygon.coords[i].x;
    }
    
    u32 counter = 0;
    for (int i = 0; i < polygon.num_coords; ++i) {
        v2 min_p = (v2){ minimum_x - 1, p.y };
        Line line0 = (Line){ min_p, p };
        Line line1 = (Line){ polygon.coords[i % polygon.num_coords], polygon.coords[(i + 1) % polygon.num_coords] };
        if (is_intersecting(line0, line1))
            counter++;
    }
    
    if (is_even(counter)) {
        return true;
    }
    
    return false;
}

#if 0
Rect create_rect_from_polygon(Polygon polygon) {
    for (int i = 0; i < polygon.num_coords; ++i) {
    }
}

Grid create_grid_from_polygon(Polygon polygon, Allocation_type allocation_type) {
    
    u32 width = 0.01, height = 0.01;
    
    Rect rect = create_rect_from_polygon(polygon);
    
    u32 num_coords = get_rect_area(rect) / (width * height);
    
    Grid grid = {
        .num_coords = num_coords,
        .coords = allocate(sizeof(v2) * num_coords, allocation_type)
    };
    
    for (int i = 0; i < num_coords; ++i) {
        grid.coords[i] = 
    }
    
}
#endif

void move_entity_to_pos(Entity *entity, v2 pos, float speed) {
    //todo
}


#if 0
void move_entity_to_point_in_grid(Entity *entity, Grid grid, v2 point) {
    move_entity(entity, sub_v2(point, entity->pos));
    for (int i = 0; i < ; ++i) {
        
    }
}
#endif

Path create_path(u32 num_nodes, v2 nodes[]) {
    Path path = {
        .num_nodes = num_nodes,
        .nodes = nodes
    };
    
    return path;
}

Path create_path_in_grid(Grid grid, v2 start, v2 end, Allocation_type allocation_type) {
    assert(start.x <= end.x);
    assert(start.y <= end.y);
#if 0
    u32 num_nodes = abs(start.x - end.x + 1) + abs(start.y - end.y + 1);
    v2 *nodes = (v2 *)allocate(sizeof(v2) * num_nodes, allocation_type);
    for (int i  = 0; i < abs(start.x - end.x + 1); ++i) {
        int index = i;
        assert(index < num_nodes);
        nodes[index] = get_v2(start.x + i, start.y);
    }
    for (int i = 0; i < abs(start.y - end.y + 1); ++i) {
        int index = i + abs(start.x - end.x + 1);
        assert(index < num_nodes);
        nodes[index] = get_v2(end.x, start.y + i);
    }
#else
    
    u32 num_nodes = (end.x - start.x + 1) + (end.y - start.y);
    int counter = 0;
    v2 *nodes = (v2 *)allocate(sizeof(*nodes) * num_nodes, allocation_type);
    
    for (int x = start.x; x <= end.x; ++x) {
        assert(num_nodes >= counter);
        nodes[counter++] = get_v2(x, start.y);
    }
    for (int y = start.y + 1; y <= end.y; ++y) {
        assert(num_nodes >= counter);
        nodes[counter++] = get_v2(end.x, y);
    }
    
    assert(num_nodes == counter);
    
#endif
    
    Path path = create_path(num_nodes, nodes);
    
    return path;
}

v2 get_pos_from_grid_index(Grid grid, v2 index) {
    v2 start_pos = { grid.rect.x0 + grid.tile_width / 2 , grid.rect.y0 + grid.tile_height / 2};
    v2 pos = (v2){ start_pos.x + index.x * grid.tile_width,  start_pos.y + index.y * grid.tile_height};
    
    return pos;
}

void move_entity_towards(Entity *entity, v3 target, float speed) {
    //printf("moving %f to %f\n", entity->pos.x, target.x);
    entity->pos = lerp_v3(entity->pos, target, 0.1);
    //entity->pos = target;
    //entity->pos = add_v3(entity->pos, 
}

v2 get_grid_point_from_pos(Grid grid, v2 pos) {
    int x = (int)((pos.x - grid.rect.x0) / grid.tile_width);
    int y = (int)((pos.y - grid.rect.y0) / grid.tile_height);
    
    x += grid.tile_width / 2;
    y += grid.tile_height / 2;
    
    v2 result = get_v2(x, y);
    
    return result;
}

bool move_entity_on_path(Entity *entity) {
    float speed = 1;
    if (entity->path_mover.current_node < g_game_info.path.num_nodes) {
        v2 current_goal = get_pos_from_grid_index(g_game_info.grid, g_game_info.path.nodes[entity->path_mover.current_node]);
        v3 current_goal_in_world_coords = get_pos_in_world_coords_from_ndc(current_goal);
        if (is_v3_equal(entity->pos, current_goal_in_world_coords)) {
            entity->path_mover.current_node++;
        }
        v2 goal = get_pos_from_grid_index(g_game_info.grid, g_game_info.path.nodes[entity->path_mover.current_node]);
        v3 goal_in_world_coords = get_pos_in_world_coords_from_ndc(goal);
        move_entity_towards(entity, goal_in_world_coords, speed);
        
        return true;
    }
    
    return false;
}

Grid create_grid(Rect rect, float tile_width, float tile_height, Allocation_type allocator_type) {
    if (tile_width == 0 || tile_height == 0) {
        return (Grid){0};
    }
    if ((rect.x1 - rect.x0) / tile_width != 0) {
        //rect.x1 += tile_width - ((rect.x1 - rect.x0) / tile_width);
    }
    if ((rect.y1 - rect.y0) / tile_height != 0) {
        //rect.y1 += tile_height - ((rect.y1 - rect.y0) / tile_height);
    }
    Grid grid = {
        .rect = rect,
        .tile_width = tile_width,
        .tile_height = tile_height,
        .num_columns = (u32)((rect.x1 - rect.x0) / tile_width),
        .num_rows = (u32)((rect.y1 - rect.y0) / tile_height),
        .num_cells =  (u32)((rect.x1 - rect.x0) / tile_width) * (u32)((rect.y1 - rect.y0) / tile_height)
    };
    grid.obstacles = (bool *)allocate_and_zero_out(sizeof(bool) * grid.num_columns * grid.num_rows, allocator_type);
    
    return grid;
}

float get_a_star_heuristic(v2 node, v2 goal) {
    float result = sqrt(pow(goal.x - node.x, 2) + pow(goal.y - node.y, 2));
    
    return result;
}

u32 get_index_from_v2(v2 p, Grid grid) {
    if (p.x >= grid.num_columns || p.y >= grid.num_rows || p.x < 0 || p.y < 0) {
        return grid.num_columns * grid.num_rows;
    }
    u32 result = p.x + p.y * grid.num_columns;
}

bool check_if_array_is_zero(bool array[], u32 num_elements) {
    for (int i = 0; i < num_elements; ++i) {
        if (array[i] != 0)
            return false;
    }
    
    return true;
}

v2 get_v2_from_index(u32 index, Grid grid) {
    if (index < 0 || index > grid.num_columns * grid.num_rows) {
        return get_v2(-1, -1);
    }
    float x = index % grid.num_columns;
    float y = index / grid.num_columns;
    v2 result = (v2){ x, y };
    
    return result;
}

Path get_a_star_path(Grid grid, v2 start, v2 end, Allocation_type allocation_type) {
    
    u32 start_node = get_index_from_v2(start, grid);
    u32 end_node = get_index_from_v2(end, grid);
    
    if (grid.obstacles[start_node] || grid.obstacles[end_node]) {
        return (Path){0};
    }
    
    u32 num_nodes = grid.num_columns * grid.num_rows;
    
    bool *open_set = allocate_and_zero_out(num_nodes * sizeof(*open_set), frame_alloc);
    open_set[start_node] = true;
    
    float *g_score = allocate(num_nodes * sizeof(*g_score), frame_alloc);
    for (int i = 0; i < num_nodes; ++i) {
        g_score[i] = FLT_MAX;
    }
    g_score[start_node] = 0;
    
    float *f_score = allocate(num_nodes * sizeof(*f_score), frame_alloc);
    f_score[start_node] = get_a_star_heuristic(start, end);
    
    float *came_from = allocate(num_nodes * sizeof(*f_score), frame_alloc);
    
    while (!check_if_array_is_zero(open_set, num_nodes)) {
        //get min f score
        u32 min_node;
        for (int i = 0 ; i < num_nodes; ++i) {
            if (open_set[i]) {
                min_node = i;
            }
        }
        for (int i = 0; i < num_nodes; ++i) {
            if (open_set[i]) {
                if (f_score[i] < f_score[min_node]) {
                    min_node = i;
                }
            }
        }
        
        assert(!grid.obstacles[min_node]);
        
        //get path
        if (min_node == end_node) {
            int num_nodes_in_path = 1;
            int node = min_node;
            while (node != start_node) {
                node = came_from[node];
                num_nodes_in_path++;
            }
            
            Path path = {
                .nodes = allocate(sizeof(*path.nodes) * num_nodes_in_path, allocation_type),
                .num_nodes = num_nodes_in_path
            };
            
            node = min_node;
            path.nodes[0] = start;
            for (int i = num_nodes_in_path - 1; i > 0; --i) {
                path.nodes[i] = get_v2_from_index(node, grid);
                node = came_from[node];
            }
            
#if 0
            for (int i = 0; i < num_nodes_in_path; ++i) {
                printf("%f %f\n", path.nodes[i].x, path.nodes[i].y);
            }
#endif
            
            return path;
            
        }
        
        //remove node from open set
        open_set[min_node] = false;
        
        v2 min_node_pos = get_v2_from_index(min_node, grid);
        
        //get neighbors
        u32 neighbors[8] = {
            [0] = get_index_from_v2(add_v2(min_node_pos, (v2){ 1, 0 }), grid),
            [1] = get_index_from_v2(add_v2(min_node_pos, (v2){ -1, 0 }), grid),
            [2] = get_index_from_v2(add_v2(min_node_pos, (v2){ 0, 1 }), grid),
            [3] = get_index_from_v2(add_v2(min_node_pos, (v2){ 0, -1 }), grid),
            [4] = get_index_from_v2(add_v2(min_node_pos, (v2){ -1, 1 }), grid),
            [5] = get_index_from_v2(add_v2(min_node_pos, (v2){ -1, -1 }), grid),
            [6] = get_index_from_v2(add_v2(min_node_pos, (v2){ 1, 1 }), grid),
            [7] = get_index_from_v2(add_v2(min_node_pos, (v2){ 1, -1 }), grid),
        };
        
        for (int i = 0; i < 8; ++i) {
            u32 neighbor = neighbors[i];
            if (neighbor >= grid.num_rows * grid.num_columns)
                continue;
            if (grid.obstacles[neighbor]) {
                continue;
            }
            
            float distance = 0;
            if (i < 4)
                distance = 1;
            else 
                distance = 1.4;
            
            float neighbor_g_score = g_score[min_node] + distance;
            if (neighbor_g_score < g_score[neighbor]) {
                came_from[neighbor] = min_node;
                g_score[neighbor] = neighbor_g_score;
                f_score[neighbor] = g_score[neighbor] + get_a_star_heuristic(get_v2_from_index(neighbor, grid), end);
                if (open_set[neighbor] == false) {
                    open_set[neighbor] = true;
                }
            }
        }
    }
    
    return (Path){0};
}

void add_obstacles_to_grid(Grid *grid, v2 obstacles[], u32 num_obstacles) {
    for (int i = 0; i < num_obstacles; ++i) {
        grid->obstacles[get_index_from_v2(obstacles[i], *grid)] = true;
    }
}

Grid create_grid_from_polygon(Polygon polygon, float tile_width, float tile_height, Allocation_type allocator_type) {
    float min_x = polygon.coords[0].x, min_y = polygon.coords[0].y;
    float max_x = polygon.coords[0].x, max_y = polygon.coords[0].y;
    for (int i = 0; i < polygon.num_coords; ++i) {
        if (polygon.coords[i].x < min_x)
            min_x = polygon.coords[i].x;
        if (polygon.coords[i].x > max_x)
            max_x = polygon.coords[i].x;
        if (polygon.coords[i].y < min_y)
            min_y = polygon.coords[i].y;
        if (polygon.coords[i].y > max_y)
            max_y = polygon.coords[i].y;
    }
    
    Grid grid = create_grid(get_rect(min_x, min_y, max_x, max_y), tile_width, tile_height, allocator_type);
    
    return grid;
}

void draw_grid(Grid grid) {
    renderer_draw_rect_to_screen_with_color(grid.rect, renderer_get_texture_by_name("white_texture"), get_v4(1, 1, 1, 0.2));
    v2 start_pos = { grid.rect.x0 + grid.tile_width / 2 , grid.rect.y0 + grid.tile_height / 2};
    for (int y = 0; y < grid.num_rows; ++y) {
        for (int x = 0; x < grid.num_columns; ++x) {
            v4 color = (v4){ 1, 0, 0, 0.5 };
            if (grid.obstacles[get_index_from_v2(get_v2(x, y), grid)]) {
                color.r = 0;
                color.b = 1;
            } 
            v2 pos = get_pos_from_grid_index(grid, (v2){ x ,y });
            Rect rect = get_rect_by_pos(pos, 0.02, 0.02);
            renderer_draw_rect_to_screen_with_color(rect, renderer_get_texture_by_name("white_texture"), color);
        }
    }
}

Mesh triangulate_using_fan_method(v2 coords[], u32 num_coords) {
    Mesh mesh = {0};
    u32 num_vertices = (num_coords - 2) * 3;
    //u32 num_vertices = 6;
    Vertex *vertices = (Vertex *)allocate(sizeof(Vertex) * num_vertices, perm_alloc);
    int *indices = (int *)allocate(sizeof(int) * 1, perm_alloc);
    memset(vertices, 0, sizeof(Vertex) * num_vertices);
    
    vertices[0].pos = get_v3_from_v2(coords[0]);
    vertices[1].pos = get_v3_from_v2(coords[1]);
    vertices[2].pos = get_v3_from_v2(coords[2]);
    
    int mesh_index = 3;
    for (int i = 2; i < num_coords - 1; ++i) {
        vertices[mesh_index + 0].pos = get_v3_from_v2(coords[0]);
        vertices[mesh_index + 1].pos = get_v3_from_v2(coords[i]);
        vertices[mesh_index + 2].pos = get_v3_from_v2(coords[i + 1]);
        mesh_index += 3;
    }
    assert(mesh_index == num_vertices);
    
    for (int i = 0 ; i < num_vertices; ++i) {
        vertices[i].color = get_v4(1, 1, 1, 0.2);
    }
    
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.num_vertices = num_vertices;
    mesh.num_indices = 1;
    
    return mesh;
}

Mesh triangulate_using_ear_clipping(v2 coords[], u32 num_coords) {
    //TODO
}

void clear_grid_of_obstalces() {
    memset(g_game_info.grid.obstacles, 0, g_game_info.grid.num_columns * g_game_info.grid.num_rows);
}

bool editor_create_polygon() {
    if (g_game_info.editor.is_drawing_polygon) {
        int i = 0;
        if (g_game_info.editor.num_polygon_clicks > 1) {
            
            for (; i < g_game_info.editor.num_polygon_clicks - 1; ++i) {
                renderer_draw_line_to_screen( g_game_info.editor.polygon_clicks[i], g_game_info.editor.polygon_clicks[i + 1], 0.005, default_color);
            }
        }
        renderer_draw_line_to_screen(g_game_info.editor.polygon_clicks[i], convert_from_screen_coords_to_ndc(g_platform.input.mouse_pos, g_screen_width, g_screen_height), 0.005, default_color);
    }
    
    if (g_platform.input.mouse_buttons[left_mouse_button].down) {
        g_game_info.editor.is_drawing_polygon = true;
        g_game_info.editor.polygon_clicks[g_game_info.editor.num_polygon_clicks++] = convert_from_screen_coords_to_ndc(g_platform.input.mouse_pos, g_screen_width, g_screen_height);
    }
    
    Mesh mesh = {0};
    
    if (g_platform.input.mouse_buttons[right_mouse_button].down) {
        clear_grid_of_obstalces();
        if (g_game_info.editor.num_polygon_clicks >= 3) {
            g_game_info.editor.nevmesh = triangulate_using_fan_method(g_game_info.editor.polygon_clicks, g_game_info.editor.num_polygon_clicks);
            
            v2 *polygon_coords = allocate(sizeof(*polygon_coords) * g_game_info.editor.num_polygon_clicks, frame_alloc);
            for (int i = 0; i < g_game_info.editor.num_polygon_clicks; ++i) {
                polygon_coords[i] = g_game_info.editor.polygon_clicks[i];
            }
            Polygon polygon = {
                .num_coords = g_game_info.editor.num_polygon_clicks,
                .coords = polygon_coords
            };
            for (int i = 0; i < g_game_info.grid.num_rows * g_game_info.grid.num_columns; ++i) {
#if 1
                bool inside = is_point_inside_polygon(polygon, get_pos_from_grid_index(g_game_info.grid, get_v2_from_index(i, g_game_info.grid)));
                
                if (inside) {
                    v2 obstacle = get_v2_from_index(i, g_game_info.grid);
                    add_obstacles_to_grid(&g_game_info.grid, &obstacle, 1);
                }
#else
                
                u32 counter = 0;
                v2 pos = get_pos_from_grid_index(g_game_info.grid, get_v2_from_index(i, g_game_info.grid));
                for (int j = 0; j < polygon.num_coords; ++j) {
                    v2 p0 = polygon.coords[j % polygon.num_coords];
                    v2 p1 =  polygon.coords[(j + 1) % polygon.num_coords];
                    if ((pos.y >= p0.y && pos.y <= p1.y) || (pos.y >= p1.y && pos.y <= p0.y)) {
                        counter++;
                        v2 obstacle = get_v2_from_index(i, g_game_info.grid);
                        add_obstacles_to_grid(&g_game_info.grid, &obstacle, 1);
                    }
                    else {
                        //g_game_info.grid.obstacles[i] = false;
                    }
                }
#endif
                
            }
            g_game_info.editor.num_polygon_clicks = 0;
            g_game_info.editor.is_drawing_polygon = false;
            
            return true;
            
        }
    }
    
    return false;
    
}

v2 get_hovered_tile() {
    v2 p0 = g_game_info.grid.rect.vec0;
    v2 p1 = g_game_info.grid.rect.vec1;
    convert_from_ndc_to_screen_coords(&p0.x, &p0.y, g_screen_width, g_screen_height);
    convert_from_ndc_to_screen_coords(&p1.x, &p1.y, g_screen_width, g_screen_height);
    
    float width = fabs(p0.x - p1.x);
    float height = fabs(p0.y - p1.y);
    
    v2 mouse_pos = get_mouse_position();
    
    float x_offset = p0.x;
    float y_offset = p0.y;
    
    v2 v = get_v2((s32)((mouse_pos.x - x_offset) / width * g_game_info.grid.num_columns), (s32)((mouse_pos.y - y_offset) / height * g_game_info.grid.num_rows ));
    
    return v;
}

void run_game() {
    Game_info *game_info = &g_game_info;
    
    handle_input();
    
    if (g_game_info.input_actions[input_action_edit_mode]) {
        g_game_info.is_in_edit_mode = true;
    }
    if (g_game_info.input_actions[input_action_game_mode]) {
        g_game_info.is_in_edit_mode = false;
    }
    
    if (g_game_info.is_in_edit_mode) {
        if (g_game_info.editor.is_entering_command) {
            read_command();
            display_command_line();
        }
        if (g_game_info.input_actions[input_action_enter_command]) {
            if (!g_game_info.editor.is_entering_command)
                g_game_info.editor.is_entering_command = true;
            else
                stop_reading_command();
        }
        else if (!g_game_info.editor.is_entering_command) {
            //draw_camera_rect(g_game_info.camera);
            float camera_speed = 0.7f * g_platform.delta_time_in_seconds;
            float camera_speed_2 = 80.0f * g_platform.delta_time_in_seconds;
            //float cameraSpeed = 2;
            
#if 1
            if (g_game_info.input_actions[input_action_move_camera_up]) {
                move_camera(get_v3(0, camera_speed, 0), null, &g_game_info.editor_camera);
            }
            if (g_game_info.input_actions[input_action_move_camera_down]) {
                move_camera(get_v3(0, -camera_speed, 0), null, &g_game_info.editor_camera);
            }
            if (g_game_info.input_actions[input_action_move_camera_left]) {
                move_camera(get_v3(-camera_speed, 0, 0), null, &g_game_info.editor_camera);
            }
            if (g_game_info.input_actions[input_action_move_camera_right]) {
                move_camera(get_v3(camera_speed, 0, 0), null, &g_game_info.editor_camera);
            }
            if (g_game_info.input_actions[input_action_move_camera_in]) {
                move_camera(get_v3(0, 0, camera_speed ), null, &g_game_info.editor_camera);
            }
            if (g_game_info.input_actions[input_action_move_camera_out]) {
                move_camera(get_v3(0, 0, -camera_speed), null, &g_game_info.editor_camera);
            }
#endif
        }
    }
    else {
        //TODO: remove this hack
        Entity *player = get_entity(g_game_info.player_entity);
        if (player->active) {
            
            v3 player_center = get_center_of_bbox(player->bounding_box);
            v4 v = get_v4(player_center.x, player_center.y, player_center.z, 1);
            v = mul_m4_by_v4(player->transform.transform_data->model, v);
            v3 player_pos = get_v3_from_v4(v);
            
            float x_camera_add = 0;
            float y_camera_add = 0;
            float z_camera_add = 0.1;
#if 0
            move_camera_to_pos(get_v3(player_pos.x + x_camera_add, player_pos.y + y_camera_add, g_game_info.camera.pos.z), &g_game_info.camera);
#endif
        }
    }
    
    update_entities();
    render_scene(g_game_info.current_scene, !g_game_info.is_in_edit_mode ? g_game_info.camera : g_game_info.editor_camera);
    
    //TODO: this code is wired, refactor it
    Rect panel_rect = (Rect){-1, -1, 1, -0.4};
    panel(true, panel_rect, renderer_get_texture_by_name("white_texture"), get_v4(0.1, 0.1, 0.1, 1));
    panel(true, center_rect(panel_rect, 1, 0.5), renderer_get_texture_by_name("white_texture"), get_v4(1, 1, 1, 0.5));
    bool is_panel_clicked = false;
    Rect panel_screen_coords_rect = convert_rect_ndc_to_screen_coords(panel_rect);
    bool is_hovering = is_point_inside_rect(panel_screen_coords_rect, g_platform.input.mouse_pos);
    if (g_game_info.input_actions[input_action_left_button_clicked] && is_hovering)
        is_panel_clicked = true;
    if (g_dialogue_system.is_dialogue_active)
        run_dialogue(game_info, is_panel_clicked);
    
#if 0
    renderer_draw_text_to_screen_formatted((Rect){-1, 0.8, 1, 1}, default_color, rect_alignment_left_top, 0, "door pos: (%f, %f, %f)\n", door_in_world_coords.x0, door_in_world_coords.y0, door_in_world_coords.z0, door_in_world_coords.x1 , door_in_world_coords.y1, door_in_world_coords.z1);
#endif
    
#if 0
    char pos_text_buffer[50];
    v3 door_pos = get_entity_by_name("door")->pos;
    sprintf(pos_text_buffer, "door pos: (%f %f %f)\n", door_pos.x, door_pos.y, door_pos.z);
    output_to_debug_view(pos_text_buffer);
#endif
    
    handle_fading();
    
    output_to_debug_view("camera pos: %f %f %f\n", g_game_info.camera.pos.x, g_game_info.camera.pos.y, g_game_info.camera.pos.z);
    
    Entity *player = get_entity_by_name("player");
    Entity *door = get_entity_by_name("door");
    if (player->active && door->active) {
        Bbox player_bbox = get_bounding_box_world_coords(player->bounding_box, player->transform.transform_data->model);
        //output_to_debug_view("player pos: %f %f %f\n", player->pos.x, player->pos.y, player->pos.z);
        
        Bbox door_bbox = get_bounding_box_world_coords(door->bounding_box, door->transform.transform_data->model);
#if 0
        output_to_debug_view("door bbox: %f %f %f %f, player bbox: %f %f %f %f\n",
                             door_bbox.x0, door_bbox.y0, door_bbox.x1, door_bbox.y1,
                             player_bbox.x0, player_bbox.y0, player_bbox.x1, player_bbox.y1
                             );
#endif
    }
    
    game_set_current_scene_background((Texture_id){0});
    
#if 1
    v2 goal_tile = get_hovered_tile();
    if (g_platform.input.keys[key_p].pressed && g_platform.input.mouse_buttons[left_mouse_button].down) {
        
        //g_game_info.path = get_a_star_path(g_game_info.grid, get_v2_from_index(door->path_mover.current_node, g_game_info.grid), goal_tile, perm_alloc);
    }
    output_to_debug_view("tile: %f %f\n", goal_tile.x, goal_tile.y);
    
    if (editor_create_polygon()) {
        //g_game_info.path = get_a_star_path(g_game_info.grid, get_v2_from_index(door->path_mover.current_node, g_game_info.grid), goal_tile, perm_alloc);
    }
    g_game_info.path = get_a_star_path(g_game_info.grid, get_v2_from_index(door->path_mover.current_node, g_game_info.grid), goal_tile, frame_alloc);
    output_to_debug_view("current node: %f \n", get_v2_from_index(door->path_mover.current_node, g_game_info.grid).x, get_v2_from_index(door->path_mover.current_node, g_game_info.grid).y);
    
    
    static bool b = true;
    if (g_game_info.editor.nevmesh.vertices) {
        if (g_platform.input.keys[key_1].down) {
            for (int i = 0; i < g_game_info.editor.nevmesh.num_vertices; ++i) {
                v3 v = g_game_info.editor.nevmesh.vertices[i].pos;
                printf("vertex %d: %f %f %f\n", i, v.x, v.y, v.z);
            }
            b = false;
        }
        renderer_draw_mesh(g_game_info.editor.nevmesh, get_default_transform_data(), renderer_get_texture_by_name("white_texture"), layer_nevmesh);
    }
#endif
    
    static bool is_drawing_rect = true;
    static v2 first_mouse_pos;
    Entity *npc = get_entity_by_name("npc0");
    static bool is_moving_entity = false;
    if (g_platform.input.mouse_buttons[left_mouse_button].down) {
        is_moving_entity = true;
        door->target = get_pos_in_world_coords_from_viewport(get_mouse_position());
        //move_entity_towards(door, door->target, 10 * g_platform.delta_time_in_seconds);
#if 0
        if (!is_drawing_rect) {
            is_drawing_rect = true;
            first_mouse_pos = convert_from_screen_coords_to_ndc(get_mouse_position(), g_screen_width, g_screen_height);
        }
        else {
            is_drawing_rect = false;
        }
        
#else
#if 1
        //set_entity_pos(get_entity(g_game_info.player_entity), ;
        
        v3 bbox_center = get_center_of_bbox(npc->bounding_box);
        v3 entity_world_coords_center = get_v3_from_v4(mul_m4_by_v4(npc->transform.transform_data->model, get_v4(bbox_center.x, bbox_center.y, bbox_center.z, 1.0f)));
        v3 lerped_mouse_pos = get_pos_in_world_coords_from_viewport(get_mouse_position());
        v3 previous_lerped_mouse_pos = get_pos_in_world_coords_from_viewport(g_platform.input.previous_mouse_pos);
        v3 offset = sub_v3(get_pos_in_world_coords_from_viewport(get_mouse_position()), entity_world_coords_center);
        //v3 offset = sub_v3(lerped_mouse_pos, previous_lerped_mouse_pos);
        //move_entity(npc, offset);
        //move_entity(npc, mul_v3_hadmard(offset, get_v3(1/npc->scale.x, 1/npc->scale.y, 1/npc->scale.z)));
        //move_entity_towards(npc, mul_v3_hadmard(get_mouse_pos_in_world_coords(get_mouse_position()), get_v3(1/npc->scale.x, 1/npc->scale.y, 1/npc->scale.z)), 10000 * g_platform.delta_time_in_seconds);
        //move_entity_towards(npc, get_mouse_pos_in_world_coords(get_mouse_position()), 100 * g_platform.delta_time_in_seconds);
#endif
#endif
    }
    else if (g_platform.input.mouse_buttons[right_mouse_button].down) {
        if (is_drawing_rect)
            is_drawing_rect = false;
    }
    //output_to_debug_view("mouse %f %f\n", get_pos_in_world_coords_from_viewport(get_mouse_position()).x, get_pos_in_world_coords_from_viewport(get_mouse_position()).y);
    v2 mouse_in_ndc = convert_from_screen_coords_to_ndc(get_mouse_position(), g_screen_width, g_screen_height);
    output_to_debug_view("mouse %f %f\n", mouse_in_ndc.x, mouse_in_ndc.y);
    output_to_debug_view("door %f %f\n", door->pos.x, door->pos.y);
    if ((fabs(door->target.x - door->pos.x) < 0.000001) && (fabs(door->target.y - door->pos.y) < 0.000001) && is_moving_entity) {
        //printf("wow\n");
        is_moving_entity = false;
    }
    if (is_moving_entity) {
        //move_entity_towards(door, door->target, 10 * g_platform.delta_time_in_seconds);
    }
    v2 mouse_pos = convert_from_screen_coords_to_ndc(get_mouse_position(), g_screen_width, g_screen_height);
    //Rect rect = get_rect(first_mouse_pos.x, first_mouse_pos.y, mouse_pos.x, mouse_pos.y);
    
    draw_grid(g_game_info.grid);
    
    static bool move_on_path;
    if (g_platform.input.keys[key_q].down) {
        move_on_path = true;
    }
    
    if (move_on_path) {
        bool is_moving = move_entity_on_path(door);
        if (!is_moving) {
            move_on_path = false;
            //door->path_mover.current_node = 0;
        }
    }
    
#if 0
    output_to_debug_view("entity->model:\n"
                         "(%f %f %f %f\n"
                         "%f %f %f %f\n"
                         "%f %f %f %f\n"
                         "%f %f %f %f)\n",
                         player->transform.transform_data->model.e[0][0], player->transform.transform_data->model.e[0][1], player->transform.transform_data->model.e[0][2], player->transform.transform_data->model.e[0][3],
                         player->transform.transform_data->model.e[1][0], player->transform.transform_data->model.e[1][1], player->transform.transform_data->model.e[1][2], player->transform.transform_data->model.e[1][3],
                         player->transform.transform_data->model.e[2][0], player->transform.transform_data->model.e[2][1], player->transform.transform_data->model.e[2][2], player->transform.transform_data->model.e[2][3],
                         player->transform.transform_data->model.e[3][0], player->transform.transform_data->model.e[3][1], player->transform.transform_data->model.e[3][2], player->transform.transform_data->model.e[3][3]);
#endif
    
}

void init_game(bool use_save_file) {
    setup_dialogues();
    
    g_game_info.is_in_edit_mode = true;
    
    vector_add(g_game_info.scene_vector, (Scene){0});
    vector_add(g_game_info.entity_vector, (Entity){0});
    
    if (use_save_file) {
        if (!load_game("save_file")) {
            fprintf(stderr, "failed to load game\n");
        }
    }
    else {
        init_entities_and_scenes();
    }
    
    g_game_info.camera = create_camera(perspective_camera, g_screen_width, g_screen_height);
    g_game_info.editor_camera = create_camera(perspective_camera, g_screen_width, g_screen_height);
    //g_game_info.camera = create_camera(orthographic_camera, g_screen_width, g_screen_height);
    g_game_info.game_mode = game_mode_normal;
    g_game_info.game_state = game_state_playing;
    
    g_game_info.grid = create_grid((Rect){ -1, -0.4, 1, 1 }, 0.1, 0.1, perm_alloc);
    //g_game_info.grid.obstacles[get_index_from_v2((v2){ 1, 1 }, g_game_info.grid.num_columns)] = true;
    v2 obstacles[] = { get_v2(0, 1), get_v2(1, 1), get_v2(1, 3) };
    //add_obstacles_to_grid(&g_game_info.grid, obstacles, array_count(obstacles));
    v2 *nodes = (v2 *)allocate(sizeof(v2) * 2, perm_alloc);
    nodes[0] = get_v2(0, 0);
    nodes[1] = get_v2(1, 0);
    //g_game_info.path = create_path(2, nodes);
    //g_game_info.path = create_path_in_grid(g_game_info.grid, get_v2(0, 0), get_v2(4, 4), perm_alloc);
    g_game_info.path = get_a_star_path(g_game_info.grid, get_v2(0, 0), get_v2(3, 3), perm_alloc);
    assert(g_game_info.path.num_nodes > 0);
    
#if 0
    v2 p = get_v2(-0.95, -0.35);
    Polygon polygon = {
        .num_coords = 4,
        .coords = allocate(sizeof(*polygon.coords) * 4, frame_alloc)
    };
    polygon.coords[0] = get_v2(-1, -0.4);
    polygon.coords[1] = get_v2(-1, -0.3);
    polygon.coords[2] = get_v2(-0.9, -0.3);
    polygon.coords[3] = get_v2(-0.9, -0.4);
    assert(is_point_inside_polygon(polygon, p));
    assert(!is_point_inside_polygon(polygon, add_v2(p, get_v2(0.1, 0))));
#endif
    
}
