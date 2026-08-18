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
	layout(location = 2) in float in_light;

	uniform mat4 u_view_proj;

	out vec2 v_uv;
	out float v_light;

	void main()
	{
		v_uv = in_uv;
		v_light = in_light;
		gl_Position = u_view_proj * vec4(in_position, 1.0);
	}
)FOO";

char *doom_fragment_code = R"FOO(
	in vec2 v_uv;
	in float v_light;

	uniform sampler2D u_flat_texture;

	out vec4 out_color;

	void main()
	{
		vec4 tex_color = texture(u_flat_texture, v_uv);
		out_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//out_color = vec4(tex_color.rgb * v_light, tex_color.a);
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
