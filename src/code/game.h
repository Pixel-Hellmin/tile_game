#if !defined(GAME_H)

#include "platform.h"
#include "shared.h"
#include "random.h"
#include <cstdio>
#include <stdarg.h> // move to shared?
#include "asset.h"

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

enum Game_State_Debug_Flags
{
    game_state_flag_prints = (1 << 1),
    game_state_flag_free_cam_mode = (1 << 2),
};

struct Camera
{
    V3 pos;
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

	Tile dude;

	// NOTE(Fermin): We only push Rects to these buffers
	Render_Buffer tiles_buffer;
	Render_Buffer ui_buffer;

	u32 test_sample_index;
	Loaded_Sound test_sound;
};

inline b32
is_tile_index_valid(f32 x, f32 y, i32 cols, i32 rows)
{
    b32 result = (x >= 0 && x < cols && y >= 0 && y < rows);

    return result;
}

inline b32
get_tile(Buffer *buffer, i32 cols, i32 rows, i32 x, i32 y, Tile **out)
{
    // TODO(Fermin): Create a NULL_ENTITY global and point to that so we dont
    // have null pointers and can instead check on that pointer instead of 
    // returning a success/fail result?
    b32 result = 0;

    if(is_tile_index_valid(x, y, cols, rows))
    {
        Tile *tiles = (Tile *)buffer->data;
        *out = tiles + x + y * cols;
        result = 1;
    }

    return result;
}

inline b32
is_set(Game_State *game_state, u32 flag)
{
    b32 result = game_state->debug_flags & flag;
    
    return result;
}

inline void
set_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags |= flag;
}

inline void
unset_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags &= ~flag;
}

inline void
toggle_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags ^= flag;
}

V3
tile_index_to_world_coord(i32 x, i32 y, f32 tile_size_in_meters)
{
    // NOTE(Fermin): For now we assume the world coord is in z = 0 since that's the only plane there's world at
    V3 result = {};
    result.x = (f32)x * tile_size_in_meters;
    result.y = (f32)y * tile_size_in_meters;
    result.z = 0.0;
    
    return result;
}

void
world_coord_to_tile_index(V3 *world_pos, f32 tile_size_in_meters, i32 *index_x, i32 *index_y)
{
    // NOTE(Fermin): For now we assume the world coord is in z = 0 since that's the only plane there's world at
    assert(tile_size_in_meters != 0.0);

    f32 world_x = world_pos->x;
    f32 world_y = world_pos->y;

    *index_x = (i32)(world_x / tile_size_in_meters);
    *index_y = (i32)floor(world_y / tile_size_in_meters); //TODO(Fermin): math.cpp/instrinsics
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
push_tile(Render_Buffer *render_buffer, Tile *tile, V4 color = {1.0, 1.0, 1.0, 1.0})
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

// NOTE: where should these variables go?
global_variable f32 debug_print_line = 0.0f;
global_variable Font *debug_print_font = {};
static void
print_debug_text(char *string, Render_Buffer *ui_buffer, V4 color = {0.0f, 1.0f, 0.0f, 1.0f}) // move to shared?
{
	assert(debug_print_font)

    f32 print_font_size = 24.0f;
    debug_print_line -= print_font_size;
	Font *font = debug_print_font;

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

static void
print_debug_text(Render_Buffer *ui_buffer, char *format, ...)
{
	char text_buffer[256];
	assert(sizeof(format) <= sizeof(text_buffer))

    va_list args;
    va_start(args, format);
    _vsnprintf_s(text_buffer, sizeof(text_buffer), _TRUNCATE, format, args);
	print_debug_text(text_buffer, ui_buffer);
    va_end(args);
}

#define GAME_H
#endif
