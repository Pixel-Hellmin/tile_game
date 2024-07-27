#include <cstdio>

#include "game.h"

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    rects->count = 0;
    Rect *tiles = (Rect *)rects->buffer.data + rects->count;
    Rect *debug_draws = (Rect *)debug->buffer.data + debug->count;

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
        if(input_state.left_mouse && !last_frame_input_state.left_mouse)
        {
            // NOTE(Fermin): Raycasting
            // TODO(Fermin): Intrinsics
            V3 origin = game_state->camera.pos;

            V4 cursor_pos_ndc = V4{input_state.cursor.x,
                                   input_state.cursor.y,
                                   -1.0,
                                   1.0};

            // NOTE(Fermin): Speed. Calculate the inverse when proj changes?
            V4 ray_eye = invert(game_state->proj) * cursor_pos_ndc;
            ray_eye.z = -1.0;
            ray_eye.w = 0.0;

            V3 ray_world = (invert(game_state->view) * ray_eye).xyz;
            V3 ray_direction = normalize(ray_world);

            // NOTE(Fermin): Ray equation
            // NOTE(Fermin): We know the plane we are trying to draw to is on z = 0.0
            // this may change, be aware
            f32 plane_z = 0.0;
            f32 distance_to_end = (plane_z - origin.z) / ray_direction.z;
            V3 ray_at_z_0 = V3 {origin.x + distance_to_end * ray_direction.x,
                                origin.y + distance_to_end * ray_direction.y,
                                0.0};

            /*
            printf("ray origin %.2f, %.2f, %.2f\n", origin.x, origin.y, origin.z);
            printf("ray direction %.2f, %.2f, %.2f\n", ray_direction.x, ray_direction.y, ray_direction.z);
            printf("ray at plane %.2f, %.2f, %.2f\n", ray_at_z_0.x, ray_at_z_0.y, ray_at_z_0.z);
            */

            Rect *rect = debug_draws;
            rect->min_p = ray_at_z_0 * 40.0 - V3{1.0, 1.0, 0.0};
            rect->min_p.z = 0.0f;
            rect->max_p = ray_at_z_0 * 40.0 + V3{1.0, 1.0, 0.0};
            rect->max_p.z = 0.0f;
            rect->color = V4{1.0, 0.0, 0.0, 1.0};
            debug->count++;
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
