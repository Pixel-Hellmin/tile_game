//char *defines = "#version 130\n";
char *defines = "#version 330 core\n";

char *header_code = R"FOO(
// Header code
)FOO";

char *tiles_vertex_code = R"FOO(
	// Vertex code
	uniform mat4x4 transform;
	smooth out vec2 frag_uv;
	smooth out vec4 frag_color;
	void main(void)
	{
		// NOTE(Fermin): This rounding still doesn't fix the gaps between
		// tiles when they are small. That is when their z is high.
		gl_Position = transform*round(gl_Vertex);

		frag_uv = gl_MultiTexCoord0.xy;
		frag_color = gl_Color;
	}
)FOO";

char *tiles_fragment_code = R"FOO(
	// Fragment code
	uniform sampler2D texture_sampler;
	smooth in vec2 frag_uv;
	smooth in vec4 frag_color;
	out vec4 result_color;
	void main(void)
	{
		vec4 tex_sample = texture(texture_sampler, frag_uv);
		result_color = frag_color*tex_sample;
	}
)FOO";

char *doom_vertex_code = R"FOO(
	layout(location = 0) in vec3 in_position;
	layout(location = 1) in vec2 in_uv;
	layout(location = 2) in uint in_sector;

	uniform mat4 u_view_proj;

	out vec2 v_uv;
	flat out uint v_sector;

	void main()
	{
		v_uv = in_uv;
		v_sector = in_sector;
		gl_Position = u_view_proj * vec4(in_position, 1.0);
	}
)FOO";

char *doom_fragment_code = R"FOO(
	in vec2 v_uv;
	flat in uint v_sector; // flat to avoid interpolation

	uniform usampler2D u_texture;     // unit 0: RG8UI (index, coverage)
	uniform sampler2D u_palette;      // unit 1: 256 x 14 RGBA8
	uniform usampler2D u_colormap;    // unit 2: 256 x 34 R8UI
	uniform usampler2D u_light_table; // unit 3: 0..255 R8UI
	uniform int u_extralight;         // used to light up the room when you fire a weapon
	uniform int u_fixed_colormap;     // -1 = none; colormap override for effects like invulnerability 
	uniform int u_palette_row;

	out vec4 out_color;

	void main()
	{
		uvec2 t = texture(u_texture, v_uv).rg;
		// NOTE(Fermin): binary transparency, like vanilla.
		// This disables or weakens early-z testing for all fragments
		if(t.g == 0u) { discard; } // coverage

		int sector = int(v_sector);
		// x: mask width-1, y:sector/width
		int raw = int(texelFetch(u_light_table, ivec2(sector & 255, sector >> 8), 0).r);

		int lightnum = clamp((raw >> 4) + u_extralight, 0, 15); // vanilla has 16 light levels [0..15]
		int row = (u_fixed_colormap >= 0) ? u_fixed_colormap : min((15 - lightnum) * 4, 31); // missing closeness term

		uint index = texelFetch(u_colormap, ivec2(int(t.r), row), 0).r;
		vec3 rgb   = texelFetch(u_palette, ivec2(int(index), u_palette_row), 0).rgb;
		out_color  = vec4(rgb, 1.0);
	}
)FOO";

char *filter_vertex_code = R"FOO(
	smooth out vec2 frag_uv;
	void main(void)
	{
		gl_Position = gl_Vertex;
		frag_uv = gl_MultiTexCoord0.xy;
	}
)FOO";

char *filter_fragment_code = R"FOO(
	uniform sampler2D texture_sampler;
	smooth in vec2 frag_uv;
	out vec4 result_color;
	void main(void)
	{
		vec4 color = texture(texture_sampler, frag_uv);
		
		// scanlines
		float scanline = sin(frag_uv.y * 800.0) * 0.04;
		color.rgb -= scanline;
		
		// slight vignette
		vec2 uv_centered = frag_uv - 0.5;
		float vignette = 1.0 - dot(uv_centered, uv_centered) * 2.0;
		color.rgb *= vignette;
		
		// green tint
		color.rgb *= vec3(0.8, 1.1, 0.8);
		
		result_color = color;
	}
)FOO";
