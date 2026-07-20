#include "windows_opengl.h"

static Opengl_Info
opengl_get_info(b32 modern_context)
{
    Opengl_Info result = {};

    result.modern_context = modern_context;
    result.vendor = (char *)glGetString(GL_VENDOR);
    result.renderer = (char *)glGetString(GL_RENDERER);
    result.version = (char *)glGetString(GL_VERSION);
    if(result.modern_context)
    {
        result.shading_language_version = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        result.shading_language_version = "(none)";
    }
    result.extensions = (char *)glGetString(GL_EXTENSIONS);

    char *at = result.extensions;
    while(*at)
    {
        while(is_whitespace(*at)) {++at;}
        char *end = at;
        while(*end && !is_whitespace(*end)) {++end;}

        umm count = end - at;

        if(0) {}
        else if(strings_are_equal(count, at, "GL_EXT_texture_sRGB")) {result.GL_EXT_texture_sRGB = true;}
        else if(strings_are_equal(count, at, "GL_EXT_framebuffer_sRGB")) {result.GL_EXT_framebuffer_sRGB = true;}

        at = end;
    }

    return result;
}

static GLuint
opengl_create_program(char *defines, char *header_code, char *vertex_code, char *fragment_code)
{
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLchar *vertex_shader_code[] =
    {
        defines,
        header_code,
        vertex_code,
    };
    glShaderSource(vertex_shader_id, array_count(vertex_shader_code),
                   vertex_shader_code, 0);
    glCompileShader(vertex_shader_id);

    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar *fragment_shader_code[] =
    {
        defines,
        header_code,
        fragment_code,
    };
    glShaderSource(fragment_shader_id, array_count(fragment_shader_code),
                   fragment_shader_code, 0);
    glCompileShader(fragment_shader_id);

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    glValidateProgram(program_id);
    GLint linked = false;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        GLsizei ignored;
        char vertex_errors[4096];
        char fragment_errors[4096];
        char program_errors[4096];
        glGetShaderInfoLog(vertex_shader_id, sizeof(vertex_errors), &ignored, vertex_errors);
        glGetShaderInfoLog(fragment_shader_id, sizeof(fragment_errors), &ignored, fragment_errors);
        glGetProgramInfoLog(program_id, sizeof(program_errors), &ignored, program_errors);

        assert(!"Shader validation failed");
    }

    return program_id;
}

static void
opengl_init(Opengl_Info info)
{
    opengl.default_internal_texture_format = GL_RGBA8;
    if(info.GL_EXT_texture_sRGB)
    {
        opengl.default_internal_texture_format = GL_SRGB8_ALPHA8;
    }

    if(info.GL_EXT_framebuffer_sRGB)
    {
        glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &opengl.max_multisample_count);
        if(opengl.max_multisample_count > 16)
        {
            opengl.max_multisample_count = 16;
        }

        GLuint test_texture;
        glGenTextures(1, &test_texture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, test_texture);
        glGetError(); // Clear the error
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                opengl.max_multisample_count,
                                GL_SRGB8_ALPHA8,
                                1920, 1080,
                                GL_FALSE);

        if(glGetError() == GL_NO_ERROR)
        {
            glEnable(GL_FRAMEBUFFER_SRGB);
            //opengl.default_frame_buffer_texture_format = GL_SRGB8_ALPHA8;
        }
        glDeleteTextures(1, &test_texture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    char *defines = "#version 130\n";
    if(false)
    {
        defines = R"FOO(
        #version 130
        #define example 1
        )FOO";
    }

    char *header_code = R"FOO(
    // Header code
    )FOO";

    char *vertex_code = R"FOO(
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

    char *fragment_code = R"FOO(
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

    opengl.program = opengl_create_program(defines, header_code, vertex_code, fragment_code);
    opengl.transform_id = glGetUniformLocation(opengl.program, "transform");
    opengl.texture_sampler_id = glGetUniformLocation(opengl.program, "texture_sampler");

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

	opengl.filter_program = opengl_create_program(defines, header_code, filter_vertex_code, filter_fragment_code);
	opengl.filter_texture_sampler_id = glGetUniformLocation(opengl.filter_program, "texture_sampler");
}

