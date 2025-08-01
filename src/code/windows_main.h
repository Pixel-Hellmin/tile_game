/*
 * COMBACK:
 * - Catch up: 358w, 359w, 360, 361, 362, 363?, 364
 * TODO(Fermin):
 * - RNG! search for std::random_device rd;
 * - Store tile indices for dude and for highlighted tile in game state instead of 
 *   what we are doing now. I guess we can follow this logic when we need a tile in the 
 *   platform layer?
 * - Investigate FileSystem::getPath("resources/textures/container.jpg"
 * - Global Managers?
 * - Investigate why are the boxes deformed when rotated?
 * - Fix font bearings
 * - Get rid of vc140.pdb when building
 * - Map each input key to an action and make it remapable
 * -  Use newtons laws for motion?
 * - Game_State struct redefinition
*/

#if !defined(MAIN_H)

#include <cstdio>
#include <cmath>
#include <cstdint>

// NOTE(Fermin): Include glad before glfw3
#if 0
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glad.c"
#endif

typedef int32_t    i32;

typedef uint8_t    u8;
typedef uint32_t   u32;
typedef uint64_t   u64;
typedef int32_t    b32;
typedef uintptr_t  umm;

typedef float      f32;
typedef double     f64;

#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#define invalid_code_path assert(!"invalid_code_path")

#define global_variable static

#define Pi32 3.14159265359f
#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define U32Max ((u32) - 1)

// NOTE(Fermin): This is used to store u32 as a (void *) type.
// Not actual pointers, useful when using u32 and pointers as
// handles.
#define u32_from_pointer(pointer) ((u32)(size_t)(pointer))
#define pointer_from_u32(type, value) (type *)((size_t)value)

global_variable f32 debug_print_line = 0.0f;
global_variable const f32 font_point_size = 64.0f;
global_variable const u32 font_first_character = '!';
global_variable const u32 font_last_character = '~';
global_variable const u32 font_character_count = font_last_character - font_first_character + 1;

#include "math.cpp"
#include "buffer.cpp"
#include "random.h"

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
    u32 cached;
    Buffer buffer;
};

struct Rect
{
    // NOTE(Fermin): This are drawn as 2d rects, I'm thinking using the Z
    // coord as an indicator of the 'floor' where this rect lives
    /* NOTE(Fermin): Is this better?
    * f32 width;
    * f32 height;
    * V3 origin;
    * u32 tex_id;
    */
    // NOTE(Fermin): If we update this struct remember to also update push_rectangle
    V3 min_p;
    V3 max_p;
    V4 color;
    u32 texture_id;
    f32 rotation;
};

u32 push_rectangle(Render_Buffer *render_buffer, Rect *rect, V4 color = {1.0, 1.0, 1.0, 1.0})
{
    // NOTE(Fermin): This is error prone since we have to update this function each time we change Rect struct
    static_assert(sizeof(Rect) == 48, "Pushing out of date Rect");

    u32 result = render_buffer->count;

    assert((result+1) * sizeof(Rect) <= render_buffer->buffer.count);

    Rect *pushed_rect = (Rect *)render_buffer->buffer.data + render_buffer->count++;
    pushed_rect->min_p = rect->min_p;
    pushed_rect->max_p = rect->max_p;
    pushed_rect->color = color;
    pushed_rect->texture_id = rect->texture_id;
    pushed_rect->rotation = rect->rotation;

    return result;
}

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

struct Particle
{
    V3 p;
    V3 d_p;
    V4 color_trans;
    V4 d_color;
    f32 rotation;
    f32 d_rotation;
};

struct Game_State
{
    f32 delta;
    Random_Series entropy;

    Input_Keys input_state;
    Input_Keys last_frame_input_state;

    u32 debug_flags;

    Camera camera;
    M4 *proj;
    M4 *view;
    f32 tile_size_in_meters;

    // TODO(Fermin): Store tile index into Render_Buffer instead of this
    b32 editing_tile;
    i32 editing_tile_x;
    i32 editing_tile_y;

    i32 level_rows;
    i32 level_cols;

    u32 highlight_texture_id;
    u32 floor_texture_id;
    u32 wall_texture_id;
    u32 roof_texture_id;

    b32 initialized;

    // NOTE(Fermin): Testing particle system stuff. Move this to where it makes sense
    u32 next_particle;
    Particle particles[64];
};

#define GAME_UPDATE_AND_RENDER(name) void name(Render_Buffer *tiles_buffer, Rect *dude, Game_State *game_state)
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
