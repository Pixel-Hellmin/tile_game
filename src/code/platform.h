#if !defined(PLATFORM_H)

#include <cstdint>
#include <cstdio>

typedef int16_t    i16;
typedef int32_t    i32;
typedef int64_t    i64;
typedef uint8_t     u8;
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

// NOTE(Fermin): This is used to store u32 as a (void *) type.
// Not actual pointers, useful when using u32 and pointers as
// handles.
#define u32_from_pointer(pointer) ((u32)(size_t)(pointer))
#define pointer_from_u32(type, value) (type *)((size_t)value)

// NOTE(Fermin): This shouldnt be here
global_variable const f32 font_point_size = 64.0f;

#include "math.cpp"
#include "buffer.cpp"

union Tile
{
	// NOTE(Fermin): This is how we store tiles in the game memory. Later these
	// are processed into Quads for rendering.
    // NOTE(Fermin): This are drawn as 2d rects, I'm thinking using the Z
    // coord as an indicator of the 'floor' where this rect lives
    /* NOTE(Fermin): Is this better?
    * f32 width;
    * f32 height;
    * V3 origin;
    * u32 tex_id;
    */
    // NOTE(Fermin): If we update this struct remember to also update push_rectangle
    /*
    V3 min_p;
    V3 max_p;
    */
    // TODO(Fermin): Union world_index -> screen_coords
	// TODO(Fermin): Tiles should only exist in game. Move this to game.h
    struct {
        V3 world_index;
        V2 dim_in_tiles;
        V4 color;
        u32 texture_id;
        f32 rotation;
    };
    struct {
        V3 pos_in_screen;
        V2 dim_in_px;
        V4 color;
        u32 texture_id;
        f32 rotation;
    };
};

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
    V2 cursor; // NOTE(Fermin): (-1, -1) to (1, 1)
};

struct Camera // here for now. Move to game
{
    V3 pos;
};

struct Particle // here for now. Move to game
{
    V3 p;
    V3 d_p;
    V4 color_trans;
    V4 d_color;
    f32 rotation;
    f32 d_rotation;
};

// NOTE(Fermin): This is here for now. Will move to game.h eventually
#include "random.h"


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

enum Game_State_Debug_Flags // here for now. Move to game
{
    game_state_flag_prints = (1 << 1),
    game_state_flag_free_cam_mode = (1 << 2),
};

struct Game_State // here for now. Move to game
{
    f32 dt_in_seconds;
    Random_Series entropy;

    u32 debug_flags;
    f32 tile_size_in_px;

    Camera camera;

    i32 window_width;
    i32 window_height;

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

	Font debug_font_consola;
};

struct Game_Sound_Output_Buffer
{
	i32 samples_per_second;
	i32 sample_count;
	i16 *samples;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Render_Buffer *tiles_buffer, Render_Buffer *ui_buffer, Tile *dude, Game_State *game_state, Render_Buffer *render_buffer, Input_Keys *input)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
GAME_UPDATE_AND_RENDER(game_update_and_render_stub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name(Game_Sound_Output_Buffer *sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples);
GAME_GET_SOUND_SAMPLES(game_get_sound_samples_stub)
{
}

static u32
push_quad(Render_Buffer *render_buffer, V3 *corners, u32 texture_id, V4 color)
{
    u32 result = render_buffer->count;

    // NOTE(Fermin): Check if we have enough space for another Quad
    assert((result+1) * sizeof(Quad) <= render_buffer->buffer.count);

    Quad *pushed_quad = (Quad *)render_buffer->buffer.data + render_buffer->count++;
    pushed_quad->corners[0] = corners[0];
    pushed_quad->corners[1] = corners[1];
    pushed_quad->corners[2] = corners[2];
    pushed_quad->corners[3] = corners[3];
    pushed_quad->texture_id = texture_id;
    pushed_quad->color = color;

    return result;
}

static u32
push_tile(Render_Buffer *render_buffer, Tile *tile, V4 color = {1.0, 1.0, 1.0, 1.0}) // TODO(Fermin): Move this to game
{
    // NOTE(Fermin): This is error prone since we have to update this function each time we change Tile struct
    static_assert(sizeof(Tile) == 44, "Pushing out of date Tile");

    u32 result = render_buffer->count;

    // NOTE(Fermin): Check if we have enough space for another Tile
    assert((result+1) * sizeof(Tile) <= render_buffer->buffer.count);

    Tile *pushed_tile = (Tile *)render_buffer->buffer.data + render_buffer->count++;
    pushed_tile->world_index = tile->world_index;
    pushed_tile->dim_in_tiles = tile->dim_in_tiles;
    pushed_tile->color = color;
    pushed_tile->texture_id = tile->texture_id;
    pushed_tile->rotation = tile->rotation;

    return result;
}

static void
get_character_metadata(char character, Glyph_Metadata *out, Font *font) // here for now. where should it go?
{
    // TODO(Fermin): Pass Font so it works when we add more fonts
    size_t index = character - font_first_character;
    Glyph_Metadata character_info = font->metadata[index];

    out->width = character_info.width;
    out->height = character_info.height;
    out->offset = character_info.offset;
    out->y_offset = character_info.y_offset;
    out->advance = character_info.advance;
}

global_variable f32 debug_print_line = 0.0f;
static void
print_debug_text(char *string, Font *font, Render_Buffer *ui_buffer, V4 color = {0.0f, 1.0f, 0.0f, 1.0f}) // here for now. move to where? platform?
{
    f32 print_font_size = 24.0f;
    debug_print_line -= print_font_size;

    f32 x = 10.0f;
    f32 line_y = debug_print_line;
    f32 max_scale = print_font_size / font_point_size;
    f32 space_width = font_point_size / 2.0f * max_scale;

    for(char *c = string; *c; c++)
    {
        if(*c != ' ')
        {
            Glyph_Metadata glyph_info = {};
            get_character_metadata(*c, &glyph_info, font);

            f32 y_scale = glyph_info.height / font_point_size;
            f32 x_scale = glyph_info.width / font_point_size;
            f32 h = print_font_size * y_scale;
            f32 w = print_font_size * x_scale;
            f32 y = line_y - glyph_info.y_offset * max_scale;
            f32 advance = glyph_info.advance * max_scale;


            Tile glyph = {};
            glyph.pos_in_screen.xy = V2{x, y};
            glyph.dim_in_px = V2{w, h};
            glyph.texture_id = font->glyph_texture_ids[*c - font_first_character];

            push_tile(ui_buffer, &glyph, color);

            x += advance;
        }
        else
        {
            x += space_width;
        }
    }
}

#define PLATFORM_H
#endif

