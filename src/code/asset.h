#if !defined(ASSET_H)

struct Loaded_Sound
{
    u32 sample_count;
    u32 channel_count;
    i16 *samples[2];
};

#define ASSET_H
#endif
