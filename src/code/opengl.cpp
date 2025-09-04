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
}

inline void
opengl_rectangle(V3 *corners, V4 pre_mul_color, u32 texture_id, V2 min_uv = {0, 0}, V2 max_uv = {1, 1})
{
    f32 z = corners[0].z;

    glBindTexture(GL_TEXTURE_2D, texture_id);
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

inline void
opengl_rectangle(V3 min_p, V3 max_p, V4 pre_mul_color, u32 texture_id, V2 min_uv = {0, 0}, V2 max_uv = {1, 1})
{
    f32 z = min_p.z;

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glColor4f(pre_mul_color.r, pre_mul_color.g, pre_mul_color.b, pre_mul_color.a);

    glBegin(GL_TRIANGLES);

    // NOTE(Fermin): Lower triangle
    glTexCoord2f(min_uv.x, min_uv.y);
    glVertex3f(min_p.x, min_p.y, z);

    glTexCoord2f(max_uv.x, min_uv.y);
    glVertex3f(max_p.x, min_p.y, z);

    glTexCoord2f(max_uv.x, max_uv.y);
    glVertex3f(max_p.x, max_p.y, z);

    // NOTE(Fermin): Upper triangle
    glTexCoord2f(min_uv.x, min_uv.y);
    glVertex3f(min_p.x, min_p.y, z);

    glTexCoord2f(max_uv.x, max_uv.y);
    glVertex3f(max_p.x, max_p.y, z);

    glTexCoord2f(min_uv.x, max_uv.y);
    glVertex3f(min_p.x, max_p.y, z);

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

    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, opengl.default_internal_texture_format,
                 width, height, 0, GL_BGRA_EXT,
                 GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

static u32
opengl_generate_texture(u8 *data, i32 width, i32 height, u32 format)
{

    u32 result;
    glGenTextures(1, &result);  
    glBindTexture(GL_TEXTURE_2D, result);  

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

    return result;
}

static u32
opengl_generate_texture(char *path, u32 format)
{
    // TODO(Fermin): Maybe use opengl.default_internal_texture_format here?
    u32 result;

    Buffer buffer = read_file(path);

    i32 width =    *(i32 *)buffer.data;
    i32 height =   *(((i32 *)buffer.data) + 1);
    i32 channels = *(((i32 *)buffer.data) + 2);
    u8 *data = buffer.data + sizeof(i32)*3;

    result = opengl_generate_texture(data, width, height, format);

    return result;
}

static void
opengl_render(i32 window_width, i32 window_height, Game_State* game_state)
{
    M4 ortho = {};
    ortho = orthogonal((f32)window_width, (f32)window_height);

    glViewport(0, 0, window_width, window_height);

    //opengl_allocate_texture(buffer.width, buffer.height, buffer.memory);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL); // GL_LESS

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    glUseProgram(opengl.program);
        
    glUniform1i(opengl.texture_sampler_id, 0);

    // NOTE(Fermin): We need to be careful with decimals here, otherwise we'll see gaps between tiles.
    // Should we truncate? round? Not sure
    V2 half_window =
    {
        (f32)(window_width / 2),
        (f32)(window_height / 2)
    };

    // NOTE(Fermin): Renders the world tiles
    ortho.m[2].w = 1.0f; // Enables perspective divide by z
    glUniformMatrix4fv(opengl.transform_id, 1, GL_FALSE, ortho.e);
    f32 tile_size_in_px = game_state->tile_size_in_px;
    Rect *rects = (Rect *)tiles_buffer.buffer.data;
    for(u32 index = 0; index < tiles_buffer.count; index++)
    {
        Rect *rect = rects + index;

        // rotate axis
        V2 x_axis = V2{_cos(rect->rotation), _sin(rect->rotation)};
        V2 y_axis = V2{-x_axis.y, x_axis.x};

        // scale by half dim because origin of rotation is at the center of the tile
        x_axis *= rect->dim_in_tiles.x * 0.5f;
        y_axis *= rect->dim_in_tiles.y * 0.5f;

        V3 origin = rect->world_index - game_state->camera.pos; // move into camera space
        origin.xy = origin.xy + rect->dim_in_px / 2.0f; // set origin in center of tile

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

        opengl_rectangle(corners, rect->color, rect->texture_id);
    }

    // NOTE(Fermin): Renders the UI
    ortho.m[2].w = 0.0f; // NOTE(Fermin): Disables perspective divide by z
    glUniformMatrix4fv(opengl.transform_id, 1, GL_FALSE, ortho.e);

    Rect *ui_rects = (Rect *)ui_buffer.buffer.data;
    for(u32 index = 0; index < ui_buffer.count; index++)
    {
        Rect *rect = ui_rects + index;

        V3 min_p = rect->pos_in_screen;
        V3 max_p = min_p;
        max_p.xy += rect->dim_in_px;

        opengl_rectangle(min_p, max_p, rect->color, rect->texture_id);
    }

    glUseProgram(0);
}
