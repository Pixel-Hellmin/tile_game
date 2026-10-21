static u32
make_magenta_texture(Platform_API *platform_API)
{
	u32 pixels[64 * 64];
	for(u32 i = 0; i < 64 * 64; ++i) { pixels[i] = 0xFFFF00FFu; }
	return platform_API->upload_texture_to_gpu(pixels, 64, 64);
}

static u8 *
get_lump_data(Wad_File *wad, i32 idx)
{
	assert(idx >= 0);

	return wad->data + wad->lumps[idx].file_pos;
}

static i32
find_lump(Wad_File *wad, char *name)
{
	for(u32 i = 0; i < wad->lump_count; ++i)
	{
		if(strings_are_equal_case_insensitive(wad->lumps[i].name, name, 8)) { return (i32)i; }
	}

	return -1; // matches W_CheckNumForName's "not found" convention
}

static i32
find_last_lump_between(Wad_File *wad, char *name, i32 first, i32 last_exclusive)
{
		for(i32 i = last_exclusive - 1; i >= first; --i)
		{
			if(strings_are_equal_case_insensitive(wad->lumps[i].name, name, 8)) { return i; }
		}

		return -1;
}

static u32
find_flat_number(Texture_Cache *cache, Wad_File *wad, char *name)
{
	i32 lump = find_last_lump_between(wad, name, cache->first_flat, cache->last_flat);
	if(lump < 0 || wad->lumps[lump].size != 64 * 64)
	{
		printf("missing flat: %.8s\n", name);
		return cache->flat_count; // fallback texture
	}

	return (u32)(lump - cache->first_flat);
}

static u32
find_texture_number(Texture_Cache *cache, char *name)
{
	if(name[0] == '-') { return 0; }

	for(u32 i = cache->texture_count; i-- > 0; )   // backward, later definitions win
	{
		if(strings_are_equal_case_insensitive(cache->defs[i].name, name, 8)) { return i; }
	}

	printf("missing texture: %.8s\n", name);

	return cache->texture_count;
}

static i32
find_map_lump(Wad_File *wad, i32 map_marker, char *name)
{
	// NOTE(Fermin): map data lumps sit within 10 entries after the marker
	for(i32 i = map_marker + 1; i <= map_marker + 10 && i < (i32)wad->lump_count; ++i)
	{
		if(strings_are_equal_case_insensitive(wad->lumps[i].name, name, 8)) { return i; }
	}
	return -1;
}

static u32
get_lump_size(Wad_File *wad, i32 idx)
{
	assert(idx >= 0);

	return (u32)wad->lumps[idx].size;
}

static void
load_playpal(Wad_File *wad, Texture_Cache *cache, Memory_Arena *arena)
{
	/*
	* Parses PLAYPAL lump
	* https://doomwiki.org/wiki/PLAYPAL
	*
	* Loads pallete of colors textures index into.
	*/

	i32 lump = find_lump(wad, "PLAYPAL");
	assert(lump  >= 0);

	u8 *raw = (u8 *)get_lump_data(wad, lump);

	// NOTE(Fermin): Loading palette 0 only
    cache->palette = push_array(arena, 256, RGB8);
    for(u32 i = 0; i < 256; ++i)
    {
        cache->palette[i].r = raw[i*3 + 0];
        cache->palette[i].g = raw[i*3 + 1];
        cache->palette[i].b = raw[i*3 + 2];
    }
}

