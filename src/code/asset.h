#if !defined(ASSET_H)

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

struct Loaded_Sound
{
    u32 sample_count;
    u32 channel_count;
    i16 *samples[2];
};

#define ASSET_H
#endif
