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

struct Win32_Game_Code
{
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;
    Game_Update_And_Render *update_and_render;
    Game_Get_Sound_Samples *get_sound_samples;

    b32 is_valid;
};

#define MAIN_H
#endif