static Decoded_Patch
decode_patch(u8 *lump_data, RGB8 *palette, Memory_Arena *arena)
{
	/*
	 * Decoding a single patch lump or graphic stored in picture format
	 * https://doomwiki.org/wiki/Picture_format#Format
	 *
	 * Patch header format
	 * field       type   size     offset  desc
	 * width       u16    2        0	   width of graphic
	 * height	   u16    2		   2	   height of graphic
	 * leftoffset  i16    2  	   4	   offset in pixels to the left of the origin
	 * topoffset   i16    2  	   6	   offset in pixels below the origin
	 * columnofs   u32[]  4*width  8	   array of column offsets relative to the beginning of the patch header
	 *
	 * Posts
	 * Each column is an array of post_t, of indeterminate length, terminated by a byte with value 0xFF (255).
	 * A post_t is a chunk(in rows) of non-transparent pixels.
	 *
	 * Post format
	 * field     type  size    offset    desc
	 * topdelta  u8    1	   0		 the y offset of this post in this patch. If 0xFF, then end-of-column
	 * length    u8    1	   1		 length of data in this post
	 * unused    u8    1  	   2		 unused padding byte
	 * data		 u8[]  length  3		 array of pixels in this post; each pixel is an index into the palette
	 * unused    u8    1	   3+length  unused padding byte
	 *
	*/

	i16 *header = (i16 *)lump_data;

	Decoded_Patch result = {};
	result.width       = (u32)header[0];
	result.height      = (u32)header[1];
	result.left_offset = header[2];
	result.top_offset  = header[3];

	u32 pixel_count = result.width * result.height;
	result.rgba = push_array(arena, pixel_count * 4, u8); // push_array zero-inits -> alpha=0 (transparent) everywhere by default

	i32 *column_offsets = (i32 *)(lump_data + 8); // 4 x i16

	for(u32 x = 0; x < result.width; ++x)
	{
		u8 *post_ptr = lump_data + column_offsets[x];
		for(;;)
		{
			u8 top_delta = *post_ptr++;
			if(top_delta == 0xFF) { break; } // end-of-column marker

			u8 length = *post_ptr++;
			post_ptr++; // skip padding byte
			u8 *pixel_indices = post_ptr;
			post_ptr += length;
			post_ptr++; // skip trailing padding byte

			for(u8 y = 0; y < length; ++y)
			{
				u32 dest_y = top_delta + y;
				u32 dest = (dest_y * result.width + x) * 4;
				RGB8 c = palette[pixel_indices[y]];
				result.rgba[dest+0] = c.r;
				result.rgba[dest+1] = c.g;
				result.rgba[dest+2] = c.b;
				result.rgba[dest+3] = 255;
			}
		}
	}
	return result;
}

static void
load_pnames(Wad_File *wad, Texture_Cache *cache, Memory_Arena *arena)
{
	/*
	 * Parsing PNAMES lump
	 * https://doomwiki.org/wiki/PNAMES
	 * 
	 * Binary data
	 * offset  length			name		   content
	 * 0x00    4      			nummappatches  int holding the number of following patches
	 * 0x04    8*nummappatches  name_p[]	   eight-char ASCII strings defining the lump names of the patches
	*/

	i32 pnames_lump = find_last_lump_between(wad, "PNAMES", 0, wad->lump_count);
	assert(pnames_lump >= 0);
	u8 *pnames = wad->data + wad->lumps[pnames_lump].file_pos;

	cache->patch_name_count = read_u32(pnames);
	cache->patch_lumps = push_array(arena, cache->patch_name_count, i32);

	u8 *p = pnames + 4;
	for(i32 i = 0; i < cache->patch_name_count; ++i)
	{
		cache->patch_lumps[i] = find_last_lump_between(wad, (char *)p, 0, wad->lump_count);
		p += 8;
	}
}

static void
load_texture_defs(Wad_File *wad, Texture_Cache *cache, Memory_Arena *arena)
{
	/*
	* Parsing TEXTURE1/TEXTURE2 lump
	* https://doomwiki.org/wiki/TEXTURE1_and_TEXTURE2
	*
	* Header binary data
	* offset     length		    name	     content
	* 0x00		 4			    numtextures  int holding the number of map textures
	* 0x04		 4*numtextures  offset[]	 array of offsets(ints) to the textures in this lump
	* offset[0]  flexible	    mtexture[]	 array with the map texture structures
	* ...
	*
	*
	* Map texture structure binary data
	* offset     length		    name			 content
	* 0x00       8				name			 ASCII string defining the name of the map texture
	* 0x08		 4				masked			 boolean
	* 0x0C		 2				width			 short int defining the width of the map texture
	* 0x0E		 2				height			 short int defining the height of the map texture
	* 0x10		 4				columndirectory  obsolete, ignore
	* 0x14		 2				patchcount		 the number of map patches
	* 0x16		 10*patchcount  patches[]		 array with the map patch structures for this tex
	*
	*/

	i32 texture_lumps[2] =
	{
		find_last_lump_between(wad, "TEXTURE1", 0, wad->lump_count),
		find_last_lump_between(wad, "TEXTURE2", 0, wad->lump_count),
	};
	assert(texture_lumps[0] >= 0);

	cache->texture_count = 0;
	for(u32 t = 0; t < 2; ++t)
	{
		if(texture_lumps[t] < 0) { continue; }
		cache->texture_count += read_u32(wad->data + wad->lumps[texture_lumps[t]].file_pos);
	}

	cache->defs = push_array(arena, cache->texture_count, Composite_Texture_Def);
	cache->textures = push_array(arena, cache->texture_count + 1, Wall_Texture);

	u32 out = 0;
	for(u32 l = 0; l < 2; ++l)
	{

		if(texture_lumps[l] < 0) { continue; }

		u8 *lump_data  = wad->data + wad->lumps[texture_lumps[l]].file_pos;
		u32 numtextures = read_u32(lump_data);
		i32 *offsets = (i32 *)(lump_data + 4);

		for(i32 t = 0; t < numtextures; ++t, ++out)
		{
			u8 *tex = lump_data + offsets[t];

			Composite_Texture_Def *def = cache->defs + out;
			copy_string(def->name, (char *)tex, 8);
			def->name[8] = 0;

			def->width  = read_u16(tex + 12);
			def->height = read_u16(tex + 14);

			u16 patch_count = read_u16(tex + 20);
			def->patch_count = (u32)patch_count;
			def->patches = push_array(arena, patch_count, Texture_Patch_Def);

			u8 *patch_data = tex + 22;
			for(u32 p = 0; p < patch_count; ++p)
			{
				// NOTE(Fermin): A map patch's size is 10.
				// https://doomwiki.org/wiki/TEXTURE1_and_TEXTURE2#Map_patches_structure.2C_binary_data
				u8 *pf = patch_data + p * 10;
				def->patches[p].origin_x    = read_i16(pf);
				def->patches[p].origin_y    = read_i16(pf + 2);
				u16 pname_index             = read_u16(pf + 4); 
				def->patches[p].patch_lump = (pname_index < cache->patch_name_count) ? cache->patch_lumps[pname_index] : -1;
			}

			cache->textures[out].width = def->width;
			cache->textures[out].height = def->height;
		}
	}
}

