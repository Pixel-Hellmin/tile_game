/*
 * TODO(Fermin):
 * - Investigate FileSystem::getPath("resources/textures/container.jpg"
 * - Global Managers?
 * - Investigate why are the boxes deformed when rotated?
 * - Recalculate projection matrices only when parameters change instead
 *   of every frame
 * - Fix font bearings
 *
 * - Get rid of vc140.pdb when building
*/

#if !defined(MAIN_H)

#include <cstdio>
#include <cmath>

// NOTE(Fermin): Include glad before glfw3
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glad.c"

typedef int32_t  i32;

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  b32;

typedef float    f32;
typedef double   f64;

#define global_variable static

#define Pi32 3.14159265359f
#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)

global_variable f32 debug_print_line = 0.0f;
global_variable const f32 font_point_size = 64.0f;
global_variable const u32 font_first_character = '!';
global_variable const u32 font_last_character = '~';
global_variable const u32 font_character_count = font_last_character - font_first_character + 1;

#include "math.cpp"
#include "buffer.cpp"

struct Glyph_Metadata
{
    // NOTE(Fermin): advance has left side bearing calculated already
    size_t offset;
    i32 width;
    i32 height;
    i32 y_offset;
    i32 advance;
};

struct Font
{
    u32 glyph_texture_ids[font_character_count];
    Glyph_Metadata metadata[font_character_count];
};

struct Render_Buffer
{
    // NOTE(Fermin): May need to eventually add a type here when
    // we have more than one type of render object
    u32 count;
    Buffer buffer;
};

struct Rect
{
    // NOTE(Fermin): This are drawn as 2d rects, I'm thinking using the Z
    // coord as an indicator of the 'floor' where this rect lives
    // NOTE(Fermin): Maybe we can just store the tile index and the 
    // width and height instead of its points
    V3 min_p;
    V3 max_p;
    V4 color;
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
    V2 cursor; // NOTE(Fermin): (-1, -1) to (1, 1)
};

enum Game_State_Debug_Flags
{
    game_state_flag_prints = (1 << 1),
    game_state_flag_wireframe_mode = (1 << 2),
    game_state_flag_free_cam_mode = (1 << 3),
};

struct Camera
{
    V3 pos;
    V3 up;
    V3 front;
};

struct Game_State
{
    f32 delta;

    Input_Keys input_state;
    Input_Keys last_frame_input_state;

    u32 debug_flags;

    Camera camera;
    M4 *proj;
    M4 *view;
    f32 tile_size_in_meters;

    b32 editing_tile;
    i32 editing_tile_x;
    i32 editing_tile_y;

    i32 level_rows;
    i32 level_cols;

    b32 initialized;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Render_Buffer *world_tiles, Rect *dude, Game_State *game_state, Render_Buffer *debug)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
GAME_UPDATE_AND_RENDER(game_update_and_render_stub)
{
}

struct Game_Code
{
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;
    Game_Update_And_Render *update_and_render;

    b32 is_valid;
};

#define MAIN_H
#endif
