

#undef PI //NOTE: provisional
#include "spine-c/spine-c/include/spine/extension.h"

#define max_vertices_per_attachment 2048

char* _spUtil_readFile(const char* path, int* length) {
    char *result = _spReadFile(path, length);
    return result;
}

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path){
#if 0
    Texture *texture = malloc(sizeof(Texture));
    *texture = load_texture(g_allocator, path);
    self->rendererObject = texture;
    
    self->width = texture->width;
    self->height = texture->height;
#else
    //Texture loaded_texture = load_texture(g_allocator, path);
    //Vulkan_texture_id texture_id = create_vulkan_texture(&loaded_texture);
    Texture *texture = assets_get_texture_by_name("spine_texture");
    self->rendererObject = null;
    self->width = texture->width;
    self->height = texture->height;
#endif
}

void _spAtlasPage_disposeTexture (spAtlasPage* self) {
#if 0
    assert(self);
    Texture *texture = (Texture *)self->rendererObject;
    free(texture->data);
#endif
}

char* animation_names[] = { "walk", "run", "shot" };

typedef struct Spine_animation_data {
    spAnimationStateData *animation_state_data;
    spSkeleton *skeleton;
    spAnimationState *animation_state;
} Spine_animation_data;

void my_listener(spAnimationState* state, spEventType type, spTrackEntry* entry, spEvent* event) {
    
}

Spine_animation_data create_spine_animation(const Spine_assets *spine_assets) {
    spAnimationStateData *animation_state_data = spAnimationStateData_create(spine_assets->skeleton_data);
    assert(animation_state_data);
    
    animation_state_data->defaultMix = 0.5f;
    //spAnimationStateData_setDefaultMix(animation_state_data, 0.1f);
    spAnimationStateData_setMixByName(animation_state_data, "walk", "run", 0.2f);
    spAnimationStateData_setMixByName(animation_state_data, "walk", "shot", 0.1f);
    
    spSkeleton *skeleton = spSkeleton_create(spine_assets->skeleton_data);
    assert(skeleton);
    skeleton->x = 0;
    skeleton->y = -300;
    
    spAnimationState *animation_state = spAnimationState_create(animation_state_data);
    assert(animation_state);
    animation_state->listener = my_listener;
    int random_number = get_random_number(0, 3);
    spAnimationState_setAnimationByName(animation_state, 0, "idle", 1);
    
    Spine_animation_data spine_animation = {0};
    spine_animation.animation_state_data = animation_state_data;
    spine_animation.skeleton = skeleton;
    spine_animation.animation_state = animation_state;
    
    return spine_animation;
}

#if 0
Spine_data spine_load(const char *atlas_path, const char *skeleton_path) {
    spAtlas* atlas = spAtlas_createFromFile(atlas_path, 0);
    assert(atlas);
    
#if 0
    spSkeletonBinary* binary = spSkeletonBinary_create(atlas);
    assert(binary);
    binary->scale = 2;
    
    spSkeletonData* skeleton_data = spSkeletonBinary_readSkeletonDataFile(binary, skeleton_path);
    assert(skeleton_data);
    
    spSkeletonBinary_dispose(binary); 
#else
    spSkeletonJson* json = spSkeletonJson_create(atlas);
    assert(json);
    json->scale = 1;
    
    spSkeletonData* skeleton_data = spSkeletonJson_readSkeletonDataFile(json, skeleton_path);
    if (!skeleton_data) {
        printf("%s\n", json->error);
        exit(0);
    }
    
    spSkeletonJson_dispose(json);
    
#endif
    
    spAnimationStateData *animation_state_data = spAnimationStateData_create(skeleton_data);
    assert(animation_state_data);
    
    animation_state_data->defaultMix = 0.5f;
    //spAnimationStateData_setDefaultMix(animation_state_data, 0.1f);
    spAnimationStateData_setMixByName(animation_state_data, "walk", "run", 0.2f);
    spAnimationStateData_setMixByName(animation_state_data, "walk", "shot", 0.1f);
    
    spSkeleton *skeleton = spSkeleton_create(skeleton_data);
    assert(skeleton);
    skeleton->x = 0;
    skeleton->y = -300;
    
    spAnimationState *animation_state = spAnimationState_create(animation_state_data);
    assert(animation_state);
    animation_state->listener = my_listener;
    int random_number = get_random_number(0, 3);
    spAnimationState_setAnimationByName(animation_state, 0, "idle", 1);
    
    Spine_data spine_data = {
        .atlas = atlas,
        .skeleton_data = skeleton_data,
        .animation_state_data = animation_state_data,
        .skeleton = skeleton,
        .animation_state = animation_state
    };
    
    return spine_data;
}
#endif

