#include <cstdio>

#include "windows_main.h"
#include "game.h"

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{

    // NOTE(Fermi): Draw tile map
    const size_t map_rows = 18;
    const size_t map_cols = 17;
    u32 tile_map[map_rows][map_cols] = 
    {
            {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 1,  1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},

            {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0, 1}, 
            {1, 0, 1, 1,  1, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0, 0}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1}, 
            {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1}
    };

    i32 tile_width = 40;
    i32 tile_height = 40;
    u32 count = 0;
    Rect *tiles = (Rect *)render_rect->data;
    for(i32 row = 0; row < map_rows; row++)
    {
        for(i32 col = 0; col < map_cols; col++)
        {
            f32 start_x = col*tile_width;
            f32 start_y = row*(-tile_height);

            Rect *current_tile = tiles + count++;
            current_tile->min_p = V2{start_x, start_y};
            current_tile->max_p = V2{start_x + tile_width, start_y + tile_height};
            current_tile->color = V4{(f32)tile_map[row][col], 0.0, 1.0, 1.0};
        }
    }
    *tile_count = count;
    
    // NOTE(Fermi): Update dude
    // TODO(Fermi): Delta time
    if (input_state->up)
    {
        dude->min_p.y += 2.5; 
        dude->max_p.y += 2.5; 
    }
    if (input_state->down)
    {
        dude->min_p.y -= 2.5; 
        dude->max_p.y -= 2.5; 
    }
    if (input_state->left)
    {
        dude->min_p.x -= 2.5; 
        dude->max_p.x -= 2.5; 
    }
    if (input_state->right)
    {
        dude->min_p.x += 2.5; 
        dude->max_p.x += 2.5; 
    }
}
