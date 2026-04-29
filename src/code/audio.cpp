static void
change_pitch(Game_Audio_State *audio_state, Playing_Sound *sound, f32 dsample)
{
	sound->dsample = dsample;
}

static Playing_Sound*
push_sound(Render_Buffer *sound_buffer)
{
    // NOTE(Fermin): Check if we have enough space for another Playing_Sound
    assert((sound_buffer->count+1) * sizeof(Playing_Sound) <= sound_buffer->buffer.size);

    Playing_Sound *pushed_sound = (Playing_Sound *)sound_buffer->buffer.data + sound_buffer->count++;

    return pushed_sound;
}

static Playing_Sound*
play_sound(Game_Audio_State *audio_state, Loaded_Sound *sound_id)
{
	// Create the idea of sound ids instead of Loaded_Sound
	if(!audio_state->first_free_playing_sound)
	{
		audio_state->first_free_playing_sound = push_sound(&audio_state->sound_buffer);
		audio_state->first_free_playing_sound->next = 0;
	}

	Playing_Sound *playing_sound = audio_state->first_free_playing_sound;
	audio_state->first_free_playing_sound = playing_sound->next;

	playing_sound->samples_played = 0.0f;
	playing_sound->current_volume = playing_sound->target_volume = V2{1.0f, 1.0f};
	playing_sound->dcurrent_volume_per_second = V2{0.0f, 0.0f};
	playing_sound->id = sound_id;
	playing_sound->dsample = 1.0f;

	playing_sound->next = audio_state->first_playing_sound;
	audio_state->first_playing_sound = playing_sound;

	return playing_sound;
}

static void
change_volume(Playing_Sound *sound, f32 fade_duration_in_seconds, V2 volume)
{
	if(fade_duration_in_seconds <= 0.0f)
	{
		sound->current_volume = sound->target_volume = volume;
	}
	else
	{
		f32 one_over_fade = 1.0f / fade_duration_in_seconds;
		sound->target_volume = volume;
		sound->dcurrent_volume_per_second = (sound->target_volume - sound->current_volume) * one_over_fade;
	}
}