typedef enum Blend_mode {
    no_blend,
    normal_blend,
    additive_blend,
    multiply_blend,    
    screen_blend, 
} Blend_mode;

void spine_render(Transform_data transform_data, Renderer_texture_id texture_id, Bbox *out_bbox, Spine_animation_data *animation, Layer layer) {
    spSkeleton *skeleton = animation->skeleton;
    
    spAnimationState_update(animation->animation_state, g_platform.delta_time_in_seconds);
    spAnimationState_apply(animation->animation_state, skeleton);
    spSkeleton_updateWorldTransform(skeleton);
    
    //TODO: getting thte bounding box this way  will eventually break, dont initialize the first coordinates to be 0
    Bounding_box bounding_box = {0};
    for (int i = 0; i < skeleton->slotsCount ; ++i) {
        spSlot* slot = skeleton->drawOrder[i];
        spAttachment* attachment = slot->attachment;
        if (!attachment) 
            continue;
        
        Blend_mode blend_mode;
        switch(slot->data->blendMode) {
            case SP_BLEND_MODE_NORMAL:
            blend_mode = normal_blend;
            break;
            case SP_BLEND_MODE_ADDITIVE:
            blend_mode = additive_blend;
            break;
            case SP_BLEND_MODE_MULTIPLY:
            blend_mode = multiply_blend;
            break;
            case SP_BLEND_MODE_SCREEN:
            blend_mode = screen_blend;
            break;
            default:
            blend_mode = normal_blend;
        }
        
        float tint_r = skeleton->color.r * slot->color.r;
        float tint_g = skeleton->color.g * slot->color.g;
        float tint_b = skeleton->color.b * slot->color.b;
        float tint_a = skeleton->color.a * slot->color.a;
        
        //Vulkan_texture_id texture_id = vulkan_object->texture_id;
        int num_vertices = 0;
        float world_vertices_positions[max_vertices_per_attachment] = {0};
        Vertex *vertices = (Vertex *)allocate_frame(sizeof(Vertex) * max_vertices_per_attachment);
        if (attachment->type == SP_ATTACHMENT_REGION) {
            spRegionAttachment *region_attachment = (spRegionAttachment *)attachment;
            
            //texture = (vulkan_texture *)((spAtlasRegion *)region_attachment->rendererObject)->page->rendererObject;
            
            spRegionAttachment_computeWorldVertices(region_attachment, slot->bone, world_vertices_positions, 0, 2);
            
            //Vertex vertices[6];
            vertices[0] = create_vertex(get_v3(world_vertices_positions[0], world_vertices_positions[1], 0), get_v2(region_attachment->uvs[0], region_attachment->uvs[1]), get_v4 (tint_r, tint_g, tint_b, tint_a));
            
            vertices[1] = create_vertex(get_v3(world_vertices_positions[2], world_vertices_positions[3], 0), get_v2(region_attachment->uvs[2], region_attachment->uvs[3]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[2] = create_vertex(get_v3(world_vertices_positions[4], world_vertices_positions[5], 0), get_v2(region_attachment->uvs[4], region_attachment->uvs[5]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[3] = create_vertex(get_v3(world_vertices_positions[4], world_vertices_positions[5], 0), get_v2(region_attachment->uvs[4], region_attachment->uvs[5]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[4] = create_vertex(get_v3(world_vertices_positions[6], world_vertices_positions[7], 0), get_v2(region_attachment->uvs[6], region_attachment->uvs[7]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[5] = create_vertex(get_v3(world_vertices_positions[0], world_vertices_positions[1], 0), get_v2(region_attachment->uvs[0], region_attachment->uvs[1]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            num_vertices = 6;
            
            for (int i = 0; i < 8; i += 2) {
                bounding_box.min_x = fmin(bounding_box.min_x, world_vertices_positions[i]);
                bounding_box.min_y = fmin(bounding_box.min_y, world_vertices_positions[i + 1]);
                bounding_box.max_x = fmax(bounding_box.max_x, world_vertices_positions[i]);
                bounding_box.max_y = fmax(bounding_box.max_y, world_vertices_positions[i + 1]);
            }
        }
        else if (attachment->type == SP_ATTACHMENT_MESH) {
            spMeshAttachment* spine_mesh = (spMeshAttachment*)attachment;
            
            // Check the number of vertices in the spine_mesh attachment. If it is bigger
            // than our scratch buffer, we don't render the spine_mesh. We do this here
            // for simplicity, in production you want to reallocate the scratch buffer
            // to fit the mesh.
            if (spine_mesh->super.worldVerticesLength > max_vertices_per_attachment)
                continue;
            
            // Our engine specific Texture is stored in the spAtlasRegion which was
            // assigned to the attachment on load. It represents the texture atlas
            // page that contains the image the mesh attachment is mapped to
            //texture = (vulkan_texture*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;
            
            // Computed the world vertices positions for the vertices that make up
            // the mesh attachment. This assumes the world transform of the
            // bone to which the slot (and hence attachment) is attached has been calculated
            // before rendering via spSkeleton_updateWorldTransform
            spVertexAttachment_computeWorldVertices(SUPER(spine_mesh), slot, 0, spine_mesh->super.worldVerticesLength, world_vertices_positions, 0, 2);
            
            // Mesh attachments use an array of vertices, and an array of indices to define which
            // 3 vertices make up each triangle. We loop through all triangle indices
            // and simply emit a vertex for each triangle's vertex.
            for (int i = 0; i < spine_mesh->trianglesCount; ++i) {
                ++num_vertices;
                int index = spine_mesh->triangles[i] << 1;
                //addVertex(world_vertices_positions[index], world_vertices_positions[index + 1],
                //mesh->uvs[index], mesh->uvs[index + 1], 
                //tint_r, tintG, tintB, tintA);
                
                vertices[i] = create_vertex(get_v3(world_vertices_positions[index],  world_vertices_positions[index + 1], 0), get_v2(spine_mesh->uvs[index], spine_mesh->uvs[index + 1]), get_v4(tint_r, tint_g, tint_b, tint_a));
                
                bounding_box.min_x = fmin(bounding_box.min_x, world_vertices_positions[index]);
                bounding_box.min_y = fmin(bounding_box.min_y, world_vertices_positions[index + 1]);
                bounding_box.max_x = fmax(bounding_box.max_x, world_vertices_positions[index]);
                bounding_box.max_y = fmax(bounding_box.max_y, world_vertices_positions[index + 1]);
            }
        }
        else {
            continue;
        }
        
        //temp
        for (int j = 0; j < num_vertices; ++j) {
            vertices[j].pos.y = vertices[j].pos.y;
        }
        
        int num_indices = 1;
        int *indices  = (int *)allocate_frame(num_indices * sizeof(int));
        Mesh mesh = {
            .vertices = vertices,
            .indices = indices,
            .num_vertices = num_vertices,
            .num_indices = num_indices
        };
        renderer_draw_mesh(mesh, transform_data, texture_id, layer);
    }
    
    //TODO: fix spine to work with vulkan coordinates
    //TODO: add 'get bounding box from mesh' function
    bounding_box.min_y = bounding_box.min_y;
    bounding_box.max_y = bounding_box.max_y;
    *out_bbox = bounding_box;
}

Mesh get_skeleton_vertices(Spine_animation_data *animation) {
    spSkeleton *skeleton = animation->skeleton;
    
    spAnimationState_update(animation->animation_state, g_platform.delta_time_in_seconds);
    spAnimationState_apply(animation->animation_state, skeleton);
    spSkeleton_updateWorldTransform(skeleton);
    
    Vertex *mesh_vertices = null;
    
    for (int i = 0; i < skeleton->slotsCount ; ++i) {
        spSlot* slot = skeleton->drawOrder[i];
        spAttachment* attachment = slot->attachment;
        if (!attachment) 
            continue;
        
        Blend_mode blend_mode;
        switch(slot->data->blendMode) {
            case SP_BLEND_MODE_NORMAL:
            blend_mode = normal_blend;
            break;
            case SP_BLEND_MODE_ADDITIVE:
            blend_mode = additive_blend;
            break;
            case SP_BLEND_MODE_MULTIPLY:
            blend_mode = multiply_blend;
            break;
            case SP_BLEND_MODE_SCREEN:
            blend_mode = screen_blend;
            break;
            default:
            blend_mode = normal_blend;
        }
        
        float tint_r = skeleton->color.r * slot->color.r;
        float tint_g = skeleton->color.g * slot->color.g;
        float tint_b = skeleton->color.b * slot->color.b;
        float tint_a = skeleton->color.a * slot->color.a;
        
        float world_vertices_positions[max_vertices_per_attachment] = {0};
        
        if (attachment->type == SP_ATTACHMENT_REGION) {
            spRegionAttachment *region_attachment = (spRegionAttachment *)attachment;
            
            spRegionAttachment_computeWorldVertices(region_attachment, slot->bone, world_vertices_positions, 0, 2);
            
            Vertex vertices[6];
            vertices[0] = create_vertex(get_v3(world_vertices_positions[0], world_vertices_positions[1], 0), get_v2(region_attachment->uvs[0], region_attachment->uvs[1]), get_v4 (tint_r, tint_g, tint_b, tint_a));
            
            vertices[1] = create_vertex(get_v3(world_vertices_positions[2], world_vertices_positions[3], 0), get_v2(region_attachment->uvs[2], region_attachment->uvs[3]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[2] = create_vertex(get_v3(world_vertices_positions[4], world_vertices_positions[5], 0), get_v2(region_attachment->uvs[4], region_attachment->uvs[5]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[3] = create_vertex(get_v3(world_vertices_positions[4], world_vertices_positions[5], 0), get_v2(region_attachment->uvs[4], region_attachment->uvs[5]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[4] = create_vertex(get_v3(world_vertices_positions[6], world_vertices_positions[7], 0), get_v2(region_attachment->uvs[6], region_attachment->uvs[7]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            vertices[5] = create_vertex(get_v3(world_vertices_positions[0], world_vertices_positions[1], 0), get_v2(region_attachment->uvs[0], region_attachment->uvs[1]), get_v4(tint_r, tint_g, tint_b, tint_a));
            
            for (int i = 0; i < 6; ++i) {
                put_in_sbuff(mesh_vertices, vertices[i]);
            }
        }
        else if (attachment->type == SP_ATTACHMENT_MESH) {
            spMeshAttachment* mesh = (spMeshAttachment*)attachment;
            
            if (mesh->super.worldVerticesLength > max_vertices_per_attachment)
                continue;
            
            spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, world_vertices_positions, 0, 2);
            
            for (int i = 0; i < mesh->trianglesCount; ++i) {
                int index = mesh->triangles[i] << 1;
                
                Vertex vertex = create_vertex(get_v3(world_vertices_positions[index],  world_vertices_positions[index + 1], 0), get_v2(mesh->uvs[index], mesh->uvs[index + 1]), get_v4(tint_r, tint_g, tint_b, tint_a));
                
                put_in_sbuff(mesh_vertices, vertex);
                
            }
        }
        else {
            continue;
        }
        
        
    }
    for (int j = 0; j < get_sbuff_length(mesh_vertices); ++j) {
        mesh_vertices[j].pos.y = -mesh_vertices[j].pos.y;
    }
    int indices[] = {1};
    int num_indices = 1;
    Mesh mesh = {.vertices = mesh_vertices, .indices = indices, .num_vertices = get_sbuff_length(mesh_vertices), .num_indices = num_indices };
    
    return mesh;
}

void destroy_spine_assets(Spine_assets *data) {
    spAtlas_dispose(data->atlas);
    spSkeletonData_dispose(data->skeleton_data);
}

void destroy_spine_animation_data(Spine_animation_data *data) {
    spAnimationStateData_dispose(data->animation_state_data);
}