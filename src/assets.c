#ifndef PR_ASSETS
#define PR_ASSETS

#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define EVENT_SIZE (sizeof(struct inotify_event))

#define get_file_code(a, b, c, d) (((u32)(a) << 0 | (u32)(b) << 8 | (u32)(c) << 16 | (u32)(d) << 24))
#define save_file_magic_value get_file_code('p', 'a', 's', 'f')
#define asset_file_magic_value get_file_code('p', 'a', 'a', 'f')

typedef struct Spine_assets {
    spAtlas *atlas;
    spSkeletonData *skeleton_data;
} Spine_assets;

typedef struct Sound {
    char name[standrad_name_length];
    u32 size;
    int num_channels;
    int sample_rate;
    int num_frames;
    float *sample_data;
} Sound;

typedef struct Sound_id {
    int id;
} Sound_id;

#define max_num_fonts 10

typedef struct Font_t {
    char name[standrad_name_length];
    stbtt_fontinfo font_info;
} Font_t;

typedef struct Assets {
    Texture *textures_vector;
    Sound *sounds_vector;
    Spine_assets spine_assets;
    
    Font_t *fonts;
    u32 num_fonts;
    
} Assets;

Assets g_assets;


typedef enum Asset_type {
    asset_type_texture,
    asset_type_spine_atlas,
    asset_type_spine_json,
    asset_type_spine_skeleton,
    asset_type_wav,
    asset_type_flac,
    asset_type_mp3,
    asset_type_num
} Asset_type;

typedef struct Asset_database_item {
    const char *name;
    const char *path;
    Asset_type type;
    //time_t modification_time;
} Asset_database_item;

static Texture load_texture(const char *path, const char *name) {
    //TODO: move this into a full formed asset system, with the custom allocator, and consider writing it yourself if necessary
    //consider baking it into a seperate asset file
    s32 width, height, channels;
    u8 *data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    
    if (!data) 
        return (Texture){};
    
    size_t image_size = width * height * channels;
    u8 *custom_allocator_data = allocate(image_size, perm_alloc);
    
    u8 *dest = custom_allocator_data;
#if 0
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 3) {
            int index = y * width + x;
            dest[0] = data[index];
            dest[1] = data[index + 1];
            dest[2] = data[index + 2];
            dest[3] = 0xFF;
            dest += 4;
        }
    }
#endif
    
    Texture texture = (Texture) {
        .width = (u32)width,
        .height = (u32)height,
        .channels = 4,
        .data = data,
        .size = width * height * 4,
    };
    
    copy_string_to_buffer(name, texture.name, standrad_name_length);
    
    return texture;
}

void destroy_texture(Texture *texture) {
    stbi_image_free(texture->data);
}

//AUDIO
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_FLAC_IMPLEMENTATION
#include "externals/dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "externals/dr_mp3.h"

Sound assets_load_sound(Asset_database_item item) {
    //TODO: use custom allocator
    unsigned int channels;
    unsigned int sample_rate;
    drwav_uint64 num_frames;
    float *sample_data = null;
    
    if (item.type == asset_type_wav) {
        sample_data = drwav_open_file_and_read_pcm_frames_f32(item.path, &channels, &sample_rate, &num_frames, NULL);
    }
    else if (item.type == asset_type_flac) {
        sample_data = drflac_open_file_and_read_pcm_frames_f32(item.path, &channels, &sample_rate, &num_frames, NULL);
    }
    else if (item.type == asset_type_mp3) {
        drmp3_config config;
        sample_data = drmp3_open_file_and_read_pcm_frames_f32(item.path, &config, &num_frames, NULL);
        sample_rate = config.sampleRate;
        channels = config.channels;
    }
    
    assert(sample_data);
    assert(channels == 2);
    assert(sample_rate == 44100);
    
    Sound sound = {
        .num_channels = channels,
        .sample_rate = sample_rate,
        .num_frames = num_frames,
        .sample_data = sample_data,
        .size = sizeof(float) * num_frames * channels
    };
    memcpy(sound.name, item.name, standrad_name_length);
    
    return sound;
}

