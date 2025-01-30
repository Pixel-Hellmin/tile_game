#if !defined(GAME_H)

#include "windows_main.h"

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

inline b32 is_tile_index_valid(i32 x, i32 y, i32 cols, i32 rows)
{
    b32 result = (x >= 0 && x < cols && y >= 0 && y < rows);

    return result;
}

b32 get_tile(Buffer *buffer, i32 cols, i32 rows, i32 x, i32 y, Rect **out)
{
    // TODO(Fermin): Create a NULL_ENTITY global and point to that so we dont
    // have null pointers and can instead check on that pointer instead of 
    // returning a success/fail result.
    b32 result = 0;

    if(is_tile_index_valid(x, y, cols, rows))
    {
        /* ????????????????????
        // NOTE(Fermin): Since the order the tiles are stored in memory and 
        // the order of the tile indexes in world are opposite from each other
        // we need to access tiles from bottom up
        *out = tiles + (x + (rows - 1)*cols - (y * cols));
        */

        Rect *tiles = (Rect *)buffer->data;
        *out = tiles + x + y * cols;
        result = 1;
    }

    return result;
}

inline b32 is_set(Game_State *game_state, u32 flag)
{
    b32 result = game_state->debug_flags & flag;
    
    return result;
}

inline void set_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags |= flag;
}

inline void unset_flag(Game_State *game_state, u32 flag)
{
    game_state->debug_flags &= ~flag;
}

V3 tile_index_to_world_coord(i32 x, i32 y, f32 tile_size_in_meters)
{
    // NOTE(Fermin): For now we assume the world coord is in z = 0 since that's the only plane there's world at
    V3 result = {};
    result.x = (f32)x * tile_size_in_meters;
    result.y = (f32)y * tile_size_in_meters;
    result.z = 0.0;
    
    return result;
}

void world_coord_to_tile_index(V3 *world_pos, f32 tile_size_in_meters, i32 *index_x, i32 *index_y)
{
    // NOTE(Fermin): For now we assume the world coord is in z = 0 since that's the only plane there's world at
    assert(tile_size_in_meters != 0.0);

    f32 world_x = world_pos->x;
    f32 world_y = world_pos->y;

    *index_x = (i32)(world_x / tile_size_in_meters);
    *index_y = (i32)floor(world_y / tile_size_in_meters); //TODO(Fermin): math.cpp/instrinsics
}

#define GAME_H
#endif
