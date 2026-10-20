#if !defined(WAD_H)

#define ML_DONTPEGTOP    0x0008
#define ML_DONTPEGBOTTOM 0x0010
#define MAX_TEXTURE_SIDE 1024

struct RGB8
{
	u8 r, g, b;
};

struct Wall_Texture
{
	u32 id; // 0 = not composited
	u16 width;
	u16 height;
};

struct Texture_Patch_Def
{ 
	i16 origin_x, origin_y;
	i32 patch_lump;
};

struct Composite_Texture_Def
{
    char name[9];
    u16 width, height;
    Texture_Patch_Def *patches;
    u32 patch_count;
};

struct Lump_Info
{ 
	char name[9];
	i32 file_pos;
	i32 size;
};

struct Texture_Cache
{
	RGB8 *palette;

	// NOTE(Fermin): flat number = lump index - first_flat.
	// Entry [flat_count] is the missing-flat fallback.
	i32 first_flat;
	i32 last_flat; // exclusive (index of F_END)
	u32 flat_count;
	u32 *flat_ids; // flat_count + 1
	u32 sky_flat_number;

	// NOTE(Fermin): texture number = index into TEXTURE1 + TEXTURE2. 0 means "-" (no texture).
	// Entry [texture_count] is the missing-texture fallback.
	u32 texture_count;
	Composite_Texture_Def *defs;
	Wall_Texture *textures; // texture_count + 1

	u32 patch_name_count;
	i32 *patch_lumps; // PNAMES index -> lump index
};

struct Wad_File
{
	u8 *data; // whole file, memory-mapped or read in wholesale
	Lump_Info *lumps;
	u32 lump_count;
};

struct Sector;
struct Side_Def
{
    u32 upper_texture;
    u32 lower_texture;
    u32 middle_texture;
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
	u16 flags;
};

struct Sector
{
    f32 floor_height;
    f32 ceiling_height;
    u32 floor_flat;			  // flat numbers
    u32 ceiling_flat;
    f32 light_level;          // 0..1, normalized from the WAD's 0..255 lightlevel

    Line_Def **lines;         // NOTE(Fermin): every line whose front or back references this sector
    u32 line_count;

    Mesh floor_mesh;
    Mesh ceiling_mesh;
};

#define WAD_H
#endif
