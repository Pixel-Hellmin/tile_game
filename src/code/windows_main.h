/*
 * COMBACK:
 * - Catch up: 358w, 359w, 360, 361, 362, 363?, 364
 * TODO(Fermin):
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
 * - Investigate FileSystem::getPath("resources/textures/container.jpg"
 * - Fix font bearings
 * - Get rid of vc140.pdb when building
*/

/*
 * Ideas from DOOM
 * @Cleanup - Init each 'system'(sound, window, etc) and exit early if errors instead of nesting.
 * @Cleanup - Combine buffer.cpp and memory.h? Get rid of buffer?
 * @Cleanup - Load assets, including fonts from game only. font.cpp
 * @ Use command line args for things
 *
 * @Graphics
 * - Sectors: flat floor/ceiling heights. No slopes.
 * - Linedefs/Sidedefs: walls between sectors, upper/lower/middle textures
 * - BSP
 *		
 *
 *			  Linedef (v1 -> v2)
 *						v1
 *						|
 *						|
 *						|
 *						|
 *		Sector A		|		Sector B
 *		Light 200		|		Light 160, floor 24
 *		Front sidedef	|		Back sidedef
 *						|
 *						|
 *					   \ /
 *					    V
 *						v2
*
* the linedef's front sidedef's sector == A → edge direction is v1 → v2
* the linedef's back sidedef's sector == A → edge direction is v2 → v1 (reversed, since sector S is on the "back" side)
*/


#if !defined(MAIN_H)

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
