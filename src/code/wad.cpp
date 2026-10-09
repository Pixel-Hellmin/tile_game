struct RGB8
{
	u8 r, g, b;
};

struct Patch_Name
{
	char name[9];
};

struct Texture_Patch_Ref
{ 
	i16 origin_x, origin_y, patch_index;
};

struct Composite_Texture_Def
{
    char name[9];
    u32 width, height;
    Texture_Patch_Ref *patches;
    u32 patch_count;
};

struct Lump_Info
{ 
	char name[9];
	i32 file_pos;
	i32 size;
};

struct Wad_File
{
	u8 *data; // whole file, memory-mapped or read in wholesale
	Lump_Info *lumps;
	u32 lump_count;
};

struct Loaded_Flat
{
	char name[9];
	u32 id;
};

struct Sector;
struct Side_Def
{
    //char *upper_texture;
    //char *lower_texture;
    //char *middle_texture;
    char upper_texture[8];
    char lower_texture[8];
    char middle_texture[8];
    f32 x_offset, y_offset;
	Sector *sector;
};

struct Line_Def
{
	u32 v1, v2;           // indices into the vertex array
	Sector *front_sector;
	Sector *back_sector;  // NULL for a one-sided line -- same meaning as sidenum[1] == -1
	Side_Def *front_side;
	Side_Def *back_side;  // only meaningful when back_sector != NULL
};

struct Sector
{
    f32 floor_height;
    f32 ceiling_height;
    char floor_texture[9];    // NOTE(Fermin): DOOM lump names are 8 chars + null
    char ceiling_texture[9];
    f32 light_level;          // 0..1, normalized from the WAD's 0..255 lightlevel

    Line_Def **lines;         // NOTE(Fermin): every line whose front or back references this sector
    u32 line_count;

    Mesh floor_mesh;
    Mesh ceiling_mesh;
};

static u8 *
get_lump_data(Wad_File *wad, i32 idx)
{
	assert(idx >= 0);

	return wad->data + wad->lumps[idx].file_pos;
}

static RGB8 *
load_playpal(Wad_File *wad, i32 lump, Memory_Arena *arena)
{
	/*
	* Parses PLAYPAL lump
	* https://doomwiki.org/wiki/PLAYPAL
	*
	* Loads pallete of colors textures index into.
	*/

	u8 *raw = (u8 *)get_lump_data(wad, lump);

    RGB8 *palette = push_array(arena, 256, RGB8);
    for(u32 i = 0; i < 256; ++i)
    {
        palette[i].r = raw[i*3 + 0];
        palette[i].g = raw[i*3 + 1];
        palette[i].b = raw[i*3 + 2];
    }

    return palette; // NOTE(Fermin): palette 0 only
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
	// IMPORTANT (FERMIN): Make sure this is init to 0
	result.pixels = push_array(arena, pixel_count * 4, u8); // push_array zero-inits -> alpha=0 (transparent) everywhere by default

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
				result.pixels[dest+0] = c.r;
				result.pixels[dest+1] = c.g;
				result.pixels[dest+2] = c.b;
				result.pixels[dest+3] = 255;
			}
		}
	}
	return result;
}

static Patch_Name *
load_pnames(u8 *lump_data, u32 *out_count, Memory_Arena *arena)
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

	i32 count = *(i32 *)lump_data;
	Patch_Name *names = push_array(arena, count, Patch_Name);

	u8 *p = lump_data + 4;
	for(i32 i = 0; i < count; ++i)
	{
		//memcpy(names[i].name, p, 8);
		copy_string((char *)p, names[i].name, 8);
		names[i].name[8] = 0; // null-terminate, WAD names aren't
		p += 8;
	}

	*out_count = (u32)count;
	return names;
}

static Composite_Texture_Def *
load_texture_defs(u8 *lump_data, u32 *out_count, Memory_Arena *arena)
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

    i32 numtextures = *(i32 *)lump_data;
    i32 *offsets = (i32 *)(lump_data + 4);
    Composite_Texture_Def *defs = push_array(arena, numtextures, Composite_Texture_Def);

    for(i32 t = 0; t < numtextures; ++t)
    {
        u8 *tex = lump_data + offsets[t];
        //memcpy(defs[t].name, tex, 8);
		copy_string((char *)tex, defs[t].name, 8);
        defs[t].name[8] = 0;

        defs[t].width  = (u32)(*(i16 *)(tex + 12));
        defs[t].height = (u32)(*(i16 *)(tex + 14));

        i16 patch_count = *(i16 *)(tex + 20);
        defs[t].patch_count = (u32)patch_count;
        defs[t].patches = push_array(arena, patch_count, Texture_Patch_Ref);

        u8 *patch_data = tex + 22;
        for(i16 p = 0; p < patch_count; ++p)
        {
			// NOTE(Fermin): A map patch's size is 10.
			// https://doomwiki.org/wiki/TEXTURE1_and_TEXTURE2#Map_patches_structure.2C_binary_data
            i16 *pf = (i16 *)(patch_data + p * 10);
            defs[t].patches[p].origin_x    = pf[0];
            defs[t].patches[p].origin_y    = pf[1];
            defs[t].patches[p].patch_index = pf[2]; // index into PNAMES, not a lump number yet
        }
    }
    *out_count = (u32)numtextures;
    return defs;
}

