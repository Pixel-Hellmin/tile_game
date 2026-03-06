/*
 * COMBACK:
 * - Catch up: 358w, 359w, 360, 361, 362, 363?, 364
 * TODO(Fermin):
 * + Platform struct to pass into the game.
 *	 We can add info like Window, Keys, Etc...
 *	 This data can get updated in(or out) game and the platform reacts.
 * + Instead of:
 *       if(input_state.w)
 *       {
 *           d_pos.y = 1.0f; 
 *       }
 *   Try something like:
 *	 d_pos.y = input_state.w * speed * delta;
 *	 or
 *	 d_pos.y = input_state.w.is_down * speed * delta;
 *	 Store states for keys?
 *	 is_down, was_pressed, was_released
 *
 * - RNG! search for std::random_device rd;
 * - Store tile indices for dude and for highlighted tile in game state instead of 
 *   what we are doing now. I guess we can follow this logic when we need a tile in the 
 *   platform layer?
 * - Investigate FileSystem::getPath("resources/textures/container.jpg"
 * - Investigate why are the boxes deformed when rotated?
 * - Fix font bearings
 * - Get rid of vc140.pdb when building
 * - Map each input key to an action and make it remapable
 * -  Use newtons laws for motion?
 * - Game_State struct redefinition
*/

#if !defined(MAIN_H)

#include "platform.h"

// NOTE(Fermin): Windows specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002


#define PROFILER 1
//#define READ_BLOCK_TIMER read_OS_timer
#include "profiler.cpp"

struct Win32_Offscreen_Buffer
{
    // NOTE(Fermin): What part does this back buffer plays when rendering with
    // OpenGL? Is the memory still needed if we never write to it?
    BITMAPINFO info;
    void *memory;
    i32 width;
    i32 height;
    i32 pitch;
    i32 bytes_per_pixel;
};

struct Win32_Sound_Output
{
	i32 samples_per_second;
	u32 running_sample_index;
	i32 bytes_per_sample;
	DWORD secondary_buffer_size;
	DWORD safety_bytes;
};

struct Win32_Window_Dimension
{
    i32 width;
    i32 height;
};

struct Glyph_Metadata
{
    // NOTE(Fermin): advance has left side bearing calculated already
    size_t offset;
    i32 width;
    i32 height;
    i32 y_offset;
    i32 advance;
};

// NOTE(Fermin): Where should these be?
global_variable const u32 font_first_character = '!';
global_variable const u32 font_last_character = '~';
global_variable const u32 font_character_count = font_last_character - font_first_character + 1;
struct Font
{
    u32 glyph_texture_ids[font_character_count];
    Glyph_Metadata metadata[font_character_count];
};


enum Game_State_Debug_Flags
{
    game_state_flag_prints = (1 << 1),
    game_state_flag_free_cam_mode = (1 << 2),
};

/* moved to platform - delete
struct Game_Sound_Output_Buffer
{
	i32 samples_per_second;
	i32 sample_count;
	i16 *samples;
};

#define GAME_GET_SOUND_SAMPLES(name) void name(Game_Sound_Output_Buffer *sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples);
GAME_GET_SOUND_SAMPLES(game_get_sound_samples_stub)
{
}
*/

struct Win32_Game_Code
{
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;
    Game_Update_And_Render *update_and_render;
    Game_Get_Sound_Samples *get_sound_samples;

    b32 is_valid;
};

static u32
push_rectangle(Render_Buffer *render_buffer, Rect *rect, V4 color = {1.0, 1.0, 1.0, 1.0})
{
    // NOTE(Fermin): This is error prone since we have to update this function each time we change Rect struct
    static_assert(sizeof(Rect) == 44, "Pushing out of date Rect");

    u32 result = render_buffer->count;

    // NOTE(Fermin): Check if we have enough space for another Rect
    assert((result+1) * sizeof(Rect) <= render_buffer->buffer.count);

    Rect *pushed_rect = (Rect *)render_buffer->buffer.data + render_buffer->count++;
    pushed_rect->world_index = rect->world_index;
    pushed_rect->dim_in_tiles = rect->dim_in_tiles;
    pushed_rect->color = color;
    pushed_rect->texture_id = rect->texture_id;
    pushed_rect->rotation = rect->rotation;

    return result;
}

#define MAIN_H
#endif
