#pragma pack(push, 1)
struct WAVE_Header
{
	u32 RIFFID;
	u32 size;
	u32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
enum
{
	WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
	WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
	WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
	WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct WAVE_Chunk
{
	u32 id;
	u32 size;
};

struct WAVE_Fmt
{
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	u16 cbSize;
	u16 wValidBitsPerSample;
	u32 dwChannelMask;
	u8 SubFormat[16];
};
#pragma pack(pop)

struct Riff_Iterator
{
	u8 *at;
	u8 *stop;
};

inline Riff_Iterator
parse_chunk_at(void *at, void *stop)
{
	Riff_Iterator iter;

	iter.at = (u8 *)at;
	iter.stop = (u8 *)stop;

	return iter;
}

inline b32
is_valid(Riff_Iterator iter)
{
	b32 result = (iter.at < iter.stop);

	return result;
}

inline Riff_Iterator
next_chunk(Riff_Iterator iter)
{
	WAVE_Chunk *chunk = (WAVE_Chunk *)iter.at;
	u32 size = (chunk->size + 1) & ~1; // NOTE(Fermin): Add one if size is odd
	iter.at += sizeof(WAVE_Chunk) + size;

	return iter;
}

inline u32
get_type(Riff_Iterator iter)
{
	WAVE_Chunk *chunk = (WAVE_Chunk *)iter.at;
	u32 result = chunk->id;

	return result;
}

inline void *
get_chunk_data(Riff_Iterator iter)
{
	void *result = (iter.at + sizeof(WAVE_Chunk));

	return result;
}

inline u32
get_chunk_data_size(Riff_Iterator iter)
{
	WAVE_Chunk *chunk = (WAVE_Chunk *)iter.at;
	u32 result = chunk->size;

	return result;
}

static Loaded_Sound
DEBUG_load_WAV(char *file_name)
{
	// NOTE(Fermin): This would go in the asset pipeline
	Loaded_Sound result = {};

	Buffer read_result = read_file(file_name);
	if(read_result.data)
	{
		WAVE_Header *header = (WAVE_Header *)read_result.data;
		assert(header->RIFFID == WAVE_ChunkID_RIFF);
		assert(header->WAVEID == WAVE_ChunkID_WAVE);

		u32 channel_count = 0;
		u32 sample_data_size = 0;
		i16 *sample_data = 0;
		for(Riff_Iterator iter = parse_chunk_at(header + 1, (u8 *)(header + 1) + header->size - 4);
			is_valid(iter);
			iter = next_chunk(iter))
		{
			switch(get_type(iter))
			{
				case WAVE_ChunkID_fmt:
				{
					WAVE_Fmt *fmt = (WAVE_Fmt *)get_chunk_data(iter);
					assert(fmt->wFormatTag == 1); // NOTE: only support PCM
					//assert(fmt->nSamplesPerSec == 48000); // TODO(Fermin): Enable
					assert(fmt->wBitsPerSample == 16);
					assert(fmt->nBlockAlign == (sizeof(i16)*fmt->nChannels));
					channel_count = fmt->nChannels;
				} break;

				case WAVE_ChunkID_data:
				{
					sample_data = (i16 *)get_chunk_data(iter);
					sample_data_size = get_chunk_data_size(iter);
				} break;
			}
		}

		assert(channel_count && sample_data);

		result.channel_count = channel_count;
		result.sample_count = sample_data_size / (channel_count * sizeof(i16));
		if(channel_count == 1)
		{
			result.samples[0] = sample_data;
			result.samples[1] = 0;
		}
		else if(channel_count == 2)
		{
			result.samples[0] = sample_data;
			result.samples[1] = sample_data + result.sample_count;

			for(u32 sample_index = 0;
				sample_index < result.sample_count;
				++sample_index)
			{
				// TODO: uninterleave the samples
				i16 source = sample_data[2*sample_index];
				sample_data[2*sample_index] = sample_data[sample_index];
				sample_data[sample_index] = source;
			}
		}
		else
		{
			assert(!"invalid channel count in WAV file");
		}

		result.channel_count = 1; // TODO: load right channels
	}

	return result;
}
