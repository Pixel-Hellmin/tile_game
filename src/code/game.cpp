#include "game.h"
#include "asset.cpp"
#include "audio.cpp"
#include "wad.cpp"
#include "sectors.cpp"

static void
partition_memory(Game_State *game_state, Game_Memory *game_memory)
{
	size_t total_memory_partitioned = sizeof(Game_State);

	initialize_arena(&game_state->level_arena, gigabytes(1),
					 game_memory->permanent_storage.data + total_memory_partitioned);
	total_memory_partitioned += game_state->level_arena.size;

	initialize_arena(&game_state->persistent_arena, gigabytes(1),
					 game_memory->permanent_storage.data + total_memory_partitioned);
	total_memory_partitioned += game_state->persistent_arena.size;

	initialize_arena(&game_state->audio_state.arena, (game_memory->permanent_storage.size - total_memory_partitioned),
					 game_memory->permanent_storage.data + total_memory_partitioned);
	total_memory_partitioned += game_state->audio_state.arena.size;

	assert(total_memory_partitioned == game_memory->permanent_storage.size)
}

static void
load_level(char *level_name, Platform_API *platform_API, Game_State *game_state)
{
	Wad_File *wad                = &game_state->wad;
	Memory_Arena *level_arena    = &game_state->level_arena;
	Memory_Arena *scratch        = &game_state->tmp_arena;
	Texture_Cache *texture_cache = &game_state->texture_cache;

	reset_arena(level_arena);
	platform_API->clear_static_mesh_buffers();

	Tmp_Memory tmp_memory = begin_tmp_memory(scratch);

	i32 map			  = find_lump(wad, level_name);		     assert(map >= 0);
	i32 vertexes_lump = find_map_lump(wad, map, "VERTEXES"); assert(vertexes_lump >= 0);
	i32 sectors_lump  = find_map_lump(wad, map, "SECTORS");  assert(sectors_lump  >= 0);
	i32 sidedefs_lump = find_map_lump(wad, map, "SIDEDEFS"); assert(sidedefs_lump >= 0);
	i32 linedefs_lump = find_map_lump(wad, map, "LINEDEFS"); assert(linedefs_lump >= 0);

	/*
	 * NOTE(Fermin): Load order matters and is fixed:
	 *	Sectors -> Side_Defs -> Line_Defs
	*/
	u32 vertex_count, sector_count, side_count, line_count;
	V2       *vertex_positions = load_vertexes(wad, vertexes_lump, &vertex_count, level_arena);
	Sector   *sectors          = load_sectors(wad, sectors_lump, &sector_count, level_arena, texture_cache);
	Side_Def *sides            = load_sidedefs(wad, sidedefs_lump, sectors, &side_count, level_arena, texture_cache);
	Line_Def *lines            = load_linedefs(wad, linedefs_lump, sides, &line_count, level_arena);

	group_lines_into_sectors(lines, line_count, sectors, sector_count, level_arena, scratch);
	for(u32 i = 0; i < sector_count; ++i)
	{
		build_sector_render_data(&sectors[i], vertex_positions, scratch,
								 platform_API, wad, texture_cache, i,
								 map);
	}

	Wall_Build_Context context = {};
	context.cache = texture_cache;
	context.wad = wad;
	context.platform_API = platform_API;
	context.scratch = scratch;
	for(u32 i = 0; i < line_count; ++i)
	{
		Wall_Segment_List segment_list = build_wall_segments_for_line(&lines[i], vertex_positions, &context);
		for(u32 i = 0; i < segment_list.segment_count; ++i)
		{
			Wall_Segment *segment = segment_list.segments + i;
			platform_API->upload_static_mesh_to_gpu(&segment->mesh);
		}
	}

	game_state->loaded_level.vertex_positions = vertex_positions;
	game_state->loaded_level.vertex_count	  = vertex_count;
	game_state->loaded_level.sectors          = sectors;
	game_state->loaded_level.sector_count     = sector_count;
	game_state->loaded_level.sides            = sides;
	game_state->loaded_level.side_count       = side_count;
	game_state->loaded_level.lines            = lines;
	game_state->loaded_level.line_count       = line_count;

	end_tmp_memory(tmp_memory);
}

