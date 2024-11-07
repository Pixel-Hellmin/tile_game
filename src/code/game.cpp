#include <cstdio>
#include <random>

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

    const size_t map_rows = 18;
    const size_t map_cols = 17;
    f32 tile_size_in_meters = game_state->tile_size_in_meters;
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
                f32 start_x = col*tile_size_in_meters;
                f32 start_y = row*tile_size_in_meters;

                Rect rect = {};
                rect.min_p = V3{start_x, start_y, 0.0};
                rect.max_p = V3{start_x + tile_size_in_meters, start_y + tile_size_in_meters, 0.0};

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

        // TODO(Fermin): Use OS RNG
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<int> distribution(0,3);

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
            direction = distribution(generator); 
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
    f32 delta = game_state->delta;
    f32 dude_speed = 10.0 * delta;
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

    if(!is_set(game_state, game_state_flag_free_cam_mode))
    {
            game_state->camera.pos.xy = dude->min_p.xy;
            //game_state->camera.pos.z += 15.0f;
    }
    

    if(game_state->editing_tile)
    {
        V3 tile_world_pos = tile_index_to_world_coord(game_state->editing_tile_x,
                                                      game_state->editing_tile_y,
                                                      tile_size_in_meters);

        Rect highlight = {};
        highlight.min_p = tile_world_pos;
        highlight.max_p = tile_world_pos + V3{tile_size_in_meters, tile_size_in_meters, 0.0};
        highlight.texture_id = game_state->highlight_texture_id;

        push_rectangle(tiles_buffer, &highlight);
    }
    push_rectangle(tiles_buffer, dude);

    game_state->last_frame_input_state = input_state;
}
