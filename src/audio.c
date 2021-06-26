
 #define audio_max_num_frames_to_play 8000 * 2
#define audio_num_channels 2
#define audio_sound_buffer_size audio_max_num_frames_to_play * audio_num_channels * 5
#define audio_max_num_samples_to_play audio_max_num_frames_to_play * audio_num_channels
#define audio_rate 44100

typedef struct Playing_sound {
    int frames_played;
    Sound *sound;
    bool loop;
    float volume;
    bool fully_played;
    struct Playing_sound *next_playing_sound;
} Playing_sound;

typedef struct Audio {
    int resample;
    snd_pcm_format_t format;
    
    snd_pcm_t *handle;
    snd_output_t *output;
    
    float samples[audio_sound_buffer_size];
    //Playing_sound *playing_sounds_vector;
    Playing_sound *playing_sounds;
} Audio;

Audio g_audio; 

static void audio_init() {
#if 0
    snd_pcm_t *handle;
    
    linux_assert(snd_output_stdio_attach(&g_audio.output, stdout, 0));
    linux_assert(snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0));
    
    //set sw params
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    
    linux_assert(snd_pcm_hw_params_any(handle, hw_params));
    
    int resample = 1;
    linux_assert(snd_pcm_hw_params_set_rate_resample(handle, hw_params, resample));
    
    snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
    linux_assert(snd_pcm_hw_params_set_access(handle, hw_params, access));
    
    snd_pcm_format_t format = SND_PCM_FORMAT_S16;
    linux_assert(snd_pcm_hw_params_set_format(handle, hw_params, format));
    
    u32 channels = 1;
    linux_assert(snd_pcm_hw_params_set_channels(handle, hw_params, channels));
    
    u32 rate = 44100;
    linux_assert(snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0)); //NOTE: consider checking for divergin returned rate
    
    u32 buffer_time = 500000;
    int dir;
    linux_assert(snd_pcm_hw_params_set_buffer_time_near(handle, hw_params, &buffer_time, &dir));
    
    snd_pcm_uframes_t buffer_size;
    linux_assert(snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size));
    
    u32 period_time = 100000;
    linux_assert(snd_pcm_hw_params_set_period_time_near(handle, hw_params, &period_time, &dir));
    
    snd_pcm_sframes_t period_size;
    linux_assert(snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir));
    
    linux_assert(snd_pcm_hw_params(handle, hw_params));
    
    //set sw params
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_sw_params_alloca(&sw_params);
    
    linux_assert(snd_pcm_sw_params_current(handle, sw_params));
    
    linux_assert(snd_pcm_sw_params_set_start_threshold(handle, sw_params, (buffer_size / period_size) * period_size));
    
    int period_event = 0;
    linux_assert(snd_pcm_sw_params_set_avail_min(handle, sw_params, period_event ? buffer_size : period_size));
    
    if (period_event) {
        linux_assert(snd_pcm_sw_params_set_period_event(handle, sw_params, 1));
    }
    
    linux_assert(snd_pcm_sw_params(handle, sw_params));