// NOTE(Fermin): extern "C" makes the compiler not mangle the function
// name so we can link to it with GetProcAddress for dynamic loading
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(game_memory->permanent_storage.size >= sizeof(Game_State));
	Game_State *game_state = (Game_State *)game_memory->permanent_storage.data;
	debug_print_font = &game_memory->debug_font_consola;

    if(!game_state->initialized)
    {
		game_state->entropy.index = 666;

		partition_memory(game_state, game_memory);
		initialize_arena(&game_state->tmp_arena,
						 game_memory->temporary_storage.size,
						 game_memory->temporary_storage.data);

		initialize_audio_state(&game_state->audio_state);

		Buffer wad_buffer = game_memory->platform_API.read_file_from_disk("..\\src\\misc\\assets\\DOOM.WAD");
		game_state->wad = open_wad_from_memory(wad_buffer.data, &game_state->persistent_arena);
		init_texture_cache(game_state, &game_memory->platform_API);

		/*
		* Keep the index texture, do the palette lookup in the shader instead of 
		* uploading a decoded texture to opengl.
		*/
		load_level("E1M9", &game_memory->platform_API, game_state);

		set_flag(game_state, game_state_flag_prints);

		game_state->test_sound = DEBUG_load_WAV("..\\src\\misc\\assets\\sounds\\test_music.wav");
		game_state->test_music = play_sound(&game_state->audio_state, &game_state->test_sound);

		*game_memory->debug_player_pos = { 1544.0f, -3380.0f, 0.0f };
		*game_memory->debug_player_angle = 1.5707963f;

        game_state->initialized = 1;
    }
	change_pitch(&game_state->audio_state, game_state->test_music, 1.125f);
    
    Input_Keys new_input = input[0];
    Input_Keys old_input = input[1];
    f32 dt_in_seconds = new_input.dt_in_seconds;
    f32 dude_speed = 50.0 * dt_in_seconds;
    if(new_input.f1 && !old_input.f1)
    {
		load_level("E1M1", &game_memory->platform_API, game_state);
        toggle_flag(game_state, game_state_flag_prints);
    }
    if(new_input.f3 && !old_input.f3)
    {
        toggle_flag(game_state, game_state_flag_free_cam_mode);
		game_state->test_music = play_sound(&game_state->audio_state, &game_state->test_sound);
    }

    if(new_input.w)
    {
		game_memory->debug_player_pos->y += 3.0f * dude_speed;
		change_volume(game_state->test_music, 2.0f, V2{1.0f, 1.0f});
    }
    if(new_input.s)
    {
		game_memory->debug_player_pos->y += -3.0f * dude_speed;
		change_volume(game_state->test_music, 2.0f, V2{0.0f, 0.0f});
    }
    if(new_input.a)
    {
		game_memory->debug_player_pos->x += -3.0f * dude_speed;
		change_volume(game_state->test_music, 5.0f, V2{1.0f, 0.0f});
    }
    if(new_input.d)
    {
		game_memory->debug_player_pos->x += 3.0f * dude_speed;
		change_volume(game_state->test_music, 5.0f, V2{0.0f, 1.0f});
    }
    if(new_input.q)
    {
		*game_memory->debug_player_angle += 0.2f * dude_speed;
    }
    if(new_input.e)
    {
		*game_memory->debug_player_angle += -0.2f * dude_speed;
    }
    if(new_input.space)
    {
		game_memory->debug_player_pos->z += 3.0f * dude_speed;
    }
    if(new_input.shift)
    {
		game_memory->debug_player_pos->z -= 3.0f * dude_speed;
    }

    // NOTE(Fermin): Experimental particle system logic START
	/*
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

        Tile *part = push_struct(&game_state->world_arena, Tile);
        part->world_index = particle->p;
        part->dim_in_tiles = V2{0.4, 0.4};
        part->rotation = particle->rotation;
        part->texture_id = game_state->level_assets.wall_texture_id;
        part->color = color_trans;
    }
	*/
    // NOTE(Fermin): Experimental particle system logic END
}

extern "C" GAME_GET_SOUND_SAMPLES(game_get_sound_samples)
{
	Game_State *game_state = (Game_State *)game_memory->permanent_storage.data;
	Game_Audio_State *audio_state = &game_state->audio_state;

	output_playing_sounds(audio_state, sound_output_buffer, &game_state->tmp_arena);
}
