#include "game.h"

static void
set_texture_to_tile_range(i32 x_start, i32 x_end, i32 y_start, i32 y_end, i32 texture_id, i32 cols, i32 rows, Render_Buffer *render_buffer)
{
    Tile *tile;
    for(u32 x = x_start; x <= x_end; x++)
    {
        for(u32 y = y_start; y <= y_end; y++)
        {
            if(get_tile(&render_buffer->buffer, cols, rows, x, y, &tile))
            {
                tile->texture_id = texture_id;
            }
        }
    }
}

static void
generate_level(Game_State *game_state, Render_Buffer *tiles_buffer, f32 map_z)
{
    u32 room_width = 6; // tiles
    const size_t rooms_in_x = 30;
    const size_t rooms_in_y = 30;

    // NOTE(Fermin): We add 3 because of the additional roof and wall tiles
    game_state->level_cols = room_width * rooms_in_x + 3; 
    game_state->level_rows = room_width * rooms_in_y + 3;

    // NOTE(Fermin): This only draws a grid, which will be turned into a maze later.
    for(i32 row = 0; row < game_state->level_rows; row++)
    {
        for(i32 col = 0; col < game_state->level_cols; col++)
        {
            Tile tile = {};
            tile.world_index = V3{(f32)col, (f32)row, map_z};
            tile.dim_in_tiles = V2{1.0f, 1.0f};

            // NOTE(Fermin): From left to right and down to up. (0, 0) == bottom left 
            if(col == 0 || row == 0 || col == game_state->level_cols - 1 || row == game_state->level_rows - 1) // Roof all around
            {
                tile.texture_id = game_state->roof_texture_id;

                if(col == 0) // First col
                {
                    tile.rotation = Pi32/2.0f;
                }
                else if(col == game_state->level_cols - 1) // Last col
                {
                    tile.rotation = -Pi32/2.0f;
                }
                else if(row == 0) // First row
                {
                    tile.rotation = Pi32;
                }
            }
            else if(row == 1 || row == game_state->level_rows - 2) // Wall top and bottom
            {
                if((col - 1) % room_width != 0)
                {
                    tile.texture_id = game_state->wall_texture_id;
                    if(row == 1)
                    {
                        tile.rotation = Pi32;
                    }
                }
                else
            {
                    tile.texture_id = game_state->roof_texture_id; // Roof for grid
                }
            }
            else if(col == 1 || col == game_state->level_cols - 2) // Wall left and right
            {
                if((row - 1) % room_width != 0)
                {
                    tile.texture_id = game_state->wall_texture_id;
                    if(col == 1)
                    {
                        tile.rotation = Pi32/2.0f;
                    }
                    else
                {
                        tile.rotation = -Pi32/2.0f;
                    }
                }
                else
            {
                    tile.texture_id = game_state->roof_texture_id; // Roof for grid
                }
            }
            else if((col - 1) % room_width == 0 || (row - 1) % room_width == 0)
            {
                tile.texture_id = game_state->roof_texture_id; // Roof for grid
            }
            else
        {
                tile.texture_id = game_state->floor_texture_id;
            }

            // Corners
            if((col == 1 && row == 1) ||
                (col == 1 && row == game_state->level_rows - 2) ||
                (col == game_state->level_cols - 2 && row == 1) ||
                (col == game_state->level_cols - 2 && row == game_state->level_rows - 2))
            { 
                tile.texture_id = game_state->wall_texture_id; 
            }

            push_tile(tiles_buffer, &tile);
        }
    }

    // NOTE(Fermin): DEBUG test maze gen algorithm (aldous-broder)
    // Organize data in a way its coherent with the game state
    assert(rooms_in_x == game_state->level_cols / room_width);
    assert(rooms_in_y == game_state->level_rows / room_width);

    // NOTE(Fermin): Room (0, 0) is bottom left
    u32 rooms[rooms_in_x][rooms_in_y] = {};
    u32 visited = 0;
    u32 current_room_x = rooms_in_x / 2;
    u32 current_room_y = rooms_in_y / 2;
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
                    if(current_room_y > 0)
                    {
                        current_room_y--;

                        // Remove top wall
                        x_start = current_room_x * room_width + 2;
                        x_end = x_start + room_width - 2;
                        y_start = current_room_y * room_width + room_width + 1;
                        y_end = y_start;
                    }
                } break;
            case(1): // Move right of current room
                {
                    if(current_room_x < (rooms_in_x - 1))
                    {
                        current_room_x++;

                        // Remove left wall
                        x_start = current_room_x * room_width + 1;
                        x_end = x_start;
                        y_start = current_room_y * room_width + 2;
                        y_end = y_start + room_width - 2;
                    }
                } break;
            case(2): // Move top of current room
                {
                    if(current_room_y < (rooms_in_y - 1))
                    {
                        current_room_y++;

                        // Remove bottom wall
                        x_start = current_room_x * room_width + 2;
                        x_end = x_start + room_width - 2;
                        y_start = current_room_y * room_width + 1;
                        y_end = y_start;
                    }
                } break;
            case(3):  // Move left of current room
                {
                    if(current_room_x > 0)
                    {
                        current_room_x--;

                        // Remove right wall
                        x_start = current_room_x * room_width + room_width + 1;
                        x_end = x_start;
                        y_start = current_room_y * room_width + 2;
                        y_end = y_start + room_width - 2;
                    }
                } break;
        }

        u32 current_room = rooms[current_room_x][current_room_y];
        if(current_room == 0) // Not visited before
        {
            // NOTE(Fermin): Place walls in gaps left by the removed roofs
            if(x_start == 2) // Left
            {
                Tile *tile;
                if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_start-1, y_start, &tile))
                {
                    tile->texture_id = game_state->wall_texture_id;
                    tile->rotation = Pi32/2.0f;
                }
            }
            if(x_end == game_state->level_cols - 3) // Right
            {
                Tile *tile;
                if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_end+1, y_end, &tile))
                {
                    tile->texture_id = game_state->wall_texture_id;
                    tile->rotation = -Pi32/2.0f;
                }
            }
            if(y_start == 2) // Bottom
            {
                Tile *tile;
                if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_start, y_start-1, &tile))
                {
                    tile->texture_id = game_state->wall_texture_id;
                    tile->rotation = Pi32;
                }
            }
            if(y_end == game_state->level_rows - 3) // Top
            {
                Tile *tile;
                if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, x_start, y_end+1, &tile))
                {
                    tile->texture_id = game_state->wall_texture_id;
                }
            }

            // NOTE(Fermin): If this room hasn't been visited before, remove wall
            set_texture_to_tile_range(x_start, x_end, y_start, y_end, game_state->floor_texture_id,
                                      game_state->level_cols, game_state->level_rows, tiles_buffer);

            rooms[current_room_x][current_room_y] = 1;
            visited++;
        }
    }

    // NOTE(Fermin): Set walls under roofs after we define every roof. 
    // Can we do this without looping over again?
    for(i32 row = 2; row <= game_state->level_rows - 2; row++)
    {
        for(i32 col = 2; col <= game_state->level_cols - 2; col++)
        {
            Tile *test_roof_tile = {};
            if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, col, row, &test_roof_tile))
            {
                if(test_roof_tile->texture_id == game_state->roof_texture_id)
                {
                    Tile *wall_tile = {};
                    if(get_tile(&tiles_buffer->buffer, game_state->level_cols, game_state->level_rows, col, row-1, &wall_tile))
                    {
                        if(wall_tile->texture_id == game_state->floor_texture_id)
                        {
                            wall_tile->texture_id = game_state->wall_texture_id;
                        }
                    }
                }
            }
        }
    }
}

