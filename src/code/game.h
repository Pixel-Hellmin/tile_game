#if !defined(GAME_H)

#include "platform.h"
#include "shared.h"
#include "random.h"
#include <cstdio>
#include <stdarg.h> // move to shared?
#include "asset.h"
#include "audio.h"
#include "wad.h"

enum Game_State_Debug_Flags
{
    game_state_flag_prints = (1 << 1),
    game_state_flag_free_cam_mode = (1 << 2),
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

struct Loaded_Level
{
	V2 *vertex_positions;
	u32 vertex_count;

	Sector *sectors;
	u32 sector_count;

	Side_Def *sides;
	u32 side_count;

	Line_Def *lines;
	u32 line_count;
};

struct Game_State
{
    Random_Series entropy;

    u32 debug_flags;

    b32 initialized;

    // NOTE(Fermin): Testing particle system stuff. Move this to where it makes sense
    u32 next_particle;
    Particle particles[64];

	Memory_Arena tmp_arena;
	Memory_Arena level_arena;
	Memory_Arena persistent_arena; // weird name

	Loaded_Sound test_sound;
	Playing_Sound *test_music;
	Game_Audio_State audio_state;

	Wad_File wad;
	Loaded_Level loaded_level;
	Texture_Cache texture_cache;
};

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

/*
 * Left this here for now. I'll delete/refactor when I get to debug
 * tools for the game
*/
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
global f32 debug_print_line = 0.0f;
global Font *debug_print_font = {};
static void
print_debug_text(char *string, Memory_Arena *arena, V4 color = {0.0f, 1.0f, 0.0f, 1.0f}) // move to shared?
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


            Tile *glyph = push_struct(arena, Tile);
            glyph->pos_in_screen.xy = V2{x, y};
            glyph->dim_in_px = V2{w, h};
            glyph->texture_id = font->glyph_texture_ids[*c - font_first_character];
            glyph->color = color;
            glyph->rotation = 0.0f;

            x += advance;
        }
        else
        {
            x += space_width;
        }
    }
}

#define GAME_H
#endif
