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

static RGB8 *
load_playpal(u8 *playpal_lump_data, Memory_Arena *arena)
{
	/*
	* Parses PLAYPAL lump
	* https://doomwiki.org/wiki/PLAYPAL
	*
	* Loads pallete of colors textures index into.
	*/

    RGB8 *palette = push_array(arena, 256, RGB8);
    for(u32 i = 0; i < 256; ++i)
    {
        palette[i].r = playpal_lump_data[i*3 + 0];
        palette[i].g = playpal_lump_data[i*3 + 1];
        palette[i].b = playpal_lump_data[i*3 + 2];
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
