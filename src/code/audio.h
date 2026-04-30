#if !defined(AUDIO_H)

#define audio_state_output_channel_count 2

struct Playing_Sound
{
	// NOTE(Fermin): V2 are the Left and right channels
	V2 current_volume;
	V2 dcurrent_volume_per_second;
	V2 target_volume;

	f32 dsample;

	Loaded_Sound *id; // TODO: Stop using *Loaded_Sound and use an ID
	f32 samples_played; // NOTE: Should this be f64
	Playing_Sound *next;
};

struct Game_Audio_State
{
	Render_Buffer sound_buffer;
	Playing_Sound *first_playing_sound;
	Playing_Sound *first_free_playing_sound;
	V2 master_volume;
};

#define AUDIO_H
#endif
