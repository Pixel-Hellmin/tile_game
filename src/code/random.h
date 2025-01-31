#if !defined(RANDOM_H)

struct Random_Series
{
    u32 index;
};

inline Random_Series random_seed(u32 value)
{
    Random_Series series;

    series.index = value;

    return series;
}

inline u32 random_next_u32(Random_Series *series)
{
    u32 result = series->index;

    result ^= result << 13;
    result ^= result >> 17;
    result ^= result << 5;
    series->index = result;

    return result;
}

inline f32 random_unilateral(Random_Series *series)
{
    f32 divisor = 1.0f / (f32)U32Max;
    f32 result = divisor * (f32)random_next_u32(series);

    return result;
}

inline f32 random_between(Random_Series *series, f32 min, f32 max)
{
    f32 result = lerp(min, random_unilateral(series), max);

    return result;
}

inline i32 random_between(Random_Series *series, i32 min, i32 max)
{
    i32 result = min + (i32)(random_next_u32(series) % ((max + 1) - min));

    return result;
}


#define RANDOM_H
#endif

