static void
change_pitch(Game_Audio_State *audio_state, Playing_Sound *sound, f32 dsample)
{
	// NOTE(Fermin): To avoid precision errors we should try to switch the
	// pitch by a power of 2.
	sound->dsample = dsample;
}

static Playing_Sound*
play_sound(Game_Audio_State *audio_state, Loaded_Sound *sound_id)
{
	// Create the idea of sound ids instead of Loaded_Sound
	if(!audio_state->first_free_playing_sound)
	{
		audio_state->first_free_playing_sound = push_struct(&audio_state->arena, Playing_Sound);
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
output_playing_sounds(Game_Audio_State *audio_state, Game_Sound_Output_Buffer *sound_output_buffer, Memory_Arena *tmp_arena)
{
	assert((tmp_arena->size - tmp_arena->used) >= (sizeof(f32) * sound_output_buffer->sample_count * audio_state_output_channel_count))

	Tmp_Memory mixer_memory = begin_tmp_memory(tmp_arena);

	assert((sound_output_buffer->sample_count & 3) == 0);
	u32 samples_per_chunk = 4;
	u32 chunk_count = sound_output_buffer->sample_count / samples_per_chunk;

	__m128 *real_channel_0 = push_array(tmp_arena, chunk_count, __m128, 4);
	__m128 *real_channel_1 = push_array(tmp_arena, chunk_count, __m128, 4);

	f32 seconds_per_sample = 1.0f / (f32)sound_output_buffer->samples_per_second;

	// Clear out the mixer channels
	__m128 one = _mm_set1_ps(1.0f);
	__m128 zero = _mm_set1_ps(0.0f);
	{
		__m128 *dest_0 = real_channel_0;
		__m128 *dest_1 = real_channel_1;
		for(u32 sample_index = 0;
			sample_index < chunk_count;
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
			V2 dvolume_per_chunk = dvolume_per_sample * (f32)samples_per_chunk;
			f32 dsample = playing_sound->dsample;
			f32 dsample_per_chunk = playing_sound->dsample * (f32)samples_per_chunk;

			__m128 *dest_0 = real_channel_0;
			__m128 *dest_1 = real_channel_1;

			// channel 0
			__m128 master_volume_0 = _mm_set1_ps(audio_state->master_volume.e[0]);
			__m128 volume_0 = _mm_setr_ps(current_volume.e[0] + 0.0f * dvolume_per_sample.e[0],
										  current_volume.e[0] + 1.0f * dvolume_per_sample.e[0],
										  current_volume.e[0] + 2.0f * dvolume_per_sample.e[0],
										  current_volume.e[0] + 3.0f * dvolume_per_sample.e[0]);
			__m128 dvolume_0 = _mm_set1_ps(dvolume_per_sample.e[0]);
			__m128 dvolume_chunk_0 = _mm_set1_ps(dvolume_per_chunk.e[0]);

			// channel 1
			__m128 master_volume_1 = _mm_set1_ps(audio_state->master_volume.e[1]);
			__m128 volume_1 = _mm_setr_ps(current_volume.e[1] + 0.0f * dvolume_per_sample.e[1],
										  current_volume.e[1] + 1.0f * dvolume_per_sample.e[1],
										  current_volume.e[1] + 2.0f * dvolume_per_sample.e[1],
										  current_volume.e[1] + 3.0f * dvolume_per_sample.e[1]);
			__m128 dvolume_1 = _mm_set1_ps(dvolume_per_sample.e[1]);
			__m128 dvolume_chunk_1 = _mm_set1_ps(dvolume_per_chunk.e[1]);

			assert(playing_sound->samples_played >= 0.0f);

			u32 chunks_to_mix = chunk_count;
			f32 real_chunks_remaining_in_sound =
				(loaded_sound->sample_count - round_f32_to_i32(playing_sound->samples_played)) / dsample_per_chunk;
			u32 chunks_remaining_in_sound = round_f32_to_i32(real_chunks_remaining_in_sound);
			if(chunks_to_mix > chunks_remaining_in_sound)
			{
				chunks_to_mix = chunks_remaining_in_sound;
			}

			b32 volume_ended[audio_state_output_channel_count] = {};
			for(u32 channel_index = 0;
				channel_index < array_count(volume_ended);
				++channel_index)
			{
				if(dvolume_per_chunk.e[channel_index] != 0.0f)
				{
					f32 delta_volume = (playing_sound->target_volume.e[channel_index] -
										current_volume.e[channel_index]);
					u32 volume_chunk_count = (u32)((delta_volume / dvolume_per_chunk.e[channel_index]) + 0.5f);
					if(chunks_to_mix > volume_chunk_count)
					{
						// NOTE(Fermin): There is a bug here. We can't cut the chunks_to_mix short if the change
						// in volume ends but the sound is still playing, we hear clipping.
						// On the other hand if we mix more chunks than the chunks needed for the volume change
						// we will overshoot the change in volume.
						// I rather have the change in volume not be precise than clipping.
						
						//chunks_to_mix = volume_chunk_count;
						volume_ended[channel_index] = true;
					}
				}
			}

			f32 begin_sample_position = playing_sound->samples_played;
			f32 end_sample_position = begin_sample_position + chunks_to_mix * dsample_per_chunk;
			for(u32 loop_index = 0;
				loop_index < chunks_to_mix;
				++loop_index)
			{
				f32 sample_position = begin_sample_position + dsample_per_chunk * (f32)loop_index;

#if 1			// NOTE(Fermin): Can't tell the difference between these two
				// NOTE(Fermin): We interpolate between samples for pitch shift.

				__m128 sample_pos = _mm_setr_ps(sample_position + 0.0f * dsample,
												sample_position + 1.0f * dsample,
												sample_position + 2.0f * dsample,
												sample_position + 3.0f * dsample);
				__m128i sample_index = _mm_cvttps_epi32(sample_pos);
				__m128 frac = _mm_sub_ps(sample_pos, _mm_cvtepi32_ps(sample_index));

				__m128 sample_value_f = _mm_setr_ps(loaded_sound->samples[0][((i32 *)&sample_index)[0]],
												    loaded_sound->samples[0][((i32 *)&sample_index)[1]],
												    loaded_sound->samples[0][((i32 *)&sample_index)[2]],
												    loaded_sound->samples[0][((i32 *)&sample_index)[3]]);
				__m128 sample_value_c = _mm_setr_ps(loaded_sound->samples[0][((i32 *)&sample_index)[0] + 1],
													loaded_sound->samples[0][((i32 *)&sample_index)[1] + 1],
													loaded_sound->samples[0][((i32 *)&sample_index)[2] + 1],
													loaded_sound->samples[0][((i32 *)&sample_index)[3] + 1]);

				__m128 sample_value = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, frac), sample_value_f),
												 _mm_mul_ps(frac, sample_value_c));
#else
				// NOTE(Fermin): We don't interpolate between samples.
				__m128 sample_value = _mm_setr_ps(loaded_sound->samples[0][round_f32_to_i32(sample_position + 0.0f * dsample)],
												  loaded_sound->samples[0][round_f32_to_i32(sample_position + 1.0f * dsample)],
												  loaded_sound->samples[0][round_f32_to_i32(sample_position + 2.0f * dsample)],
												  loaded_sound->samples[0][round_f32_to_i32(sample_position + 3.0f * dsample)]);
#endif

				__m128 d0 = _mm_load_ps((f32 *)&dest_0[0]);
				__m128 d1 = _mm_load_ps((f32 *)&dest_1[0]);

				d0 = _mm_add_ps(d0, _mm_mul_ps(_mm_mul_ps(master_volume_0, volume_0), sample_value));
				d1 = _mm_add_ps(d1, _mm_mul_ps(_mm_mul_ps(master_volume_1, volume_1), sample_value));

				_mm_store_ps((f32 *)&dest_0[0], d0);
				_mm_store_ps((f32 *)&dest_1[0], d1);

				++dest_0;
				++dest_1;
				volume_0 = _mm_add_ps(volume_0, dvolume_chunk_0);
				volume_1 = _mm_add_ps(volume_1, dvolume_chunk_1);
			}

			playing_sound->current_volume.e[0] = ((f32 *)&volume_0)[0];
			playing_sound->current_volume.e[1] = ((f32 *)&volume_1)[0];

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

			playing_sound->samples_played = end_sample_position;
			assert(chunk_count >= chunks_to_mix);
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
			sample_index < chunk_count;
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

	end_tmp_memory(mixer_memory);
}

static void
initialize_audio_state(Game_Audio_State *audio_state)
{
	assert(audio_state->arena.size != 0)

	audio_state->first_playing_sound = 0;
	audio_state->first_free_playing_sound = 0;
	audio_state->master_volume = V2{1.0f, 1.0f};
}