Sound_id assets_add_sound_to_assets(Sound sound) {
    Sound_id sound_id = { vector_get_length(g_assets.sounds_vector) };
    vector_add(g_assets.sounds_vector, sound);
    
    return sound_id;
}

Sound_id assets_load_and_add_sound_to_assets(Asset_database_item item) {
    Sound sound = assets_load_sound(item);
    Sound_id sound_id = assets_add_sound_to_assets(sound);
    
    return sound_id;
}

void free_sound(Sound  *sound) {
    drwav_free(sound->sample_data, null);
}

Texture load_font(const char *name) {
    stbtt_fontinfo font;
    u8 *bitmap;
    int width, height, x_offset, y_offset;
    int code_point = 'a';
    int pixel_height = 200;
    char* ttf_buffer = malloc(100000);
    
    FILE *file = fopen(name, "rb");
    assert(file);
    fread(ttf_buffer, 100000, 1, file);
    stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
    bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, pixel_height), code_point, &width, &height, &x_offset, &y_offset);
    
    u8 *font_bitmap = allocate_perm(height * width * 3);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = y * width + x;
            font_bitmap[index * 3] = bitmap[index];
            font_bitmap[index * 3 + 1] = bitmap[index];
            font_bitmap[index * 3 + 2] = bitmap[index];
            font_bitmap[index * 3 + 3] = bitmap[index];
        }
    }
    
    Texture font_texture = {
        .width = width,
        .height = height,
        .channels = 3,
        .data = font_bitmap,
        .size = width * height * 3
    };
    
#if 0
    for (int y = 0; y < height; ++y) {
        for (int x = 0 ; x < width; ++x) {
            putchar(" .:ioVM@"[bitmap[y*width+x]>>5]);
        }
        putchar('\n');
    }
#endif
    
    free(ttf_buffer);
    stbtt_FreeBitmap(bitmap, null);
    
    return font_texture;
}