static Decoded_Patch
composite_texture(Composite_Texture_Def *def, u8 **resolved_patch_lumps, RGB8 *palette, Memory_Arena *arena)
{
	Decoded_Patch result = {};
	result.width = def->width;
	result.height = def->height;

	u32 pixel_count = result.width * result.height;
	// IMPORTANT (FERMIN): Make sure this is init to 0
	result.pixels = push_array(arena, pixel_count * 4, u8); // zero-init -> fully transparent base
	//
	for(u32 i = 0; i < def->patch_count; ++i)
	{
		Texture_Patch_Ref *ref = def->patches + i;
		Decoded_Patch patch = decode_patch(resolved_patch_lumps[ref->patch_index], palette, arena);

		for(u32 y = 0; y < patch.height; ++y)
		{
			i32 dest_y = ref->origin_y + y;
			if(dest_y < 0 || (u32)dest_y >= result.height) { continue; }

			for(u32 x = 0; x < patch.width; ++x)
			{
				i32 dest_x = ref->origin_x + x;
				if(dest_x < 0 || (u32)dest_x >= result.width) { continue; }

				u32 src = (y * patch.width + x) * 4;
				if(patch.pixels[src + 3] == 0) { continue; } // alpha; transparent source pixel -- don't stamp, preserve whatever's underneath

				u32 dst = ((u32)dest_y * result.width + (u32)dest_x) * 4;
				result.pixels[dst+0] = patch.pixels[src+0];
				result.pixels[dst+1] = patch.pixels[src+1];
				result.pixels[dst+2] = patch.pixels[src+2];
				result.pixels[dst+3] = 255;
			}
		}
	}
	return result;
}

static void
load_wall_texture(char *texture_name, umm name_length,
				  Composite_Texture_Def *defs, u32 def_count,
				  u8 **resolved_patch_lumps, RGB8 *palette, Memory_Arena *arena)
{
	for(u32 i = 0; i < def_count; ++i)
	{
		// @Cleanup: Build a lookup table or hash instead of this linear scan
		if(strings_are_equal(name_length, texture_name, defs[i].name))
		//if(strcmp(defs[i].name, texture_name) == 0)
		{
			Decoded_Patch composited = composite_texture(&defs[i], resolved_patch_lumps, palette, arena);
			// TODO: Return texture handle and upload patch to gpu
			//upload_patch_to_gpu(&composited);

			return;
		}
	}

	assert(!"texture name not found in TEXTURE1/TEXTURE2"); // shouldn't happen on a well-formed WAD
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
	assert(strings_are_equal(4, ident, "IWAD") || strings_are_equal(4, ident, "PWAD"));
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
		copy_string((char *)(entry + 8), result.lumps[i].name, 8);
		//memcpy(result.lumps[i].name, entry + 8, 8);
		result.lumps[i].name[8] = 0;
	}
	return result;
}

static i32
find_lump(Wad_File *wad, char *name)
{
	for(u32 i = 0; i < wad->lump_count; ++i)
	{
		if(strings_are_equal(8, wad->lumps[i].name, name)) { return (i32)i; }
	}

	return -1; // matches W_CheckNumForName's "not found" convention
}

static i32
find_map_lump(Wad_File *wad, i32 map_marker, char *name)
{
	// NOTE(Fermin): map data lumps sit within 10 entries after the marker
	for(i32 i = map_marker + 1; i <= map_marker + 10 && i < (i32)wad->lump_count; ++i)
	{
		if(strings_are_equal(8, wad->lumps[i].name, name)) { return i; }
	}
	return -1;
}