static void
render_tiles(Game_State *game_state, Render_Buffer *tiles_buffer, Render_Buffer *render_buffer)
{
	// NOTE(Fermin): @Speed - We should only render the tiles that are visible on screen

	f32 tile_size_in_px = game_state->tile_size_in_px;

    // NOTE(Fermin): We need to be careful with decimals here, otherwise we'll see gaps between tiles.
    // Should we truncate? round? Not sure
    V2 half_window =
    {
        ((f32)game_state->window_width) / 2.0f,
        ((f32)game_state->window_height) / 2.0f
    };

	Tile *tiles = (Tile *)tiles_buffer->buffer.data;
	for(u32 index = 0; index < tiles_buffer->count; index++)
	{
		Tile *tile = tiles + index;

		// rotate axis. Perp.
		V2 x_axis = V2{_cos(tile->rotation), _sin(tile->rotation)};
		V2 y_axis = V2{-x_axis.y, x_axis.x};

		// scale by half dim because origin is at the center of the tile
		x_axis *= tile->dim_in_tiles.x * 0.5f;
		y_axis *= tile->dim_in_tiles.y * 0.5f;

		V3 origin = tile->world_index - game_state->camera.pos; // move into camera space
		origin.xy = origin.xy + tile->dim_in_tiles * 0.5f; // set origin in center of tile

		// NOTE(Fermin): We dont need all the z
		V3 corners[4];
		corners[0].xy = origin.xy - x_axis - y_axis; // Lower left
		corners[0].z = origin.z;
		corners[1].xy = origin.xy + x_axis - y_axis; // Lower right
		corners[1].z = origin.z;
		corners[2].xy = origin.xy + x_axis + y_axis; // Upper right
		corners[2].z = origin.z;
		corners[3].xy = origin.xy - x_axis + y_axis; // Upper left
		corners[3].z = origin.z;

		// Transform from world index to pixels. Not Z since that is used for z-buffer
		corners[0].xy *= tile_size_in_px;
		corners[1].xy *= tile_size_in_px;
		corners[2].xy *= tile_size_in_px;
		corners[3].xy *= tile_size_in_px;

		// Move the origin of the camera from bottom left to the center of the window
		corners[0].xy += half_window;
		corners[1].xy += half_window;
		corners[2].xy += half_window;
		corners[3].xy += half_window;

		push_quad(render_buffer, corners, tile->texture_id, tile->color);
	}
}