static void
output_playing_sounds(Game_Audio_State *audio_state, Game_Sound_Output_Buffer *sound_output_buffer, void *temp_storage)
{
	f32 *real_channel_0 = (f32 *)temp_storage;
	f32 *real_channel_1 = real_channel_0 + sizeof(f32)*sound_output_buffer->sample_count;
	assert((real_channel_1 - real_channel_0) == (sizeof(f32) * sound_output_buffer->sample_count))

	f32 seconds_per_sample = 1.0f / (f32)sound_output_buffer->samples_per_second;

	// Clear out the mixer channels
	{
		f32 *dest_0 = real_channel_0;
		f32 *dest_1 = real_channel_1;
		for(i32 sample_index = 0;
			sample_index < sound_output_buffer->sample_count;
			++sample_index)
		{
			*dest_0++ = 0.0f;
			*dest_1++ = 0.0f;
		}
	}

	// Sum all sounds
	for(Playing_Sound **playing_sound_ptr = &audio_state->first_playing_sound;
		*playing_sound_ptr;
		)
	{
		Playing_Sound *playing_sound = *playing_sound_ptr;
		b32 sound_finished = false;

		Loaded_Sound *loaded_sound = playing_sound->id;
		if(loaded_sound)
		{
			// TODO: handle stereo
			V2 current_volume = playing_sound->current_volume;
			V2 dvolume_per_sample = playing_sound->dcurrent_volume_per_second * seconds_per_sample;
			f32 dsample = playing_sound->dsample;

			f32 *dest_0 = real_channel_0;
			f32 *dest_1 = real_channel_1;

			assert(playing_sound->samples_played >= 0.0f);

			u32 samples_to_mix = sound_output_buffer->sample_count;
			f32 real_samples_remaining_in_sound =
				(loaded_sound->sample_count - round_f32_to_i32(playing_sound->samples_played)) / dsample;
			u32 samples_remaining_in_sound = round_f32_to_i32(real_samples_remaining_in_sound);
			if(samples_to_mix > samples_remaining_in_sound)
			{
				samples_to_mix = samples_remaining_in_sound;
			}

			b32 volume_ended[audio_state_output_channel_count] = {};
			for(u32 channel_index = 0;
				channel_index < array_count(volume_ended);
				++channel_index)
			{
				if(dvolume_per_sample.e[channel_index] != 0.0f)
				{
					f32 delta_volume = (playing_sound->target_volume.e[channel_index] -
										current_volume.e[channel_index]);
					u32 volume_sample_count = (u32)((delta_volume / dvolume_per_sample.e[channel_index]) + 0.5f);
					if(samples_to_mix > volume_sample_count)
					{
						samples_to_mix = volume_sample_count;
						volume_ended[channel_index] = true;
					}
				}
			}

			f32 sample_position = playing_sound->samples_played;
			for(u32 loop_index = 0;
				loop_index < samples_to_mix;
				++loop_index)
			{
				// NOTE(Fermin): Can't tell the difference between these two
#if 1
				// NOTE(Fermin): We interpolate between samples for pitch shift.
				u32 sample_index = floor_f32_to_i32(sample_position);
				f32 frac = sample_position - (f32)sample_index;
				f32 sample_0 = (f32)loaded_sound->samples[0][sample_index];
				f32 sample_1 = (f32)loaded_sound->samples[0][sample_index + 1];
				f32 sample_value = lerp(sample_0, frac, sample_1);
#else
				// NOTE(Fermin): We don't interpolate between samples.
				u32 sample_index = round_f32_to_i32(sample_position);
				f32 sample_value = loaded_sound->samples[0][sample_index];
#endif

				*dest_0++ += audio_state->master_volume.e[0] * current_volume.e[0] * sample_value;
				*dest_1++ += audio_state->master_volume.e[1] * current_volume.e[1] * sample_value;

				current_volume += dvolume_per_sample;
				sample_position += dsample;
			}

			playing_sound->current_volume = current_volume;

			for(u32 channel_index = 0;
				channel_index < array_count(volume_ended);
				++channel_index)
			{
				if(volume_ended[channel_index])
				{
					playing_sound->current_volume.e[channel_index] =
						playing_sound->target_volume.e[channel_index];
					playing_sound->dcurrent_volume_per_second.e[channel_index] = 0.0f;
				}
			}

			// NOTE: Shouldnt we add samples played before checking if its finished?
			// This will keep the Playing_Sound for one more frame before freeing it. Why?
			sound_finished = (round_f32_to_i32(playing_sound->samples_played) == loaded_sound->sample_count);
			playing_sound->samples_played = sample_position;
		}
		else
		{
			// TODO: handle playing not loaded sound
		}

		if(sound_finished)
		{
			*playing_sound_ptr = playing_sound->next;
			playing_sound->next = audio_state->first_free_playing_sound;
			audio_state->first_free_playing_sound = playing_sound;
		}
		else
		{
			playing_sound_ptr = &playing_sound->next;
		}
	}

	// convert to 16-bit
	{
		f32 *source_0 = real_channel_0;
		f32 *source_1 = real_channel_1;
		i16 *sample_out = sound_output_buffer->samples;
		for(int sample_index = 0;
			sample_index < sound_output_buffer->sample_count;
			++sample_index)
		{
			*sample_out++ = (i16)clamp(-32768.0f, *source_0++ + 0.5f, 32767.0f);
			*sample_out++ = (i16)clamp(-32768.0f, *source_1++ + 0.5f, 32767.0f);
		}
	}
}

static void
initialize_audio_state(Game_Audio_State *audio_state)
{
	assert(audio_state->sound_buffer.buffer.size != 0)

	audio_state->first_playing_sound = 0;
	audio_state->first_free_playing_sound = 0;
	audio_state->master_volume = V2{1.0f, 1.0f};
}
