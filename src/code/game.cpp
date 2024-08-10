#include <cstdio>

#include "game.h"

void world_coord_to_tile_index(V3 *world_pos, f32 tile_size_in_meters, i32 *index_x, i32 *index_y)
{
    // NOTE(Fermin): For now we assume the world coord is in z = 0 since that's the only plane there's world at
    assert(tile_size_in_meters != 0.0);

    f32 world_x = world_pos->x;
    f32 world_y = world_pos->y;

    *index_x = (i32)(world_x / tile_size_in_meters);
    *index_y = (i32)floor(world_y / tile_size_in_meters); //TODO(Fermin): math.cpp/instrinsics
}

b32 get_tile(Rect *tiles, i32 cols, i32 rows, i32 x, i32 y, Rect **out)
{
    b32 result = 0;

    if(x >= 0 && x < cols &&
       y >= 0 && y < rows)
    {
        // NOTE(Fermin): Since the order the tiles are stored in memory and 
        // the order of the tile indexes in world are opposite from each other
        // we need to access tiles from bottom up
        *out = tiles + (x + (rows - 1)*cols - (y * cols));

        result = 1;
    }

    return result;
}

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    Rect *tiles = (Rect *)rects->buffer.data;
    Rect *debug_draws = (Rect *)debug->buffer.data + debug->count;

    const size_t map_rows = 18;
    const size_t map_cols = 17;
    f32 tile_size_in_meters = game_state->tile_size_in_meters;
    if(!game_state->initialized)
    {
        rects->count = 0;
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

        // TODO(Fermin): Set this value in game context since it's needed in rendering too
        for(i32 row = 0; row < map_rows; row++)
        {
            for(i32 col = 0; col < map_cols; col++)
            {
                f32 start_x = col*tile_size_in_meters;
                f32 start_y = (map_rows - 1 - row)*tile_size_in_meters;

                Rect *current_tile = tiles + rects->count++;
                current_tile->min_p = V3{start_x, start_y, 0.0};
                current_tile->max_p = V3{start_x + tile_size_in_meters, start_y + tile_size_in_meters, 0.0};
                current_tile->color = V4{(f32)tile_map[row][col], 0.0, 1.0, 1.0};
            }
        }

        game_state->initialized = 1;
    }
    
    // NOTE(Fermin): Update dude
    // TODO(Fermin): Map each key to an action and make it remapable
    // TODO(Fermin): Use newtons laws for motion?
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

            // NOTE(Fermin): I dont like having to check if we get a tile, but since this is only for the map editor maybe its ok?
            Rect *hit;
            if(get_tile(tiles, map_cols, map_rows, tile_x, tile_y, &hit))
            {
                hit->color = V4{0.0, 1.0, 1.0, 1.0};
            }


            /* DEBUG prints and draws
            printf("ray origin %.2f, %.2f, %.2f\n", origin.x, origin.y, origin.z);
            printf("ray direction %.2f, %.2f, %.2f\n", ray_direction.x, ray_direction.y, ray_direction.z);
            printf("ray at plane %.2f, %.2f, %.2f\n", ray_at_z_0.x, ray_at_z_0.y, ray_at_z_0.z);

            Rect *rect = debug_draws;
            rect->min_p = ray_at_z_0 - V3{0.05, 0.05, 0.0};
            rect->min_p.z = 0.0f;
            rect->max_p = ray_at_z_0 + V3{0.05, 0.05, 0.0};
            rect->max_p.z = 0.0f;
            rect->color = V4{1.0, 0.0, 0.0, 1.0};
            debug->count++;
            */
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