static Decoded_Patch
composite_texture(Wad_File* wad, Composite_Texture_Def *def, RGB8 *palette, Memory_Arena *arena)
{
	Decoded_Patch result = {};
	result.width = def->width;
	result.height = def->height;

	u32 pixel_count = result.width * result.height;
	result.rgba = push_array(arena, pixel_count * 4, u8); // zero-init -> fully transparent base

	for(u32 i = 0; i < def->patch_count; ++i)
	{
		Texture_Patch_Def *patch_def = def->patches + i;
		if(patch_def->patch_lump < 0)
		{
			printf("patch_lump < 0 for %.8s\n", def->name);
			continue;
		}

		u8 *lump_data = wad->data + wad->lumps[patch_def->patch_lump].file_pos;
		Decoded_Patch patch = decode_patch(lump_data, palette, arena);

		for(u32 y = 0; y < patch.height; ++y)
		{
			i32 dest_y = patch_def->origin_y + y;
			if(dest_y < 0 || (u32)dest_y >= result.height) { continue; }

			for(u32 x = 0; x < patch.width; ++x)
			{
				i32 dest_x = patch_def->origin_x + x;
				if(dest_x < 0 || (u32)dest_x >= result.width) { continue; }

				u32 src = (y * patch.width + x) * 4;
				if(patch.rgba[src + 3] == 0) { continue; } // alpha; transparent source pixel -- don't stamp, preserve whatever's underneath

				u32 dst = ((u32)dest_y * result.width + (u32)dest_x) * 4;
				result.rgba[dst+0] = patch.rgba[src+0];
				result.rgba[dst+1] = patch.rgba[src+1];
				result.rgba[dst+2] = patch.rgba[src+2];
				result.rgba[dst+3] = 255;
			}
		}
	}

	return result;
}

static Wall_Texture *
get_or_load_wall_texture(Wad_File *wad, Texture_Cache *cache, u32 number, Platform_API *platform_API, Memory_Arena *tmp_arena)
{
	Wall_Texture *tex = cache->textures + number;
	if(tex->id) { return tex; }

	Composite_Texture_Def *def = cache->defs + number;
	assert(def->width <= MAX_TEXTURE_SIDE && def->height <= MAX_TEXTURE_SIDE);
	Decoded_Patch composited = composite_texture(wad, def, cache->palette, tmp_arena);
	tex->id = platform_API->upload_texture_to_gpu((u32 *)composited.rgba, composited.width, composited.height);

	return tex;
}

