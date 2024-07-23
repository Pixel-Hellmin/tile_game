#include <cstdio>

#include "game.h"

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    rects->count = 0;

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
    Rect *tiles = (Rect *)rects->buffer.data + rects->count;
    for(i32 row = 0; row < map_rows; row++)
    {
        for(i32 col = 0; col < map_cols; col++)
        {
            f32 start_x = col*tile_width;
            f32 start_y = row*(-tile_height);

            Rect *current_tile = tiles + rects->count++;
            current_tile->min_p = V3{start_x, start_y, 0.0};
            current_tile->max_p = V3{start_x + tile_width, start_y + tile_height, 0.0};
            current_tile->color = V4{(f32)tile_map[row][col], 0.0, 1.0, 1.0};
        }
    }
    
    // NOTE(Fermin): Update dude
    // TODO(Fermin): Map each key to an action and make it remapable
    // TODO(Fermin): Use newtons laws for motion?
    Input_Keys input_state = game_state->input_state;
    Input_Keys last_frame_input_state = game_state->last_frame_input_state;
    f32 delta = game_state->delta;
    f32 dude_speed = 100.0 * delta;
    f32 camera_speed = 30.0 * delta;
    if(input_state.f1 && !last_frame_input_state.f1)
    {
        if(is_set(game_state, game_state_flag_prints))
        {
            unset_flag(game_state, game_state_flag_prints);
        }
        else
        {
            set_flag(game_state, game_state_flag_prints);
        }
    }
    if(input_state.f2 && !last_frame_input_state.f2)
    {
        if(is_set(game_state, game_state_flag_prints))
        {
            unset_flag(game_state, game_state_flag_wireframe_mode);
        }
        else
        {
            set_flag(game_state, game_state_flag_wireframe_mode);
        }
    }
    if(input_state.f3 && !last_frame_input_state.f3)
    {
        if(is_set(game_state, game_state_flag_free_cam_mode))
        {
            unset_flag(game_state, game_state_flag_free_cam_mode);
        }
        else
        {
            set_flag(game_state, game_state_flag_free_cam_mode);
        }
    }
    if(!is_set(game_state, game_state_flag_free_cam_mode))
    {
        if(input_state.w)
        {
            dude->min_p.y += dude_speed; 
            dude->max_p.y += dude_speed; 
        }
        if(input_state.s)
        {
            dude->min_p.y -= dude_speed; 
            dude->max_p.y -= dude_speed; 
        }
        if(input_state.a)
        {
            dude->min_p.x -= dude_speed; 
            dude->max_p.x -= dude_speed; 
        }
        if(input_state.d)
        {
            dude->min_p.x += dude_speed; 
            dude->max_p.x += dude_speed; 
        }
    }
    else
    {
        if(input_state.w)
        {
            game_state->camera.pos += camera_speed * game_state->camera.front;
        }
        if(input_state.s)
        {
            game_state->camera.pos -= camera_speed * game_state->camera.front;
        }
        if(input_state.a)
        {
            game_state->camera.pos -= normalize(cross(game_state->camera.front, game_state->camera.up)) * camera_speed;
        }
        if(input_state.d)
        {
            game_state->camera.pos += normalize(cross(game_state->camera.front, game_state->camera.up)) * camera_speed;
        }
    }
    

    game_state->last_frame_input_state = input_state;
}