static u32
get_lump_size(Wad_File *wad, i32 idx)
{
	assert(idx >= 0);

	return (u32)wad->lumps[idx].size;
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
load_sectors(Wad_File *wad, i32 lump, u32 *out_count, Memory_Arena *arena)
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

		sectors[i].floor_height   = (f32)(*(i16 *)(e + 0));
		sectors[i].ceiling_height = (f32)(*(i16 *)(e + 2));

		copy_string((char *)(e + 4), sectors[i].floor_texture, 8);
		sectors[i].floor_texture[8] = 0;
		//memcpy(sectors[i].floor_texture,   e + 4,  8); sectors[i].floor_texture[8]   = 0;

		copy_string((char *)(e + 12), sectors[i].ceiling_texture, 8);
		sectors[i].ceiling_texture[8] = 0;
		//memcpy(sectors[i].ceiling_texture, e + 12, 8); sectors[i].ceiling_texture[8] = 0;

		sectors[i].light_level = (f32)(*(i16 *)(e + 20)) / 255.0f;
		
		// special @ e+22, tag @ e+24 -- doors/lifts, not consumed yet
	}

	*out_count = count;
	return sectors;
}

static Side_Def *
load_sidedefs(Wad_File *wad, i32 lump, Sector *sectors, u32 *out_count, Memory_Arena *arena)
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
		sides[i].x_offset = (f32)(*(i16 *)(e + 0));
		sides[i].y_offset = (f32)(*(i16 *)(e + 2));

		copy_string((char *)(e + 4), sides[i].upper_texture, 8);
		sides[i].upper_texture[8]  = 0;
		copy_string((char *)(e + 12), sides[i].lower_texture, 8);
		sides[i].lower_texture[8]  = 0;
		copy_string((char *)(e + 20), sides[i].middle_texture, 8);
		sides[i].middle_texture[8] = 0;
		//memcpy(sides[i].upper_texture,  e + 4,  8); sides[i].upper_texture[8]  = 0;
		//memcpy(sides[i].lower_texture,  e + 12, 8); sides[i].lower_texture[8]  = 0;
		//memcpy(sides[i].middle_texture, e + 20, 8); sides[i].middle_texture[8] = 0;

		i16 sector_index = *(i16 *)(e + 28);
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
	i16 *raw = (i16 *)get_lump_data(wad, lump);

	Line_Def *lines = push_array(arena, count, Line_Def);
	for(u32 i = 0; i < count; ++i)
	{
		i16 *e = raw + i * 7; // by 7 because points to i16s
		lines[i].v1 = (u32)(u16)e[0]; // double cast to avoid sign extension
		lines[i].v2 = (u32)(u16)e[1];
		// NOTE(Fermin): flags @ e[2], special @ e[3], tag @ e[4] -- not consumed yet

		i16 front_side_index = e[5];
		lines[i].front_side   = &sides[front_side_index];
		lines[i].front_sector = sides[front_side_index].sector;

		i16 back_side_index = e[6];
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
get_or_load_flat(Wad_File *wad, u8 *palette, char *name, Platform_API *platform_API,
				 Memory_Arena *flat_arena)
{
	/*
	* https://doomwiki.org/wiki/Flat
	*
	* Each flat is a named lump of 4096 bytes representing a 64×64 square.
	* These lumps are between F_START and F_END
	*/

	Loaded_Flat *loaded_flat_base = (Loaded_Flat *)flat_arena->base;

	size_t entry_count = flat_arena->cached / sizeof(Loaded_Flat);
	for(u32 i = 0; i < entry_count; ++i)
	{
	    Loaded_Flat *loaded_flat = loaded_flat_base + i;
		if(strings_are_equal(8, loaded_flat->name, name)) { return loaded_flat->id; }
	}

	i32 first = find_lump(wad, "F_START");
	i32 last  = find_lump(wad, "F_END");
	for(i32 i = last - 1; i > first; --i)
	{
		if(!strings_are_equal(8, wad->lumps[i].name, name)) { continue; }
		if(wad->lumps[i].size != 64 * 64) { return 0; } // marker lump, not a flat
		
		u8 *src = wad->data + wad->lumps[i].file_pos; // indices
		u32 rgba[64 * 64];
		for(u32 p = 0; p < 64 * 64; ++p)
		{
		    u8 *c = palette + src[p] * 3; // * 3 because palette is RGB8 *
			rgba[p] = 0xFF000000u | ((u32)c[2] << 16) | ((u32)c[1] << 8) | c[0];
		}
		u32 id = platform_API->upload_flat_to_gpu(rgba);

		Loaded_Flat *new_entry = push_struct(flat_arena, Loaded_Flat);
		flat_arena->cached = flat_arena->used;
		copy_string(name, new_entry->name, 8);
		new_entry->name[8] = 0;
		new_entry->id = id;

		return id;
	}

	assert(!"TODO: Handle texture not found");

	return 0; // not found
}