static Wad_File
open_wad_from_memory(u8 *file_data, Memory_Arena *arena)
{
	/*
	* WAD file. "Where's all the data?"
	* https://doomwiki.org/wiki/WAD
	*
	* Header
	* offset  length  name			  content
	* 0x00    4       identification  ASCII characters "IWAD" or "PWAD"
	* 0x04    4       numlumps		  integer specifying the number of lumps in the WAD
	* 0x08    4       infotableofs    integer holding a pointer to the location of the directory
	*
	* Directory
	* offset  length  name	   content
	* 0x00    4       filepos  integer holding a pointer to the start of the lump's data in the file
	* 0x04    4       size	   integer representing the size of the lump in bytes
	* 0x08    8       name     ASCII string defining the lump's name, 8 char max
	*/

	Wad_File result = {0};
	result.data = file_data;

	char *ident = (char *)file_data;
	assert(strings_are_equal(ident, "IWAD", 4) || strings_are_equal(ident, "PWAD", 4));
	//Assert(memcmp(ident, "IWAD", 4) == 0 || memcmp(ident, "PWAD", 4) == 0);

	i32 num_lumps    = *(i32 *)(file_data + 4);
	i32 dir_offset   = *(i32 *)(file_data + 8);

	result.lump_count = (u32)num_lumps;
	result.lumps = push_array(arena, result.lump_count, Lump_Info);

	u8 *dir = file_data + dir_offset;
	for(u32 i = 0; i < result.lump_count; ++i)
	{
		u8 *entry = dir + i * 16; // filelump_t is 16 bytes -- filepos(4) + size(4) + name(8)
		result.lumps[i].file_pos = *(i32 *)(entry + 0);
		result.lumps[i].size     = *(i32 *)(entry + 4);
		copy_string(result.lumps[i].name, (char *)(entry + 8), 8);
		//memcpy(result.lumps[i].name, entry + 8, 8);
		result.lumps[i].name[8] = 0;
	}
	return result;
}

static V2 *
load_vertexes(Wad_File *wad, i32 lump, u32 *out_count, Memory_Arena *arena)
{
	/*
	* Lump: VERTEXES
	* https://doomwiki.org/wiki/Vertex
	*
	* Vertex structure
	* offset  size  type  description
	* 0		  2		i16	  x position
	* 2		  2	    i16   y position
	*/

	u32 count = get_lump_size(wad, lump) / 4;
	i16 *raw = (i16 *)get_lump_data(wad, lump);

	V2 *verts = push_array(arena, count, V2);
	for(u32 i = 0; i < count; ++i)
	{ 
		verts[i] = { (f32)raw[i*2], (f32)raw[i*2+1] };
	}

	*out_count = count;
	return verts;
}

static Sector *
load_sectors(Wad_File *wad, i32 lump, u32 *out_count, Memory_Arena *arena, Texture_Cache *cache)
{
	/*
	* Lump: SECTORS
	* https://doomwiki.org/wiki/Sector
	*
	* Sector structure
	* offset  size  type   description
	* 0		  2		i16	   floor height
	* 2		  2	    i16    ceiling height
	* 4       8     i8[8]  name of floor texture(flat)
	* 12      8     i8[8]  name of ceiling texture(flat)
	* 20	  2		i16	   light level
	* 22	  2	    i16	   special type
	* 24	  2	    i16	   tag number
	*/

	u32 count = get_lump_size(wad, lump) / 26;
	u8 *raw = get_lump_data(wad, lump);

	Sector *sectors = push_array(arena, count, Sector);
	for(u32 i = 0; i < count; ++i)
	{
		u8 *e = raw + i * 26;

		sectors[i].floor_height   = (f32)read_i16(e + 0);
		sectors[i].ceiling_height = (f32)read_i16(e + 2);
		sectors[i].floor_flat	  = find_flat_number(cache, wad, (char *)(e + 4));
		sectors[i].ceiling_flat   = find_flat_number(cache, wad, (char *)(e + 12));
		sectors[i].light_level	  = (f32)read_i16(e + 20) / 255.0f;
		
		// special @ e+22, tag @ e+24 -- doors/lifts, not consumed yet
	}

	*out_count = count;
	return sectors;
}

static Side_Def *
load_sidedefs(Wad_File *wad, i32 lump, Sector *sectors, u32 *out_count,
			  Memory_Arena *arena, Texture_Cache *cache)
{
	/*
	* Lump: SIDEDEFS
	* https://doomwiki.org/wiki/Sidedef
	*
	* Sidedef structure
	* offset  size  type   description
	* 0		  2		i16	   x offset
	* 2		  2	    i16    y offset
	* 4       8     i8[8]  name of upper texture
	* 12      8     i8[8]  name of lower texture
	* 20	  8		i8[8]  name of middle texture
	* 28	  2	    i16	   sector number this sidedef faces
	*/

	u32 count = get_lump_size(wad, lump) / 30;
	u8 *raw = get_lump_data(wad, lump);

	Side_Def *sides = push_array(arena, count, Side_Def);
	for(u32 i = 0; i < count; ++i)
	{
		u8 *e = raw + i * 30;
		sides[i].x_offset = (f32)read_i16(e + 0);
		sides[i].y_offset = (f32)read_i16(e + 2);
		sides[i].upper_texture = find_texture_number(cache, (char *)(e + 4));
		sides[i].lower_texture = find_texture_number(cache, (char *)(e + 12));
		sides[i].middle_texture = find_texture_number(cache, (char *)(e + 20));

		i16 sector_index = read_i16(e + 28);
		sides[i].sector = &sectors[sector_index];
	}

	*out_count = count;
	return sides;
}

