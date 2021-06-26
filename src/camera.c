typedef enum Camera_type { perspective_camera, orthographic_camera } Camera_type;
typedef enum Camera_mode { camera_mode_free, camera_mode_focus_on_player } Camera_mode;

#define camera_near 0.1f
#define camera_far 100.0f
#define fov 45.0f

#define camera_default_pos (v3){0, 0, 0}
#define camera_default_up (v3){0, 1, 0}
#define camera_default_front (v3){0, 0, -1}

typedef struct Camera {
    v3 pos;
    v3 front;
    v3 up;
    m4 projection;
    Camera_type camera_type;
    Camera_mode mode;
    
} Camera;

static Camera create_camera(Camera_type camera_type, int screen_width, int screen_height) {
    Camera camera = {
        .pos = get_v3(0, 0, 0),
        .front = get_v3(0, 0, -1),
        .up = get_v3(0, 1, 0),
        .projection = math_get_identity_matrix(),
    };
    
    float aspect_ratio = (float)screen_width / (float)screen_height;
    
    if (camera_type == perspective_camera) {
        camera.projection = math_get_perspective_matrix(math_convert_to_radians(fov), aspect_ratio, camera_near, camera_far);
    }
    else if (camera_type == orthographic_camera) {
        camera.projection = math_get_orthographic_matrix(get_rect(-screen_width / 2, screen_width / 2, screen_height / 2, -screen_height / 2), camera_near, camera_far);
    }
    
    return camera;
}

m4 calculate_view_matrix_from_camera(Camera camera) {
    m4 view = math_lookat(camera.pos, add_v3(camera.pos, camera.front), camera.up);
    
    return view;
}

m4 get_projection_matrix_from_camera(Camera camera) {
    m4 projection = camera.projection;
    
    return projection;
}

void move_camera_to_pos(v3 pos, Camera *camera) {
#if 1
    v3 final_pos = get_v3(lerp(camera->pos.x, pos.x, 0.08), pos.y, pos.z);
    camera->pos = final_pos;
#endif
}

void set_camera_pos(v3 pos, Camera *camera) {
#if 1
    camera->pos = pos;
#endif
}

void move_camera(v3 amount, Bounding_box *bbox, Camera *camera) {
    
    float amount_x = amount.x;
    float amount_y = amount.y;
    float amount_z = amount.z;
    
    v3 left = cross_v3(camera->front, camera->up);
    //v3 right = get_v3(-1 * left.x, left.y, left.z);
    
    v3 new_camera_pos = camera->pos;
    
    v3 add_x = mul_v3_by_scalar(left, amount_x);
    new_camera_pos = add_v3(new_camera_pos, add_x);
    
    v3 add_y = mul_v3_by_scalar(camera->up, amount_y);
    new_camera_pos = add_v3(new_camera_pos, add_y);
    
    v3 add_z = mul_v3_by_scalar(camera->front, amount_z);
    new_camera_pos = add_v3(new_camera_pos, add_z);
    
    camera->pos = new_camera_pos;
    
#if 0
    if (!bbox || (new_camera_pos.x > bbox->min_x && new_camera_pos.x < bbox->max_x)) {
        camera->pos = new_camera_pos;
    }
    
    if (!bbox || (new_camera_pos.y > bbox->min_y && new_camera_pos.y < bbox->max_y)) {
        camera->pos = new_camera_pos;
    }
#endif
    
    
    
}

