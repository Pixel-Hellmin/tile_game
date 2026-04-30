static void
change_pitch(Game_Audio_State *audio_state, Playing_Sound *sound, f32 dsample)
{
	// NOTE(Fermin): To avoid precision errors we should try to switch the
	// pitch by a power of 2.
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
	u32 sample_count_4_aligned = align4(sound_output_buffer->sample_count);
	u32 sample_count_by_4 = sample_count_4_aligned / 4;

	// TODO(Fermin): Memory_Arena
	size_t alignment_offset = get_alignment_offset((size_t)temp_storage, 4);
	__m128 *real_channel_0 = (__m128 *)(((size_t *)temp_storage) + alignment_offset);
	__m128 *real_channel_1 = real_channel_0 + sizeof(__m128)*sample_count_by_4;

	//f32 *real_channel_0 = (f32 *)(((size_t *)temp_storage) + alignment_offset);
	//f32 *real_channel_1 = real_channel_0 + sizeof(f32)*sound_output_buffer->sample_count;

	assert((real_channel_1 - real_channel_0) == (sizeof(__m128) * sample_count_by_4))

	f32 seconds_per_sample = 1.0f / (f32)sound_output_buffer->samples_per_second;

	// Clear out the mixer channels
	__m128 zero = _mm_set1_ps(0.0f);
	{
		__m128 *dest_0 = real_channel_0;
		__m128 *dest_1 = real_channel_1;
		for(u32 sample_index = 0;
			sample_index < sample_count_by_4;
			++sample_index)
		{
			_mm_store_ps((float *)dest_0++, zero);
			_mm_store_ps((float *)dest_1++, zero);
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

			f32 *dest_0 = (f32 *)real_channel_0;
			f32 *dest_1 = (f32 *)real_channel_1;

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

			for(u32 loop_index = 0;
				loop_index < samples_to_mix;
				++loop_index)
			{
				f32 sample_position = playing_sound->samples_played + (loop_index * dsample);
#if 1			// NOTE(Fermin): Can't tell the difference between these two
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

			playing_sound->samples_played += samples_to_mix * dsample;
			sound_finished = ((u32)playing_sound->samples_played == loaded_sound->sample_count);
			assert((u32)playing_sound->samples_played <= loaded_sound->sample_count)
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
		__m128 *source_0 = real_channel_0;
		__m128 *source_1 = real_channel_1;

		__m128i *sample_out = (__m128i *)sound_output_buffer->samples;
		for(u32 sample_index = 0;
			sample_index < sample_count_by_4;
			++sample_index)
		{
			// load
			__m128 s0 = _mm_load_ps((f32 *)source_0++);
			__m128 s1 = _mm_load_ps((f32 *)source_1++);

			// convert from float to int
			__m128i left =  _mm_cvtps_epi32(s0);
			__m128i right = _mm_cvtps_epi32(s1);

			// unpack and interleave 32-bit ints from the low half
			__m128i left_right0 = _mm_unpacklo_epi32(left, right);
			// unpack and interleave 32-bit ints from the high half
			__m128i left_right1 = _mm_unpackhi_epi32(left, right);

			// convert packed 32-bit ints to packed 16-bit ints using signed saturation
			__m128i s01 = _mm_packs_epi32(left_right0, left_right1);

			// no need to clamp because convertion is saturated 
			*sample_out++ = s01;
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
