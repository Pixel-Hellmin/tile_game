#include <cstdio>

#include "game.h"

void set_texture_to_tile_range(i32 x_start, i32 x_end, i32 y_start, i32 y_end, i32 texture_id, i32 cols, i32 rows, Render_Buffer *render_buffer)
{
    Rect *rect;
    for(u32 x = x_start; x <= x_end; x++)
    {
        for(u32 y = y_start; y <= y_end; y++)
        {
            if(get_tile(&render_buffer->buffer, cols, rows, x, y, &rect))
            {
                rect->texture_id = texture_id;
            }
        }
    }
}

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(tiles_buffer->count == tiles_buffer->cached);

    // TODO(Fermin): Where should this be?
    // Also we should have separate z-buffer values
    // Also we should set a z level for the actual level and dude that is > than 0 so the camera can zoom out
    f32 dude_z = 2.0f;
    f32 map_z = 2.0f;
    f32 camera_z = 2.0f;

    const size_t map_rows = 18;
    const size_t map_cols = 17;
    if(!game_state->initialized)
    {
        assert(tiles_buffer->count == 0);

        u32 tile_map[map_rows][map_cols] = 
        {
                {2, 2, 2, 2,  2, 2, 2, 2,  2, 2, 2, 2,  2, 2, 2, 2, 2}, 
                {2, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 2}, 
                {2, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2}, 
                {0, 0, 0, 0,  0, 0, 0, 0,  0, 2, 2, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 0, 0,  0, 2, 0, 0,  0, 1, 2, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 0, 2,  2, 2, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 0, 1,  1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2},

                {2, 2, 2, 2,  2, 2, 2, 2,  0, 2, 2, 2,  2, 2, 2, 2, 2}, 
                {2, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 2}, 
                {2, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 0, 0,  2, 0, 0, 0,  0, 0, 0, 2,  2, 0, 0, 1, 2}, 
                {2, 1, 0, 2,  2, 0, 0, 0,  0, 0, 0, 2,  2, 0, 0, 0, 0}, 
                {2, 1, 0, 1,  1, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 1, 2}, 
                {2, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2}, 
                {2, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 2}, 
                {2, 2, 2, 2,  2, 2, 2, 2,  2, 2, 2, 2,  2, 2, 2, 2, 2}
        };

        u32 room_width = 8;
        for(i32 row = 0; row < game_state->level_rows; row++)
        {
            for(i32 col = 0; col < game_state->level_cols; col++)
            {
                // NOTE(Fermin): Currently this only draws a grid, which will be turned into a maze later
                Rect rect = {};
                rect.world_index = V3{(f32)col, (f32)row, map_z}; // TODO(Fermin): Maybe Vi3???
                rect.dim_in_tiles = V2{1.0f, 1.0f};

                // NOTE(Fermin): From left to right and down to up. (0, 0) == bottom left 
                // TODO(Fermin): Corner textures
                // TODO(Fermin): b32 for checks?
                if(col % room_width == 0 || row % room_width == 0 || col == game_state->level_cols - 1 || row == game_state->level_rows - 1) // To build a grid before a maze
                {
                    rect.texture_id = game_state->roof_texture_id;

                    if(col == 0) // First col
                    {
                        rect.rotation = Pi32/2.0f;
                    }
                    else if(col == game_state->level_cols - 1) // Last col
                    {
                        rect.rotation = -Pi32/2.0f;
                    }
                    else if(row == 0) // First row
                    {
                        rect.rotation = Pi32;
                    }
                }
                else if(row == 1) // Second row
                {
                    rect.texture_id = game_state->wall_texture_id;
                    rect.rotation = Pi32;
                }
                else if(row == game_state->level_rows - 2) // Second to last row
                {
                    rect.texture_id = game_state->wall_texture_id;
                }
                else if(col == 1) // Second col
                {
                    rect.texture_id = game_state->wall_texture_id;
                    rect.rotation = Pi32/2.0f;
                }
                else if(col == game_state->level_cols - 2) // Second to last col
                {
                    rect.texture_id = game_state->wall_texture_id;
                    rect.rotation = -Pi32/2.0f;
                }
                else
                {
                    rect.texture_id = game_state->floor_texture_id;
                }

                /* This draws the hardcoded tile map
                // NOTE(Fermin): Tile rotation needs to be done in this order to override previous rotations
                if(tile_map[row][col] == 0)
                {
                    rect.texture_id = game_state->floor_texture_id;
                }
                if(tile_map[row][col] == 1)
                {
                    rect.texture_id = game_state->wall_texture_id;

                    if(row < (game_state->level_rows - 1) && tile_map[row + 1][col] == 2) // Check bottom tile
                    {
                        rect.rotation = Pi32;
                    }
                    if(col < (game_state->level_cols - 1) && tile_map[row][col + 1] == 2) // Check tile to the right
                    {
                        rect.rotation = -Pi32/2.0f;
                    }
                    if(col > 0 && tile_map[row][col - 1] == 2) // Check tile to the left
                    {
                        rect.rotation = Pi32/2.0f;
                    }
                    if(row > 0 && tile_map[row - 1][col] == 2) // Check upper tile
                    {
                        rect.rotation = 0.0;
                    }
                }
                if(tile_map[row][col] == 2)
                {
                    rect.texture_id = game_state->roof_texture_id;

                    if(col < (game_state->level_cols - 1) && tile_map[row][col + 1] == 1) // Check tile to the right
                    {
                        rect.rotation = Pi32/2.0f;
                    }
                    if(col > 0 && tile_map[row][col - 1] == 1) // Check tile to the left
                    {
                        rect.rotation = -Pi32/2.0f;
                    }
                    if(row > 0 && tile_map[row - 1][col] == 1) // Check upper tile
                    {
                        rect.rotation = Pi32;
                    }
                    if(row < (game_state->level_rows - 1) && tile_map[row + 1][col] == 1) // Check bottom tile
                    {
                        rect.rotation = 0.0f;
                    }
                }
                */

                push_rectangle(tiles_buffer, &rect);
            }
        }

        // NOTE(Fermin): DEBUG test maze gen algorithm (aldous-broder)
        // Organize data in a way its coherent with the game state
        // NOTE(Fermin): Are these rows/cols or ROOMS????
        const size_t rooms_in_x = 8;
        const size_t rooms_in_y = 8;

        assert(rooms_in_x == game_state->level_rows / room_width);
        assert(rooms_in_y == game_state->level_cols / room_width);

        // NOTE(Fermin): Room (0, 0) is bottom left
        u32 rooms[rooms_in_x][rooms_in_y] = {};
        u32 visited = 0;
        u32 current_x = rooms_in_x / 2;
        u32 current_y = rooms_in_y / 2;
        i32 direction;
        i32 x_start;
        i32 x_end;
        i32 y_start;
        i32 y_end;
        while (visited < rooms_in_x * rooms_in_y)
        {
            direction = random_between(&game_state->entropy, 0, 3); 
            switch(direction)
            {
                case(0): // Move bottom of current room
                {
                    if(current_y > 0)
                    {
                        current_y--;

                        // Remove top wall
                        x_start = current_x * room_width + 1;
                        x_end = x_start + room_width - 2;
                        y_start = current_y * room_width + room_width;
                        y_end = y_start;
                    }
                } break;
                case(1): // Move right of current room
                {
                    if(current_x < (rooms_in_x - 1))
                    {
                        current_x++;

                        // Remove left wall
                        x_start = current_x * room_width;
                        x_end = x_start;
                        y_start = current_y * room_width + 1;
                        y_end = y_start + room_width - 2;
                    }
                } break;
                case(2): // Move top of current room
                {
                    if(current_y < (rooms_in_y - 1))
                    {
                        current_y++;

                        // Remove bottom wall
                        x_start = current_x * room_width + 1;
                        x_end = x_start + room_width - 2;
                        y_start = current_y * room_width;
                        y_end = y_start;
                    }
                } break;
                case(3):  // Move left of current room
                {
                    if(current_x > 0)
                    {
                        current_x--;

                        // Remove right wall
                        x_start = current_x * room_width + room_width;
                        x_end = x_start;
                        y_start = current_y * room_width + 1;
                        y_end = y_start + room_width - 2;
                    }
                } break;
            }

            u32 current_room = rooms[current_y][current_x];
            if(current_room == 0) // Not visited before
            {
                // NOTE(Fermin): Place walls in gaps left by the removed roofs
                if(x_start == 1) // Left
                {
                    Rect *rect;
                    if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_start, y_start, &rect))
                    {
                        rect->texture_id = game_state->wall_texture_id;
                        rect->rotation = Pi32/2.0f;
                    }
                    x_start++;
                }
                if(x_end == game_state->level_cols - 1) // Right
                {
                    x_end--;

                    Rect *rect;
                    if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_end, y_end, &rect))
                    {
                        rect->texture_id = game_state->wall_texture_id;
                        rect->rotation = -Pi32/2.0f;
                    }

                    x_end--;
                }
                if(y_start == 1) // Bottom
                {
                    Rect *rect;
                    if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_start, y_start, &rect))
                    {
                        rect->texture_id = game_state->wall_texture_id;
                        rect->rotation = Pi32;
                    }
                    y_start++;
                }
                if(y_end == game_state->level_rows - 1) // Top
                {
                    y_end--;

                    Rect *rect;
                    if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_start, y_end, &rect))
                    {
                        rect->texture_id = game_state->wall_texture_id;
                    }

                    y_end--;
                }

                // NOTE(Fermin): If this room hasn't been visited before, remove wall
                set_texture_to_tile_range(x_start, x_end, y_start, y_end, game_state->floor_texture_id,
                                          game_state->level_cols, game_state->level_rows, tiles_buffer);

                rooms[current_y][current_x] = 1;
                visited++;
            }
        }

        // NOTE(Fermin): Set walls under roofs after we define every roof. 
        // Can we do this without looping over again?
        for(i32 row = 2; row <= game_state->level_rows - 2; row++)
        {
            for(i32 col = 2; col <= game_state->level_cols - 2; col++)
            {
                Rect *test_roof_rect = {};
                if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, col, row, &test_roof_rect))
                {
                    if(test_roof_rect->texture_id == game_state->roof_texture_id)
                    {
                        Rect *wall_rect = {};
                        if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, col, row-1, &wall_rect))
                        {
                            if(wall_rect->texture_id == game_state->floor_texture_id)
                            {
                                wall_rect->texture_id = game_state->wall_texture_id;
                            }
                        }
                    }
                }
            }
        }

        tiles_buffer->cached = tiles_buffer->count;
        game_state->initialized = 1;
    }
    
    // NOTE(Fermin): Update dude
    Input_Keys input_state = game_state->input_state;
    Input_Keys last_frame_input_state = game_state->last_frame_input_state;
    f32 dt_in_seconds = game_state->dt_in_seconds;
    f32 dude_speed = 10.0 * dt_in_seconds;
    f32 camera_speed = 30.0 * dt_in_seconds;
    if(input_state.f1 && !last_frame_input_state.f1)
    {
        toggle_flag(game_state, game_state_flag_prints);
    }
    if(input_state.f3 && !last_frame_input_state.f3)
    {
        toggle_flag(game_state, game_state_flag_free_cam_mode);
    }

    V3 d_pos = {};
    if(input_state.w)
    {
        d_pos.y = 1.0f; 
    }
    if(input_state.s)
    {
        d_pos.y = -1.0f; 
    }
    if(input_state.a)
    {
        d_pos.x = -1.0f; 
    }
    if(input_state.d)
    {
        d_pos.x = 1.0f; 
    }

    if(!is_set(game_state, game_state_flag_free_cam_mode))
    {
        d_pos *= dude_speed;
        dude->world_index += d_pos; 
        game_state->camera.pos = dude->world_index;
    }
    else
    {
        d_pos *= camera_speed;
        game_state->camera.pos += d_pos; 
    }
    dude->world_index.z = dude_z;
    game_state->camera.pos.z = camera_z;

    // NOTE(Fermin): This needs to happen after we update dude and camera position
    if(input_state.left_mouse && !last_frame_input_state.left_mouse)
    {
        // TODO(Fermin): Intrinsics.
        // Store ortho somewhere and only update when screen size changes.
        M4 ortho = orthogonal(game_state->window_width, game_state->window_height);
        V4 cursor_pos_ndc = V4{input_state.cursor.x, input_state.cursor.y, 0.0, 1.0};
        V4 cursor_pos_in_px = invert(&ortho) * cursor_pos_ndc;
        f32 map_tile_size_in_px_with_perspective = safe_ratio_n(game_state->tile_size_in_px, (map_z - game_state->camera.pos.z + 1.0f), game_state->tile_size_in_px);
        V2 cursor_pos_in_tiles = (cursor_pos_in_px).xy / map_tile_size_in_px_with_perspective;
        V2 cursor_pos_in_world_index = cursor_pos_in_tiles + game_state->camera.pos.xy;

        if(is_tile_index_valid(cursor_pos_in_world_index.x, cursor_pos_in_world_index.y, game_state->level_cols, game_state->level_rows))
        {
            // TODO(Fermin): This logic is broken, highligh is drawn on top of dude alwasys
            game_state->editing_tile = 1;
            game_state->editing_tile_x = (i32)cursor_pos_in_world_index.x;
            game_state->editing_tile_y = (i32)cursor_pos_in_world_index.y;
        }
        else
        {
            game_state->editing_tile = 0;
        }
    }

    // NOTE(Fermin): Experimental particle system logic START
    for(u32 particle_spawn_index = 0;
        particle_spawn_index < 1;
        ++particle_spawn_index)
    {
        Particle *particle = game_state->particles + game_state->next_particle++;

        if(game_state->next_particle >= array_count(game_state->particles))
        {
            game_state->next_particle = 0;
        }

        //particle->p = dude->min_p + V3{random_unilateral(&game_state->entropy), 0.0f, 0.0f};
        particle->p = V3{5.0f, 5.0f, map_z};
        particle->d_p = {
            random_between(&game_state->entropy, -1.0f, 1.0f),
            random_between(&game_state->entropy, 0.5f, 3.0f),
            0.0f
        };
        particle->color_trans = {1.0f, 1.0f, 1.0f, 1.0f};
        //particle->d_color = {random_between(&game_state->entropy, -2.0f, -1.0f), 0.5f, -1.0f, -0.7f};
        particle->d_color = {0.3f, 0.5f, -1.0f, -1.0f};
        particle->d_rotation = random_between(&game_state->entropy, -5.0f, 5.0f);
        particle->rotation = random_between(&game_state->entropy, 0.0f, Pi32*2);
    }

    for(u32 particle_index = 0;
        particle_index < array_count(game_state->particles);
        ++particle_index)
    {
        Particle *particle = game_state->particles + particle_index;

        // NOTE(Fermin): Simulate particles forward in time
        particle->p += game_state->dt_in_seconds * particle->d_p;
        particle->color_trans += game_state->dt_in_seconds * particle->d_color;

        V4 color_trans;
        color_trans.r = clamp01(particle->color_trans.r);
        color_trans.g = clamp01(particle->color_trans.g);
        color_trans.b = clamp01(particle->color_trans.b);
        color_trans.a = clamp01(particle->color_trans.a);

        // NOTE(Fermin): This is to avoid particles spawning with full alpha.
        // Instead the they are spawned with 0.0f alpha and builds up to ~1.0f
        // before decreasing again
        if(color_trans.a > 0.9f)
        {
            // NOTE(Fermin): Passing a higher min and lower max is the equivalent of
            // passing a low min and high max and then (1.0f - result)
            color_trans.a = 0.9f * clamp01_map_to_range(1.0f, color_trans.a, 0.9f);
        }

        particle->rotation += game_state->dt_in_seconds * particle->d_rotation;

        Rect part = {};
        part.world_index = particle->p;
        part.dim_in_tiles = V2{0.4, 0.4};
        part.rotation = particle->rotation;
        part.texture_id = game_state->wall_texture_id;

        push_rectangle(tiles_buffer, &part, color_trans);
    }
    // NOTE(Fermin): Experimental particle system logic END
    

    if(game_state->editing_tile)
    {
        // NOTE(Fermin): We can avoid doing this if we use the z coord
        Rect highlight = {};
        highlight.world_index = V3{(f32)game_state->editing_tile_x, (f32)game_state->editing_tile_y, map_z};
        highlight.dim_in_tiles = V2{1.0f, 1.0f};
        highlight.texture_id = game_state->highlight_texture_id;

        push_rectangle(tiles_buffer, &highlight, V4{0.0f, 1.0f, 0.0f, 1.0f});
    }

    dude->rotation += dt_in_seconds; // nocheckin
    push_rectangle(tiles_buffer, dude);

    game_state->last_frame_input_state = input_state;
}