char *get_file_buffer(const char *path) {
    FILE *file = fopen(path, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* ttf_buffer = malloc(size + 1);
    fread(ttf_buffer, size, 1, file);
    ttf_buffer[size] = 0;
    fclose(file);
    
    return ttf_buffer;
}

char *read_file(const char *path, size_t *out_size) {
    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *file_data = malloc(size + 1);
    fread(file_data, size, 1, file);
    file_data[size] = 0;
    
    *out_size = size;
    
    return file_data;
}

void add_font(const char *path, const char *name) {
    char *ttf_buffer = get_file_buffer(path);
    stbtt_fontinfo font_info;
    assert(stbtt_InitFont(&font_info, ttf_buffer, 0));
    Font_t font = { .font_info = font_info };
    strcpy(font.name, name);
    g_assets.fonts[g_assets.num_fonts++] = font;
}

Font_t get_font(const char *name) {
    for (int i = 0; i < g_assets.num_fonts; ++i) {
        if (strcmp(name, g_assets.fonts[i].name) == 0)
            return g_assets.fonts[i];
    }
    assert(0);
    return (Font_t){0};
}

Sound *assets_get_sound_by_id(Sound_id sound_id) {
    return &g_assets.sounds_vector[sound_id.id];
}

Texture *assets_get_texture_by_name(const char *name) {
    if (!name || !(*name))
        return (Texture *){0};
    
    for (int i = 0; i < get_sbuff_length(g_assets.textures_vector); ++i) {
        if (is_literal_string_equal(name, g_assets.textures_vector[i].name, standrad_name_length)) {
            return &g_assets.textures_vector[i];
        }
    }
    
    return (Texture *){0};
}

typedef struct Texture_id {
    int id;
} Texture_id;

Texture assets_load_texture(Asset_database_item item) {
    Texture texture = load_texture(item.path, item.name);
    
    return texture;
}

Texture_id assets_add_texture_to_assets(Texture texture) {
    Texture_id texture_id = { .id = vector_get_length(g_assets.textures_vector) } ;
    vector_add(g_assets.textures_vector, texture);
    
    return texture_id;
}

Texture_id assets_load_and_add_texture_to_assets(Asset_database_item item) {
    Texture texture = assets_load_texture(item);
    Texture_id texture_id = assets_add_texture_to_assets(texture);
    
    return texture_id;
}

Spine_assets load_spine_assets(const char *atlas_path, const char *skeleton_path) {
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
    
    Spine_assets spine_assets = {
        .atlas = atlas,
        .skeleton_data = skeleton_data
    };
    
    return spine_assets;
}

void assets_load_wav(Asset_database_item item) {
    
}

void load_all_assets(Asset_database_item *items, int num_items) {
    for (int i = 0; i < num_items; ++i) {
        Asset_database_item item = items[i];
        if (item.type == asset_type_texture) {
            assets_load_and_add_texture_to_assets(item);
        }
        else if (item.type == asset_type_wav || item.type == asset_type_flac || item.type == asset_type_mp3) {
            assets_load_and_add_sound_to_assets(item);
        }
    }
    
    g_assets.spine_assets = load_spine_assets("spine_assets/export/spineboy.atlas", "spine_assets/export/spineboy-ess.json");
}

Sound *assets_get_sound_by_name(const char *name) {
    for (int i = 0; i < get_sbuff_length(g_assets.sounds_vector); ++i) {
        if (is_literal_string_equal(name, g_assets.sounds_vector[i].name, standrad_name_length)) {
            return &g_assets.sounds_vector[i];
        }
    }
    
    assert(0);
}

#endif


typedef struct File {
    const char *name;
    time_t modification_time;
} File;

time_t get_modification_time(const char *path) {
    struct stat new_modification_time;
    int error = stat(path, &new_modification_time);
    if (error != 0) { 
        assert(0);
    }
    
    return new_modification_time.st_mtime;
}

#if 0
void check_and_load_modified_assets() {
    for (int i = 0; i < vector_get_length(g_asset_system.asset_database.database_item_vector); ++i) {
        time_t modification_time = get_modification_time(g_asset_system.asset_database.database_item_vector[i].path);
        time_t last_modification_time = g_asset_system.asset_database.database_item_vector[i].modification_time;
        if (modification_time > last_modification_time) {
            Asset_database_item database_item = g_asset_system.asset_database.database_item_vector[i];
            int id = get_asset_id(g_asset_system.asset_database.database_item_vector[i].name).id;
            g_asset_system.asset_vector[i] = load_and_get_asset(database_item);
        }
    }
}
#endif

#pragma pack(push, 1)

typedef struct Texture_info {
    char name[standrad_name_length];
    u32 size;
    u32 width, height, channels;
} Texture_info;

typedef struct Sound_info {
    char name[standrad_name_length];
    u32 size;
    int num_channels;
    int sample_rate;
    int num_frames;
} Sound_info;

Texture_info get_texture_info(Texture texture) {
    
    Texture_info texture_info = {
        .width = texture.width,
        .height = texture.height,
        .channels = texture.channels,
        .size = texture.size,
    };
    copy_string_to_buffer(texture.name, texture_info.name, standrad_name_length);
    
    return texture_info;
}

Sound_info get_sound_info(Sound sound) {
    
    Sound_info sound_info = {
        .num_channels = sound.num_channels,
        .sample_rate = sound.sample_rate,
        .num_frames = sound.num_frames,
        .size = sound.size
    };
    memcpy(sound_info.name, sound.name, standrad_name_length);
    
    return sound_info;
    
}

typedef struct Asset_file_header {
    u32 magic_value;
    u32 num_textures, num_sounds;
} Asset_file_header;

#pragma pack(pop)

bool create_asset_file(const char *asset_file_name, Asset_database_item *items, u32 num_items) {
    FILE *file = fopen(asset_file_name, "wb+");
    if (!file)
        return false;
    
    Texture *textures = null;
    Sound *sounds = null;
    
    for (int i = 0; i < num_items; ++i) {
        Asset_database_item item = items[i];
        switch(item.type) {
            case asset_type_texture: {
                Texture texture = assets_load_texture(item);
                vector_add(textures, texture);
                break;
            }
            case asset_type_spine_atlas: {
                break;
            }
            case asset_type_spine_json: {
                break;
            }
            case asset_type_spine_skeleton: {
                break;
            }
            case asset_type_wav:
            case asset_type_mp3:
            case asset_type_flac: {
                Sound sound = assets_load_sound(item);
                vector_add(sounds, sound);
                break;
            }
        }
    }
    
    Asset_file_header header = {
        .magic_value = asset_file_magic_value,
        .num_textures = vector_get_length(textures),
        .num_sounds = vector_get_length(sounds)
    };
    
    fwrite(&header, 1, sizeof(Asset_file_header), file);
    
    for (int i = 0; i < vector_get_length(textures); ++i) {
        Texture_info texture_info = get_texture_info(textures[i]);
        fwrite(&texture_info, 1, sizeof(Texture_info), file);
    }
    
    for (int i = 0; i < vector_get_length(sounds); ++i) {
        Sound_info sound_info = get_sound_info(sounds[i]);
        fwrite(&sound_info, 1, sizeof(Sound_info), file);
    }
    
    for (int i = 0; i < vector_get_length(textures); ++i) {
        fwrite(textures[i].data, 1, textures[i].size, file);
    }
    
    for (int i = 0; i < vector_get_length(sounds); ++i) {
        fwrite(sounds[i].sample_data, 1, sounds[i].size, file);
    }
    
    vector_free(sounds);
    vector_free(textures);
    
    fclose(file);
    
    return true;
}

Texture get_texture_from_texture_info(Texture_info texture_info) {
    Texture texture = {
        .width = texture_info.width,
        .height = texture_info.height,
        .channels = texture_info.channels,
        .size = texture_info.size,
        .data = allocate_perm(texture_info.size)
    };
    
    copy_string_to_buffer(texture_info.name, texture.name, standrad_name_length);
    
    return texture;
}

Sound get_sound_from_sound_info(Sound_info sound_info) {
    Sound sound = {
        .num_channels = sound_info.num_channels,
        .sample_rate = sound_info.sample_rate,
        .num_frames = sound_info.num_frames,
        .size = sound_info.size,
        .sample_data = (float *)allocate_perm(sound_info.size)
    };
    
    copy_string_to_buffer(sound_info.name, sound.name, standrad_name_length);
    
    return sound;
}

bool load_from_asset_file(const char *asset_file_name) {
    FILE *file = fopen(asset_file_name, "rb");
    if (!file)
        return false;
    
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (!length)
        return false;
    
    Asset_file_header header = {};
    fread(&header, 1, sizeof(Asset_file_header), file);
    assert(header.magic_value == asset_file_magic_value);
    
    Texture *textures = null;
    Sound *sounds = null;
    
    for (int i = 0; i < header.num_textures; ++i) {
        Texture_info texture_info = {};
        fread(&texture_info, 1, sizeof(Texture_info), file);
        Texture texture = get_texture_from_texture_info(texture_info);
        vector_add(textures, texture);
    }
    
    for (int i = 0; i < header.num_sounds; ++i) {
        Sound_info sound_info = {};
        fread(&sound_info, 1, sizeof(Sound_info), file);
        Sound sound = get_sound_from_sound_info(sound_info);
        vector_add(sounds, sound);
    }
    
    for (int i = 0; i < vector_get_length(textures); ++i) {
        fread(textures[i].data, 1, textures[i].size, file);
        assets_add_texture_to_assets(textures[i]);
    }
    
    for (int i = 0; i < vector_get_length(sounds); ++i) {
        fread(sounds[i].sample_data, 1, sounds[i].size, file);
        assets_add_sound_to_assets(sounds[i]);
    }
    
    vector_free(sounds);
    vector_free(textures);
    
}

void assets_init() {
    g_assets.fonts = (Font_t *)allocate_perm(sizeof(Font_t) * max_num_fonts);
    add_font("fonts/OpenSans-Bold.ttf", "open_sans_bold");
}