static void
opengl_init_fbo(i32 window_width, i32 window_height)
{
	if(opengl.fbo != 0)
	{
		glDeleteFramebuffers(1, &opengl.fbo);
		glDeleteTextures(1, &opengl.fbo_texture);
		glDeleteRenderbuffers(1, &opengl.fbo_depth);
	}

	glGenFramebuffers(1, &opengl.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, opengl.fbo);

	// color texture
	glGenTextures(1, &opengl.fbo_texture);
	glBindTexture(GL_TEXTURE_2D, opengl.fbo_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, opengl.default_internal_texture_format, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, opengl.fbo_texture, 0);

	// depth renderbuffer
	glGenRenderbuffers(1, &opengl.fbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, opengl.fbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, opengl.fbo_depth);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


inline void
opengl_rectangle(V3 *corners, V4 pre_mul_color, u32 texture_id, V2 min_uv = {0, 0}, V2 max_uv = {1, 1})
{
    f32 z = corners[0].z;

    glBindTexture(GL_TEXTURE_2D, texture_id);
    //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glColor4f(pre_mul_color.r, pre_mul_color.g, pre_mul_color.b, pre_mul_color.a);

    glBegin(GL_TRIANGLES);

    // NOTE(Fermin): Lower triangle
    glTexCoord2f(min_uv.x, min_uv.y);
    glVertex3f(corners[0].x, corners[0].y, z);

    glTexCoord2f(max_uv.x, min_uv.y);
    glVertex3f(corners[1].x, corners[1].y, z);

    glTexCoord2f(max_uv.x, max_uv.y);
    glVertex3f(corners[2].x, corners[2].y, z);

    // NOTE(Fermin): Upper triangle
    glTexCoord2f(min_uv.x, min_uv.y);
    glVertex3f(corners[0].x, corners[0].y, z);

    glTexCoord2f(max_uv.x, max_uv.y);
    glVertex3f(corners[2].x, corners[2].y, z);

    glTexCoord2f(min_uv.x, max_uv.y);
    glVertex3f(corners[3].x, corners[3].y, z);

    glEnd();
}

static void //internal void *
opengl_allocate_texture(u32 width, u32 height, void *data)
{
    /*
    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, opengl.default_internal_texture_format,
                 width, height, 0, GL_BGRA_EXT,
                 GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBindTexture(GL_TEXTURE_2D, 0);

    // NOTE(Fermin): In this case we are not actually pointing to any memory.
    // We use this void * type to store a u32 which is the texture handle.
    // We do this because opengl uses u32 as texture handles but we may want
    // to use actual pointers on the cpu side. This way we can store both
    // in a (void *)
    assert(sizeof(handle) <= sizeof(void *));
    void *result = pointer_from_u32(void, handle);
    return result;
    */

    glBindTexture(GL_TEXTURE_2D, 1); // texture handle?
    glTexImage2D(GL_TEXTURE_2D, 0, opengl.default_internal_texture_format,
                 width, height, 0, GL_BGRA_EXT,
                 GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

static void
opengl_load_texture(u8 *data, i32 width, i32 height, u32 *id, u32 format)
{
	// NOTE(Fermin): Use glDeleteTextures(1, id); if we want to explicitly
	// unload the texture

	if(*id == 0)
	{
		glGenTextures(1, id);  
	}
    glBindTexture(GL_TEXTURE_2D, *id);  

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, opengl.default_internal_texture_format,
                     width, height, 0, format, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
    }
}

static void
opengl_load_texture(char *path, u32 *id, u32 format)
{
    // TODO(Fermin): Maybe use opengl.default_internal_texture_format here?
    Buffer buffer = read_file(path);

    i32 width =    *(i32 *)buffer.data;
    i32 height =   *(((i32 *)buffer.data) + 1);
    i32 channels = *(((i32 *)buffer.data) + 2);
    u8 *data = buffer.data + sizeof(i32)*3;

    opengl_load_texture(data, width, height, id, format);
}

static void
opengl_render(i32 window_width, i32 window_height, Memory_Arena* render_arena)
{
    time_function;

    M4 ortho = {};
    ortho = orthogonal((f32)window_width, (f32)window_height);

	glBindFramebuffer(GL_FRAMEBUFFER, opengl.fbo); // NOTE(Fermin): Render to frame buffer
    glViewport(0, 0, window_width, window_height);
	//glScissor(0, 0, window_width, window_height);

    //opengl_allocate_texture(buffer.width, buffer.height, buffer.memory);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL); // GL_LESS

    glEnable(GL_TEXTURE_2D);
    //glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    glUseProgram(opengl.program);
        
    glUniform1i(opengl.texture_sampler_id, 0);

    ortho.m[2].w = 1.0f; // Enables perspective divide by z
    glUniformMatrix4fv(opengl.transform_id, 1, GL_FALSE, ortho.e);

    Quad *quads = (Quad *)render_arena->base;
	u32 quad_count = render_arena->used / sizeof(Quad);
    for(u32 index = 0; index < quad_count; index++)
    {
        Quad *quad = quads + index;
        opengl_rectangle(quad->corners, quad->color, quad->texture_id);
    }

    glUseProgram(0);
}

static void
opengl_render_to_screen()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // 0 = back to screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	glUseProgram(opengl.filter_program);
	glUniform1i(opengl.filter_texture_sampler_id, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, opengl.fbo_texture);
	glBegin(GL_TRIANGLES);

    // lower triangle
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    // upper triangle
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);

    glEnd();
	glUseProgram(0);
	glEnable(GL_BLEND);
}