static void
render_ui(Game_State *game_state, Render_Buffer *ui_buffer, Render_Buffer *render_buffer)
{
	Tile *ui_rects = (Tile *)ui_buffer->buffer.data;
	for(u32 index = ui_buffer->cached; index < ui_buffer->count; index++)
	{
		// NOTE(Fermin): IMPORTANT - Its important to note here we skip all of our
		// cached tiles. We currently do not cache any UI tiles and since we reuse
		// the tile buffer for the ui, we need to skip the cached tiles.
		Tile *tile = ui_rects + index;

		// rotate axis. Perp.
		V2 x_axis = V2{_cos(tile->rotation), _sin(tile->rotation)};
		V2 y_axis = V2{-x_axis.y, x_axis.x};

		// scale by half dim because origin is at the center of the tile
		x_axis *= tile->dim_in_px.x * 0.5f;
		y_axis *= tile->dim_in_px.y * 0.5f;

		V3 origin = tile->pos_in_screen;
		origin.xy = origin.xy + tile->dim_in_px * 0.5f; // set origin in center of tile
		// NOTE(Fermin): We dont need all the z
		V3 corners[4];
		corners[0].xy = origin.xy - x_axis - y_axis; // Lower left
		corners[0].z = origin.z;
		corners[1].xy = origin.xy + x_axis - y_axis; // Lower right
		corners[1].z = origin.z;
		corners[2].xy = origin.xy + x_axis + y_axis; // Upper right
		corners[2].z = origin.z;
		corners[3].xy = origin.xy - x_axis + y_axis; // Upper left
		corners[3].z = origin.z;

		push_quad(render_buffer, corners, tile->texture_id, tile->color);
	}
}

