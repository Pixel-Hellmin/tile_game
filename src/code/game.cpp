#include <cstdio>

#include "game.h"

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(tiles_buffer->count == tiles_buffer->cached);

    const size_t map_rows = 18;
    const size_t map_cols = 17;
    f32 tile_size_in_meters = game_state->tile_size_in_meters;
    if(!game_state->initialized)
    {
        assert(tiles_buffer->count == 0);

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

        game_state->level_rows = map_rows;
        game_state->level_cols = map_cols;
        for(i32 row = 0; row < game_state->level_rows; row++)
        {
            for(i32 col = 0; col < game_state->level_cols; col++)
            {
                f32 start_x = col*tile_size_in_meters;
                f32 start_y = (game_state->level_rows - 1 - row)*tile_size_in_meters;

                Rect rect = {};
                rect.min_p = V3{start_x, start_y, 0.0};
                rect.max_p = V3{start_x + tile_size_in_meters, start_y + tile_size_in_meters, 0.0};
                rect.color = V4{(f32)tile_map[row][col], 0.0, 1.0, 1.0};
                push_rectangle(tiles_buffer, &rect);
            }
        }

        tiles_buffer->cached = tiles_buffer->count;
        game_state->initialized = 1;
    }
    
    // NOTE(Fermin): Update dude
    Input_Keys input_state = game_state->input_state;
    Input_Keys last_frame_input_state = game_state->last_frame_input_state;
    f32 delta = game_state->delta;
    f32 dude_speed = 2.0 * delta;
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

            // NOTE(Fermin): This coords are in 'model' space.
            V3 ray_at_z_0 = V3 {origin.x + distance_to_end * ray_direction.x,
                                origin.y + distance_to_end * ray_direction.y,
                                0.0};
            // NOTE(Fermin): We bring this to world coords
            ray_at_z_0 *= tile_size_in_meters;

            i32 tile_x, tile_y;
            world_coord_to_tile_index(&ray_at_z_0, tile_size_in_meters, &tile_x, &tile_y);
            if(is_tile_index_valid(tile_x, tile_y, game_state->level_cols, game_state->level_rows))
            {
                game_state->editing_tile = 1;
                game_state->editing_tile_x = tile_x;
                game_state->editing_tile_y = tile_y;
            }
            else
            {
                game_state->editing_tile = 0;
            }
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
    

    if(game_state->editing_tile)
    {
        V3 tile_world_pos = tile_index_to_world_coord(game_state->editing_tile_x,
                                                      game_state->editing_tile_y,
                                                      tile_size_in_meters);

        Rect highlight = {};
        highlight.min_p = tile_world_pos;
        highlight.max_p = tile_world_pos + V3{tile_size_in_meters, tile_size_in_meters, 0.0};
        highlight.color = V4{0.3, 0.3, 0.3, 0.3};

        push_rectangle(tiles_buffer, &highlight);
    }
    push_rectangle(tiles_buffer, dude);

    game_state->last_frame_input_state = input_state;
}