static Line_Def *
load_linedefs(Wad_File *wad, i32 lump, Side_Def *sides, u32 *out_count, Memory_Arena *arena)
{
	/*
	* Lump: LINEDEFS
	* https://doomwiki.org/wiki/Linedef
	*
	* Linedef structure
	* offset  size  type   code        description
	* 0		  2		i16	   v1		   starting vertex
	* 2		  2	    i16    v2		   ending vertex
	* 4       2     i16    flags	   flags: attribute bits
	* 6       2     i16    special	   linedef type: special action or behavior
	* 8 	  2		i16    tag		   tag: associates sector(s)/line(s) with special
	* 10	  2	    i16	   sidenum[0]  front sidedef
	* 12	  2	    i16	   sidenum[1]  back sidedef
	*/

	u32 count = get_lump_size(wad, lump) / 14;
	u8 *raw = get_lump_data(wad, lump);

	Line_Def *lines = push_array(arena, count, Line_Def);
	for(u32 i = 0; i < count; ++i)
	{
		u8 *e = raw + i * 14; // by 14 because points to u8
		lines[i].v1 = (u32)read_u16(e + 0); // double cast to avoid sign extension
		lines[i].v2 = (u32)read_u16(e + 2);
		lines[i].flags = read_u16(e + 4);
		// NOTE(Fermin): special @ e[6], tag @ e[8] -- not consumed yet

		i16 front_side_index = read_i16(e + 10);
		lines[i].front_side   = &sides[front_side_index];
		lines[i].front_sector = sides[front_side_index].sector;

		i16 back_side_index = read_i16(e + 12);
		if(back_side_index == -1)
		{ 
			lines[i].back_sector = NULL;
		}
		else
		{
			lines[i].back_side   = &sides[back_side_index];
			lines[i].back_sector = sides[back_side_index].sector;
		}
	}

	*out_count = count;
	return lines;
}

static u32
get_or_load_flat_id(Wad_File *wad, Texture_Cache *cache, u32 flat_number, Platform_API *platform_API)
{
	/*
	* https://doomwiki.org/wiki/Flat
	*
	* Each flat is a named lump of 4096 bytes representing a 64×64 square.
	* These lumps are between F_START and F_END
	*/

	u32 *id = cache->flat_ids + flat_number;
	if(*id) { return *id; }

	u8 *src = wad->data + wad->lumps[cache->first_flat + flat_number].file_pos; // indices
	u32 rgba[64 * 64];
	for(u32 p = 0; p < 64 * 64; ++p)
	{
		u8 *c = (u8 *)cache->palette + src[p] * 3; // * 3 because palette is RGB8 *
		rgba[p] = 0xFF000000u | ((u32)c[2] << 16) | ((u32)c[1] << 8) | c[0];
	}

	*id = platform_API->upload_texture_to_gpu(rgba, 64, 64);

	return *id;
}

static void
init_texture_cache(Game_State *game_state, Platform_API *platform_API)
{
	Wad_File *wad = &game_state->wad;
	Memory_Arena *arena = &game_state->tmp_arena; // TODO(Fermin): Use right arena
	Texture_Cache *cache = &game_state->texture_cache;

	u32 fallback_texture = make_magenta_texture(platform_API);

	cache->first_flat = find_last_lump_between(wad, "F_START", 0, wad->lump_count) + 1;
	cache->last_flat = find_last_lump_between(wad, "F_END", 0, wad->lump_count);
	assert(cache->first_flat > 0 && cache->last_flat > cache->first_flat);
	cache->flat_count = cache->last_flat - cache->first_flat;
	cache->flat_ids = push_array(arena, cache->flat_count + 1, u32);
	cache->flat_ids[cache->flat_count] = fallback_texture;
	cache->sky_flat_number = find_flat_number(cache, wad, "F_SKY1");

	load_pnames(wad, cache, arena);
	load_playpal(wad, cache, arena);
	load_texture_defs(wad, cache, arena);

	cache->textures[cache->texture_count] = { fallback_texture, 64, 64 };
}