static void
reset_non_cached_memory(Render_Buffer *buffer)
{
	buffer->count = buffer->cached;
}

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    // TODO(Fermin): Where should this be?
    // Also we should have separate z-buffer values
    // Also we should set a z level for the actual level and dude that is > than 0 so the camera can zoom out
    f32 dude_z = 2.0f;
    f32 map_z = 2.0f;
    f32 camera_z = 2.0f;

    assert(game_memory->permanent_storage.count >= sizeof(Game_State));
	Game_State *game_state = (Game_State *)game_memory->permanent_storage.data;
	Render_Buffer *tiles_buffer = &game_state->tiles_buffer;
	Render_Buffer *ui_buffer = &game_state->ui_buffer;
	Tile *dude = &game_state->dude;

	f64 last_frame_render_buffer_used = (f64)render_buffer->count;
	f64 last_frame_ui_buffer_used = (f64)ui_buffer->count;
	reset_non_cached_memory(tiles_buffer);
	reset_non_cached_memory(ui_buffer);
	reset_non_cached_memory(render_buffer);

    if(!game_state->initialized)
    {
        assert(tiles_buffer->count == 0);

		game_state->entropy.index = 666;
		game_state->camera.pos   = {10.0f, 10.0f, 10.0f};
		game_state->tile_size_in_px = 64.0f;
		// TODO(Fermin): Handle assets properly. How?
		game_state->floor_texture_id     = game_memory->floor_texture_id;
		game_state->wall_texture_id      = game_memory->wall_texture_id;
		game_state->roof_texture_id      = game_memory->roof_texture_id;
		game_state->highlight_texture_id = game_memory->highlight_texture_id;

		dude->world_index = V3{0.0f, 0.0f, 0.0f};
		dude->dim_in_tiles = V2{1.0f, 1.0f};
		dude->texture_id = game_memory->dude_texture_id;

		tiles_buffer->buffer.data = (u8 *)(game_memory->permanent_storage.data + sizeof(Game_State));
		tiles_buffer->buffer.count = gigabytes(1);

		ui_buffer->buffer.data = (u8 *)(game_memory->permanent_storage.data + sizeof(Game_State) + tiles_buffer->buffer.count);
		ui_buffer->buffer.count = game_memory->permanent_storage.count - sizeof(Game_State) - tiles_buffer->buffer.count;

		// NOTE(Fermin): End of memory partition
		assert((sizeof(Game_State) + tiles_buffer->buffer.count + ui_buffer->buffer.count) == game_memory->permanent_storage.count)

        generate_level(game_state, tiles_buffer, map_z);
		tiles_buffer->cached = tiles_buffer->count; // NOTE(Fermin): Cache the level

		set_flag(game_state, game_state_flag_prints);
        game_state->initialized = 1;
    }

	game_state->window_width = game_memory->window_width;
	game_state->window_height = game_memory->window_height;
    
    // NOTE(Fermin): Update dude
    Input_Keys new_input = input[0];
    Input_Keys old_input = input[1];
    //Input_Keys last_frame_input_state = game_state->last_frame_input_state;
    f32 dt_in_seconds = new_input.dt_in_seconds;
    f32 dude_speed = 10.0 * dt_in_seconds;
    f32 camera_speed = 30.0 * dt_in_seconds;
    if(new_input.f1 && !old_input.f1)
    {
        toggle_flag(game_state, game_state_flag_prints);
    }
    if(new_input.f3 && !old_input.f3)
    {
        toggle_flag(game_state, game_state_flag_free_cam_mode);
    }

    V3 d_pos = {};
    if(new_input.w)
    {
        d_pos.y = 1.0f; 
    }
    if(new_input.s)
    {
        d_pos.y = -1.0f; 
    }
    if(new_input.a)
    {
        d_pos.x = -1.0f; 
    }
    if(new_input.d)
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
    if(new_input.left_mouse && !old_input.left_mouse)
    {
        // TODO(Fermin): Intrinsics.
        // Store ortho somewhere and only update when screen size changes.
        M4 ortho = orthogonal(game_state->window_width, game_state->window_height);
        V4 cursor_pos_ndc = V4{new_input.cursor.x, new_input.cursor.y, 0.0, 1.0};
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
        particle->p += new_input.dt_in_seconds * particle->d_p;
        particle->color_trans += new_input.dt_in_seconds * particle->d_color;

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

        particle->rotation += new_input.dt_in_seconds * particle->d_rotation;

        Tile part = {};
        part.world_index = particle->p;
        part.dim_in_tiles = V2{0.4, 0.4};
        part.rotation = particle->rotation;
        part.texture_id = game_state->wall_texture_id;

        push_tile(tiles_buffer, &part, color_trans);
    }
    // NOTE(Fermin): Experimental particle system logic END
    

    if(game_state->editing_tile)
    {
        // NOTE(Fermin): We can avoid doing this if we use the z coord
        Tile highlight = {};
        highlight.world_index = V3{(f32)game_state->editing_tile_x, (f32)game_state->editing_tile_y, map_z};
        highlight.dim_in_tiles = V2{1.0f, 1.0f};
        highlight.texture_id = game_state->highlight_texture_id;

        push_tile(tiles_buffer, &highlight, V4{0.0f, 1.0f, 0.0f, 1.0f});
    }

    dude->rotation += dt_in_seconds;
    push_tile(tiles_buffer, dude);
	
	// ------------------ DEBUG PRINTS ------------------
	// TODO(Fermin): We just moved this into game from platform,
	// some of this may not make much sense anymore. Take a look
	// and refactor. THIS IS SHIT; FIX ASAP
	debug_print_line = game_state->window_height;
	char text_buffer[256];

	Font *debug_font_consola = &game_memory->debug_font_consola;
	_snprintf_s(text_buffer, sizeof(text_buffer), "[f]   [ms]");
	print_debug_text(text_buffer, debug_font_consola, ui_buffer);
	_snprintf_s(text_buffer, sizeof(text_buffer), "%i  %.3f", round_f32_to_i32(1.0f/new_input.dt_in_seconds), new_input.dt_in_seconds*1000.0f);
	print_debug_text(text_buffer, debug_font_consola, ui_buffer);
	if(is_set(game_state, game_state_flag_prints))
	{
		// --------------- Tiles Buffer ---------------
		_snprintf_s(text_buffer, sizeof(text_buffer), "Tiles buffer:");
		print_debug_text(text_buffer, debug_font_consola, ui_buffer);

		f64 tiles_max_capacity = (f64)tiles_buffer->buffer.count/(f64)sizeof(Tile);
		u64 tiles_bytes = (u64)tiles_buffer->count * (u64)sizeof(Tile);
		f32 used_tiles_ratio = (f32)((f64)tiles_buffer->count / tiles_max_capacity);
		V4 text_color = V4{used_tiles_ratio, (1.0f-used_tiles_ratio), 0.0f, 1.0f};
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %i/%llu Tiles", tiles_buffer->count, (u64)tiles_max_capacity);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %llu/%zu Bytes", tiles_bytes, tiles_buffer->buffer.count);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);

		// --------------- UI Buffer ---------------
		_snprintf_s(text_buffer, sizeof(text_buffer), "UI buffer:");
		print_debug_text(text_buffer, debug_font_consola, ui_buffer);

		f64 ui_max_capacity = (f64)ui_buffer->buffer.count/(f64)sizeof(Tile);
		u64 ui_render_bytes = (u64)last_frame_ui_buffer_used * (u64)sizeof(Tile);
		f32 used_ui_quads_ratio = (f32)(last_frame_ui_buffer_used / ui_max_capacity);
		text_color = V4{used_ui_quads_ratio, (1.0f-used_ui_quads_ratio), 0.0f, 1.0f};
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %llu/%llu Tiles", (u64)last_frame_ui_buffer_used, (u64)ui_max_capacity);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %llu/%zu Bytes", ui_render_bytes, ui_buffer->buffer.count);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);
		
		// --------------- Game Memory ---------------
		_snprintf_s(text_buffer, sizeof(text_buffer), "Game Memory:");
		print_debug_text(text_buffer, debug_font_consola, ui_buffer);
		u64 game_memory_used = (u64)tiles_buffer->count*sizeof(Tile) + sizeof(Game_State) + (u64)last_frame_ui_buffer_used*sizeof(Tile);
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %llu/%zu Bytes", game_memory_used, game_memory->permanent_storage.count);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);

		// --------------- Render Buffer ---------------
		_snprintf_s(text_buffer, sizeof(text_buffer), "Render buffer:");
		print_debug_text(text_buffer, debug_font_consola, ui_buffer);

		f64 quads_max_capacity = (f64)render_buffer->buffer.count/(f64)sizeof(Quad);
		u64 render_bytes = (u64)last_frame_render_buffer_used * (u64)sizeof(Quad);
		f32 used_quads_ratio = (f32)(last_frame_render_buffer_used / quads_max_capacity);
		text_color = V4{used_quads_ratio, (1.0f-used_quads_ratio), 0.0f, 1.0f};
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %llu/%llu Quads", (u64)last_frame_render_buffer_used, (u64)quads_max_capacity);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %llu/%zu Bytes", render_bytes, tiles_buffer->buffer.count);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer, text_color);

		// --------------- Dude ---------------
		_snprintf_s(text_buffer, sizeof(text_buffer), "dude world_index:");
		print_debug_text(text_buffer, debug_font_consola, ui_buffer);
		_snprintf_s(text_buffer, sizeof(text_buffer), "   %.2f, %.2f", dude->world_index.x, dude->world_index.y);
		print_debug_text(text_buffer, debug_font_consola, ui_buffer);

		if(game_state->editing_tile)
		{
			// NOTE(Fermin): Rethink how get_tile should be used, these parameters seem inconvenient
			Tile *editing;
			if(get_tile(&tiles_buffer->buffer,
			   game_state->level_cols,
			   game_state->level_rows,
			   game_state->editing_tile_x,
			   game_state->editing_tile_y,
			   &editing))
			{
				_snprintf_s(text_buffer, sizeof(text_buffer), "Editing tile:");
				print_debug_text(text_buffer, debug_font_consola, ui_buffer);

				_snprintf_s(text_buffer, sizeof(text_buffer), "   x: %i, y: %i", game_state->editing_tile_x, game_state->editing_tile_y);
				print_debug_text(text_buffer, debug_font_consola, ui_buffer);

				_snprintf_s(text_buffer, sizeof(text_buffer), "   texture_id: %i", editing->texture_id);
				print_debug_text(text_buffer, debug_font_consola, ui_buffer);
			}
		}
	}
	// ------------------ DEBUG PRINTS ------------------

	assert(render_buffer->count == 0)
	render_tiles(game_state, tiles_buffer, render_buffer);
	render_ui(game_state, ui_buffer, render_buffer);
}

extern "C" GAME_GET_SOUND_SAMPLES(game_get_sound_samples)
{
#define Tau32 6.28318530717958647692f
	static float tsine = 0.0f;
    i16 tone_volume = 3000;
	i32 tone_hz = 400;
    int wave_period = sound_buffer->samples_per_second / tone_hz;

    i16 *sample_out = sound_buffer->samples;
    for(int sample_index = 0;
        sample_index < sound_buffer->sample_count;
        ++sample_index)
    {
#if 0
        float sine_value = sinf(tsine);
        i16 sample_value = (i16)(sine_value * tone_volume);
#else
        i16 sample_value = 0;
#endif
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
#if 0
        tsine += Tau32*1.0f/(float)wave_period;
        if(tsine > Tau32)
        {
            tsine -= Tau32;
        }
#endif
    }
}
