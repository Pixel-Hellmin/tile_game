#if !defined(PLATFORM_H)

#include <cstdint>
#include <cstdio>

// NOTE(Fermin): Intel Intrinsics Guide
// NOTE(Fermin): support others?
#include <xmmintrin.h>
#include <emmintrin.h>

typedef int16_t    i16;
typedef int32_t    i32;
typedef int64_t    i64;
typedef uint8_t     u8;
typedef uint16_t   u16;
typedef uint32_t   u32;
typedef uint64_t   u64;
typedef uintptr_t  umm;
typedef float      f32;
typedef double     f64;
typedef int32_t    b32;

#define global_variable static

#define Pi32 3.14159265359f

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)

#define U32Max ((u32) - 1)

#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#define invalid_code_path assert(!"invalid_code_path")
#define array_count(array) (sizeof(array) / sizeof((array)[0]))
#define align_pow2(value, alignment) ((value + ((alignment) - 1)) & ~((alignment) - 1))
#define align4(value) align_pow2(value, 4)

// NOTE(Fermin): This is used to store u32 as a (void *) type.
// Not actual pointers, useful when using u32 and pointers as
// handles.
#define u32_from_pointer(pointer) ((u32)(size_t)(pointer))
#define pointer_from_u32(type, value) (type *)((size_t)value)

// NOTE(Fermin): This shouldnt be here
global_variable const f32 font_point_size = 64.0f;

#include "math.cpp"
#include "buffer.cpp"

struct Quad
{
	// NOTE(Fermin): This is how we store tiles in the Render_Buffer.
	// Ready to pass down to opengl
	V3 corners[4];
	V4 color;
	i32 texture_id;
};

struct Render_Buffer
{
    // NOTE(Fermin): May need to eventually add a type here when we have more than one type of render object.
    // For now we only store Quads.
    u32 count;
    u32 cached;
    Buffer buffer;
};

struct Input_Keys
{
    b32 w;
    b32 a;
    b32 s;
    b32 d;
    b32 left_mouse;
    b32 f1;
    b32 f2;
    b32 f3;
    f32 dt_in_seconds;
    V2 cursor; // NOTE(Fermin): (-1, -1) to (1, 1)
};

struct Glyph_Metadata // here for now. where should it go?
{
    // NOTE(Fermin): advance has left side bearing calculated already
    size_t offset;
    i32 width;
    i32 height;
    i32 y_offset;
    i32 advance;
};

global_variable const u32 font_first_character = '!';
global_variable const u32 font_last_character = '~';
global_variable const u32 font_character_count = font_last_character - font_first_character + 1;
struct Font // here for now. where should it go?
{
    u32 glyph_texture_ids[font_character_count];
    Glyph_Metadata metadata[font_character_count];
};

struct Game_Sound_Output_Buffer
{
	i32 samples_per_second;
	i32 sample_count;
	i16 *samples;
};

struct Game_Memory
{
	// USE this instead of all the buffers in win layer
    Buffer permanent_storage; // REQUIRED to be cleared to zero at startup
    Buffer temporary_storage; // REQUIRED to be cleared to zero at startup

	i32 window_width;
	i32 window_height;

	// debug assets
    u32 highlight_texture_id;
    u32 floor_texture_id;
    u32 wall_texture_id;
    u32 roof_texture_id;
	u32 dude_texture_id;
	Font debug_font_consola;

	b32 debug_initialized;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Game_Memory *game_memory, Render_Buffer *render_buffer, Input_Keys *input)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
GAME_UPDATE_AND_RENDER(game_update_and_render_stub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name(Game_Memory *game_memory, Game_Sound_Output_Buffer *sound_output_buffer)
typedef GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples);
GAME_GET_SOUND_SAMPLES(game_get_sound_samples_stub)
{
}

#define PLATFORM_H
#endif