#else
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    
    u32 channels = 2;
    u32 sample_rate = 44100;
    //u32 sample_rate = 22050;
    
    linux_assert(snd_pcm_open(&g_audio.handle, "default", SND_PCM_STREAM_PLAYBACK, 0));
    linux_assert(snd_pcm_hw_params_any(g_audio.handle, hw_params));
    linux_assert(snd_pcm_hw_params_set_access(g_audio.handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
    linux_assert(snd_pcm_hw_params_set_format(g_audio.handle, hw_params, SND_PCM_FORMAT_FLOAT_LE));
    linux_assert(snd_pcm_hw_params_set_channels(g_audio.handle, hw_params, channels));
    linux_assert(snd_pcm_hw_params_set_rate_near(g_audio.handle, hw_params, &sample_rate, 0));
    int resample = 1;
    linux_assert(snd_pcm_hw_params_set_rate_resample(g_audio.handle, hw_params, resample));
    linux_assert(snd_pcm_hw_params(g_audio.handle, hw_params));
    snd_pcm_nonblock(g_audio.handle, 1);
#endif
}

void audio_start() {
    int result = snd_pcm_start(g_audio.handle);
}

void audio_change_volume(Playing_sound *playing_sound, float volume) {
    
}

static void audio_mix_sounds(int num_samples_to_add) {
    memset(g_audio.samples, 0, audio_max_num_samples_to_play);
    
    for (Playing_sound *playing_sound = g_audio.playing_sounds;
         playing_sound != null;
         playing_sound = playing_sound->next_playing_sound) {
        int num_samples_played = playing_sound->frames_played * audio_num_channels;
        int num_samples_in_sound = playing_sound->sound->num_frames * audio_num_channels;
        if (num_samples_played + num_samples_to_add > num_samples_in_sound) {
            num_samples_to_add = num_samples_in_sound - num_samples_played;
        }
        for (int j = 0; j < num_samples_to_add; ++j) {
            float sample = playing_sound->sound->sample_data[j + num_samples_played];
            g_audio.samples[j] += sample * playing_sound->volume;
        }
    }
}

static int linux_push_to_sound_buffer(float *samples, int num_frames_to_play) {
    
    if (!samples || !num_frames_to_play)
        return 0;
    
    int error_or_num_frames_written = 0;
    
    while (1) {
        error_or_num_frames_written = snd_pcm_writei(g_audio.handle, samples, num_frames_to_play);
        if (error_or_num_frames_written >= 0) {
            break;
        }
        else if (error_or_num_frames_written < 0) {
            if (error_or_num_frames_written == -EAGAIN) {
                continue;
            }
            else if (error_or_num_frames_written == -EPIPE) { 
                int recovery_error = snd_pcm_recover(g_audio.handle, error_or_num_frames_written, 0);
                //int recovery_error = snd_pcm_prepare(g_audio.handle);
                linux_assert(recovery_error);
                
                int start_error = snd_pcm_start(g_audio.handle);
                linux_assert(start_error);
                
                error_or_num_frames_written = snd_pcm_writei(g_audio.handle, samples, num_frames_to_play);
                
                return 0;
            }
        }
    }
    
    return error_or_num_frames_written;
}

static void audio_update_playing_sounds(int frames_played) {
#if 0
    for (Playing_sound *playing_sound = g_audio.playing_sounds;
         playing_sound != null;
         playing_sound = playing_sound->next_playing_sound) {
        playing_sound->frames_played += frames_played;
    }
#endif
    
    Playing_sound **playing_sound_ptr = &g_audio.playing_sounds;
    while (*playing_sound_ptr) {
        (*playing_sound_ptr)->frames_played += frames_played;
        if ((*playing_sound_ptr)->frames_played >= (*playing_sound_ptr)->sound->num_frames) {
#if 0
            Playing_sound *next_playing_sound = (*playing_sound_ptr)->next_playing_sound;
            free(*playing_sound_ptr);
            *playing_sound_ptr = next_playing_sound;
#endif
        }
        playing_sound_ptr = &(*playing_sound_ptr)->next_playing_sound;
    }
}

static void audio_deinit() {
    //snd_pcm_drain(g_audio.handle);
    snd_pcm_close(g_audio.handle);
}

static void audio_play_sound(Sound *sound, float volume) {
    Playing_sound new_playing_sound = {
        .sound = sound,
        .volume = volume
    };
    if (!g_audio.playing_sounds) {
        g_audio.playing_sounds = malloc(sizeof(Playing_sound));
        memcpy(g_audio.playing_sounds, &new_playing_sound , sizeof(Playing_sound));
        return;
    }
    
    Playing_sound **playing_sound = &g_audio.playing_sounds;
    while ((*playing_sound)->next_playing_sound) {
        playing_sound = &(*playing_sound)->next_playing_sound ;
    }
    
    *playing_sound = malloc(sizeof(Playing_sound));
    memcpy(g_audio.playing_sounds, &new_playing_sound , sizeof(Playing_sound));
}
