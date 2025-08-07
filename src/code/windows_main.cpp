#include "windows.h"
#include "windows_main.h"
#include "game.h"
#include <gl/gl.h>

global_variable u32 screen_width = 1280;
global_variable u32 screen_height = 720;
global_variable f32 aspect_ratio = ((f32)screen_width)/((f32)screen_height);
global_variable f32 fov = 45.0f;
global_variable M4 projection = perspective(radians(fov), aspect_ratio, 1.0f, 100.0f);
global_variable f32 last_mouse_x = ((f32)screen_width) / 2.0f;
global_variable f32 last_mouse_y = ((f32)screen_height) / 2.0f;
global_variable b32 mouse_enabled = 1;
global_variable f32 camera_pitch = 0.0f;
global_variable f32 camera_yaw = -90.0f;
global_variable u32 bytes_per_pixel = 4;
global_variable Font consola = {};
global_variable u32 dude_texture_id;
// TODO(Fermin): This goes in opengl header
struct Opengl {
    GLuint program;
    GLuint transform_id;
    GLuint texture_sampler_id;
    GLuint default_internal_texture_format;
    GLint max_multisample_count;
};
global_variable Opengl opengl = {};
// END(Fermin): This goes in opengl header


// NOTE(Fermin): | Start | win32 stuff
struct Win32_Offscreen_Buffer
{
    BITMAPINFO info;
    void *memory;
    i32 width;
    i32 height;
    i32 pitch;
    i32 bytes_per_pixel;
};

struct Win32_Window_Dimension
{
    i32 width;
    i32 height;
};

global_variable b32 win32_running;
global_variable Win32_Offscreen_Buffer global_back_buffer;
global_variable GLuint texture_handle;

typedef BOOL WINAPI Wgl_Swap_Interval_Ext(int interval);
global_variable Wgl_Swap_Interval_Ext *wgl_swap_interval;

typedef HGLRC WINAPI Wgl_Create_Context_Attribs_Arb(HDC hdc, HGLRC h_share_context,
                                                    const int *attrib_list);

// NOTE(Fermin): Got these from https://registry.khronos.org/OpenGL/api/GL/glcorearb.h
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100


typedef char GLchar;
typedef void Gl_Attach_Shader(GLuint program, GLuint shader);
typedef void Gl_Compile_Shader(GLuint shader);
typedef GLuint Gl_Create_Program(void);
typedef GLuint Gl_Create_Shader(GLenum type);
typedef void Gl_Link_Program(GLuint program);
typedef void Gl_Shader_Source(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void Gl_Use_Program(GLuint program);
typedef void Gl_Validate_Program(GLuint program);
typedef void Gl_Get_Programiv(GLuint program, GLenum pname, GLint *params);
typedef void Gl_Get_Shader_Info_Log(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void Gl_Get_Program_Info_Log(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint Gl_Get_Uniform_Location(GLuint program, const GLchar *name);
typedef void Gl_Uniform_Matrix_4vf(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void Gl_Uniform_1i(GLint location, GLint v0);
typedef void Gl_Tex_Image_2D_Multisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
//typedef void Gl_Bind_Attrib_Location(GLuint program, GLuint index, const GLchar *name);

global_variable Gl_Attach_Shader *glAttachShader;
global_variable Gl_Compile_Shader *glCompileShader;
global_variable Gl_Create_Program *glCreateProgram;
global_variable Gl_Create_Shader *glCreateShader;
global_variable Gl_Link_Program *glLinkProgram;
global_variable Gl_Shader_Source *glShaderSource;
global_variable Gl_Use_Program *glUseProgram;
global_variable Gl_Validate_Program *glValidateProgram;
global_variable Gl_Get_Programiv *glGetProgramiv;
global_variable Gl_Get_Shader_Info_Log *glGetShaderInfoLog;
global_variable Gl_Get_Program_Info_Log *glGetProgramInfoLog;
global_variable Gl_Get_Uniform_Location *glGetUniformLocation;
global_variable Gl_Uniform_Matrix_4vf *glUniformMatrix4fv;
global_variable Gl_Uniform_1i *glUniform1i;
global_variable Gl_Tex_Image_2D_Multisample *glTexImage2DMultisample;
 

// NOTE(Fermin): Windows specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

inline b32
strings_are_equal(umm a_length, char *a, char *b)
{
    char *at = b;
    for(umm index = 0;
        index < a_length;
        ++index, ++at)
    {
        if((*at == 0) ||
           (a[index] != *at))
        {
            return false;
        }
    }

    b32 result = (*at == 0);
    return result;
}

inline b32
is_end_of_line(char c)
{
    b32 result = ((c == '\n') ||
                  (c == '\r'));

    return result;
}

inline b32
is_whitespace(char c)
{
    b32 result = ((c == ' ') ||
                  (c == '\t') ||
                  (c == '\v') ||
                  (c == '\f') ||
                  is_end_of_line(c));

    return result;
}

struct Opengl_Info
{
    b32 modern_context;
    char *vendor;
    char *renderer;
    char *version;
    char *shading_language_version;
    char *extensions;

    b32 GL_EXT_texture_sRGB;
    b32 GL_EXT_framebuffer_sRGB;
};

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

    // NOTE(Fermin): Not using this check atm. This is for frame buffer texture format
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
    //in vec2 in_uv;
    //in vec4 in_color;
    smooth out vec2 frag_uv;
    smooth out vec4 frag_color;
    void main(void)
    {
        gl_Position = transform*gl_Vertex;

        frag_uv = gl_MultiTexCoord0.xy; //in_uv;
        frag_color = gl_Color; //in_color;
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

static void
win32_init_opengl(HWND window)
{
    HDC window_dc = GetDC(window);

    PIXELFORMATDESCRIPTOR desired_pixel_format = {};
    desired_pixel_format.nSize = sizeof(desired_pixel_format);
    desired_pixel_format.nVersion = 1;
    desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
    desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
    desired_pixel_format.cColorBits = 32;
    desired_pixel_format.cAlphaBits = 8;
    desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
    
    i32 suggested_pixel_format_index = ChoosePixelFormat(window_dc, &desired_pixel_format);
    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    DescribePixelFormat(window_dc, suggested_pixel_format_index,
                        sizeof(suggested_pixel_format),
                        &suggested_pixel_format);
    SetPixelFormat(window_dc, suggested_pixel_format_index,
                   &suggested_pixel_format);

    HGLRC open_GLRC = wglCreateContext(window_dc);
    if(wglMakeCurrent(window_dc, open_GLRC))
    {
        b32 modern_context = false;

        Wgl_Create_Context_Attribs_Arb *wgl_create_context_attribs_arb =
            (Wgl_Create_Context_Attribs_Arb *)wglGetProcAddress("wglCreateContextAttribsARB");
        if(wgl_create_context_attribs_arb)
        {
            // NOTE(Fermin): Modern verson of openGL
            i32 attribs[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 0,
                WGL_CONTEXT_FLAGS_ARB, 0 // WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if 0 // NOTE(Fermin): Debug 
                |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
                ,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                0,
            };

            HGLRC share_context = 0;
            HGLRC modern_GLRC = wgl_create_context_attribs_arb(window_dc, share_context, attribs);
            if(modern_GLRC)
            {
                if(wglMakeCurrent(window_dc, modern_GLRC))
                {
                    modern_context = true;
                    wglDeleteContext(open_GLRC);
                    open_GLRC = modern_GLRC;
                }
            }
        }
        else
        {
            // NOTE(Fermin): Antiquated verson of openGL
        }

        glAttachShader = (Gl_Attach_Shader *)wglGetProcAddress("glAttachShader");
        glCompileShader = (Gl_Compile_Shader *)wglGetProcAddress("glCompileShader");
        glCreateProgram = (Gl_Create_Program *)wglGetProcAddress("glCreateProgram");
        glCreateShader = (Gl_Create_Shader *)wglGetProcAddress("glCreateShader");
        glLinkProgram = (Gl_Link_Program *)wglGetProcAddress("glLinkProgram");
        glShaderSource = (Gl_Shader_Source *)wglGetProcAddress("glShaderSource");
        glUseProgram = (Gl_Use_Program *)wglGetProcAddress("glUseProgram");
        glValidateProgram = (Gl_Validate_Program *)wglGetProcAddress("glValidateProgram");
        glGetProgramiv = (Gl_Get_Programiv *)wglGetProcAddress("glGetProgramiv");
        glGetShaderInfoLog = (Gl_Get_Shader_Info_Log *)wglGetProcAddress("glGetShaderInfoLog");
        glGetProgramInfoLog = (Gl_Get_Program_Info_Log *)wglGetProcAddress("glGetProgramInfoLog");
        glGetUniformLocation = (Gl_Get_Uniform_Location *)wglGetProcAddress("glGetUniformLocation");
        glUniformMatrix4fv = (Gl_Uniform_Matrix_4vf *)wglGetProcAddress("glUniformMatrix4fv");
        glUniform1i = (Gl_Uniform_1i *)wglGetProcAddress("glUniform1i");
        glTexImage2DMultisample = (Gl_Tex_Image_2D_Multisample  *)wglGetProcAddress("glTexImage2DMultisample");

        wgl_swap_interval = (Wgl_Swap_Interval_Ext *)wglGetProcAddress("wglSwapIntervalEXT");
        if(wgl_swap_interval)
        {
            // NOTE(Fermin): V-Sync
            wgl_swap_interval(1);
        }

        Opengl_Info info = opengl_get_info(modern_context);
        opengl_init(info);
    }
    else
    {
        invalid_code_path
    }
    ReleaseDC(window, window_dc);
}

static Win32_Window_Dimension
win32_get_window_dimension(HWND window)
{
    Win32_Window_Dimension result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return result;
}

static void
render_gradient(Win32_Offscreen_Buffer buffer)
{
    u8 *row = (u8 *)buffer.memory;
    for(i32 y = 0;
        y < buffer.height;
        y++)
    {
        u32 *pixel = (u32 *)row;
        for(i32 x = 0;
            x < buffer.width;
            x++)
        {
            u8 blue = x;
            u8 green = y;

            *pixel++ = (green << 8) | blue;
        }
        row += buffer.pitch;
    }
}

static void
win32_resize_DIB_section(Win32_Offscreen_Buffer *buffer, i32 width, i32 height)
{
    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = 4;
    buffer->pitch = width * buffer->bytes_per_pixel;
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    i32 bitmap_memory_size = (buffer->width * buffer->height) * buffer->bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

inline void
opengl_rectangle(V2 min_p, V2 max_p, V4 pre_mul_color, M4* mat, V2 min_uv = {0, 0}, V2 max_uv = {1, 1})
{
    // NOTE(Fermin): Not sure where this should go atm
    glUseProgram(opengl.program);
    //glBindProgram(opengl.program);
    
    glBindTexture(GL_TEXTURE_2D, dude_texture_id);
    glUniformMatrix4fv(opengl.transform_id, 1, GL_FALSE, mat->e);
    //glUniformMatrix4fv(opengl.transform_id, 1, GL_FALSE, m4_ident().e);
    glUniform1i(opengl.texture_sampler_id, 0);

    glBegin(GL_TRIANGLES);

    glColor4f(pre_mul_color.r, pre_mul_color.g, pre_mul_color.b, pre_mul_color.a);

    // NOTE(Fermin): Lower triangle
    glTexCoord2f(min_uv.x, min_uv.y);
    glVertex2f(min_p.x, min_p.y);

    glTexCoord2f(max_uv.x, min_uv.y);
    glVertex2f(max_p.x, min_p.y);

    glTexCoord2f(max_uv.x, max_uv.y);
    glVertex2f(max_p.x, max_p.y);

    // NOTE(Fermin): Upper triangle
    glTexCoord2f(min_uv.x, min_uv.y);
    glVertex2f(min_p.x, min_p.y);

    glTexCoord2f(max_uv.x, max_uv.y);
    glVertex2f(max_p.x, max_p.y);

    glTexCoord2f(min_uv.x, max_uv.y);
    glVertex2f(min_p.x, max_p.y);

    glEnd();
    glUseProgram(0);
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

static void
win32_display_buffer_in_window(HDC device_context,
                               i32 window_width, i32 window_height,
                               Win32_Offscreen_Buffer buffer)
{
#if 0
    StretchDIBits(device_context,
                  /*
                  x, y, width, height,
                  x, y, width, height,
                  */
                  0, 0, window_width, window_height,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory,
                  &buffer.info,
                  DIB_RGB_COLORS, SRCCOPY);
#endif
    glViewport(0, 0, window_width, window_height);

    opengl_allocate_texture(buffer.width, buffer.height, buffer.memory);

    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glEnable(GL_TEXTURE_2D);
    //glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
 


    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // NOTE(Fermin): Should change this to 3d eventually
    /*
    V2 min_p = {-1.0f, -1.0f};
    V2 max_p = { 1.0f,  1.0f};
    V4 pre_mul_color = {1.0f,  1.0f, 1.0f, 1.0f};
    opengl_rectangle(min_p, max_p, pre_mul_color);
    */

    /*  Orthogonal

        | 2/width      0         0         0 |
        |    0     2/height      0         0 |
        |    0         0         1         0 |
        |   -1        -1         0         1 |
    */
    M4 ortho = {};
    ortho.m[0].e[0] =  2.0f/(f32)window_width;
    ortho.m[1].e[1] =  2.0f/(f32)window_height;
    ortho.m[2].e[2] =  1.0f;
    ortho.m[3].e[3] =  1.0f;
    ortho.m[3].e[0] = -1.0f;
    ortho.m[3].e[1] = -1.0f;

    V2 min_p = {10.0f, 10.0f};
    V2 max_p = {120.0f, 120.0f};
    V4 pre_mul_color = {1.0f,  1.0f, 1.0f, 1.0f};
    opengl_rectangle(min_p, max_p, pre_mul_color, &ortho);

    SwapBuffers(device_context);
}

LRESULT CALLBACK
win32_main_window_callback(HWND window, UINT message, WPARAM w_param,
                           LPARAM l_param)
{
    LRESULT result = 0;

    switch(message)
    {
        case WM_SIZE:
        {
        } break;

        case WM_CLOSE:
        {
            win32_running = 0;
        } break;

        case WM_ACTIVATEAPP:
        {
            printf("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            win32_running = 0;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            Win32_Window_Dimension dimension = win32_get_window_dimension(window);
            win32_display_buffer_in_window(device_context,
                                           dimension.width, dimension.height,
                                           global_back_buffer);
            EndPaint(window, &paint);
        } break;

        default:
        {
            result = DefWindowProc(window, message, w_param, l_param);
        } break;
    }

    return result;
}
// NOTE(Fermin): | End | win32 stuff

#if 0
// OpenGL
static void
framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height)
{
    screen_width = width;
    screen_height = height;
    aspect_ratio = ((f32)screen_width)/((f32)screen_height);
    projection = perspective(radians(fov), aspect_ratio, 1.0f, 100.0f);

    glViewport(0, 0, width, height);
}  

// OpenGL
static void
mouse_callback(GLFWwindow* window, double x_pos, double y_pos)
{
    static b32 first_mouse = 1;
    if(first_mouse)
    {
        last_mouse_x = x_pos;
        last_mouse_y = y_pos;
        first_mouse = 0;
    }

    if(!mouse_enabled)
    {
        f32 x_offset = x_pos - last_mouse_x;
        f32 y_offset = last_mouse_y - y_pos; // reversed since y-coord range from bot to top

        f32 sensitivity = 0.05f;
        x_offset *= sensitivity;
        y_offset *= sensitivity;

        camera_pitch += y_offset;
        camera_yaw += x_offset;

        if(camera_pitch > 89.0f)
            camera_pitch =  89.0f;

        if(camera_pitch < -89.0f)
            camera_pitch = -89.0f;
    }

    last_mouse_x = x_pos;
    last_mouse_y = y_pos;
}

// OpenGL
static void
scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    fov -= y_offset;

    if(fov < 1.0f)
        fov = 1.0f;
    if(fov > 45.0f)
        fov = 45.0f;
}

// OpenGL
static u32
compile_shader(i32 type, char* source)
{
    u32 id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);

    i32 success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::%s::COMPILATION_FAILED\n%s", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
        assert(!"Program validation failed");
    }

    return id;
}

struct Program
{
    GLuint id;
    GLuint model;
    GLuint view;
    GLuint light_pos;
    GLuint color_trans;
    GLuint proj;
    GLuint vao;
};
// OpenGL
static GLuint
build_program(char *vertex_code, char *fragment_code)
{
    u32 vertex_shader_id = compile_shader(GL_VERTEX_SHADER, vertex_code);
    u32 fragment_shader_id = compile_shader(GL_FRAGMENT_SHADER, fragment_code);

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    i32 success;
    char infoLog[512];
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program_id, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::LINKING_FAILED\n%s", infoLog);
        assert(!"Program validation failed");
    } 
    glUseProgram(program_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id); 

    return program_id;
}

static void
get_character_metadata(char character, Glyph_Metadata *out)
{
    // TODO(Fermin): Pass Font so it works when we add more fonts
    size_t index = character - font_first_character;
    Glyph_Metadata character_info = consola.metadata[index];

    out->width = character_info.width;
    out->height = character_info.height;
    out->offset = character_info.offset;
    out->y_offset = character_info.y_offset;
    out->advance = character_info.advance;
}

static void
init_font(Font *font, char *source)
{
    Buffer data = read_file(source);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    Glyph_Metadata *src = (Glyph_Metadata *)(data.data);
    for(char character = font_first_character;
        character <= font_last_character;
        character++)
    {
        u32 index     = character - font_first_character;
        size_t offset = src->offset; 
        i32 width     = src->width;
        i32 height    = src->height;
        i32 y_offset  = src->y_offset;
        i32 advance   = src->advance;
        src++;

        Glyph_Metadata *dest_metadata = &font->metadata[index];
        dest_metadata->offset   = offset;
        dest_metadata->width    = width;
        dest_metadata->height   = height;
        dest_metadata->y_offset = y_offset;
        dest_metadata->advance  = advance;
        
        u32 font_bytes_per_pixel = 1;
        u32 character_bitmap_size = width * font_bytes_per_pixel * height;

        // TODO(Fermin): How can I avoid using a buffer here? Reuse the same one for every character?
        Buffer character_buffer = {};
        character_buffer = allocate_buffer(character_bitmap_size);

        u8 *bitmap_source = data.data + sizeof(Glyph_Metadata) * font_character_count + offset;
        u8 *bitmap_dest = character_buffer.data;
        for(u32 y = 0; y < height; y++)
        {
            for(u32 x = 0; x < width; x++)
            {
                u8 result = *bitmap_source++;
                *bitmap_dest++ = result;
            }
        }

        font->glyph_texture_ids[index] = generate_texture(character_buffer.data, width, height, GL_RED);

        free_buffer(&character_buffer);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    free_buffer(&data);
}
static void
print_debug_text(char *string, Font *font, u32 VBO, u32 program_id)
{
    glActiveTexture(GL_TEXTURE0);

    f32 x = 10.0f;
    f32 font_size = 25.0f;
    f32 base_y = screen_height - font_size - debug_print_line * 27.0f - 4.0f;
    f32 char_size = font_size;
    f32 max_scale = char_size / font_point_size;

    for(char *c = string; *c; c++)
    {
        if(*c != ' ')
        {
            Glyph_Metadata glyph_info = {};
            get_character_metadata(*c, &glyph_info);

            f32 y_scale = glyph_info.height/font_point_size;
            f32 x_scale = glyph_info.width/font_point_size;
            f32 h = char_size * y_scale;
            f32 w = char_size * x_scale;
            f32 y = base_y - glyph_info.y_offset * max_scale;
            f32 advance = glyph_info.advance*max_scale;

            float font_vertices[] = {
                x,     y + h, 0.0f, 1.0f,            
                x,     y,     0.0f, 0.0f,
                x + w, y,     1.0f, 0.0f,

                x,     y + h, 0.0f, 1.0f,
                x + w, y,     1.0f, 0.0f,
                x + w, y + h, 1.0f, 1.0f           
            };

            glBindTexture(GL_TEXTURE_2D, font->glyph_texture_ids[*c - font_first_character]);
            // TODO(Fermin)-IMPORTANT: Figure a way to transform the glyphs without
            // uploading new data each character. Using uniforms and
            // matrices? This is too slow, probably.
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(font_vertices), font_vertices); 
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            x += advance + 1;
        }
        else
        {
            // NOTE(Fermin): space
            x += 8.0f;
        }

    }

    glBindTexture(GL_TEXTURE_2D, 0);

    debug_print_line++;
}

void
draw_rectangles(Program *prog, Render_Buffer *render_buffer, M4 *view, M4 *projection, f32 tile_size_in_meters, V3 light_pos)
{
    // TODO(Fermin): Currently we use a dummy model of a rectangle and
    // transformations to draw the rects. Another option is to upload
    // vertices directly using a dynamic buffer object. Test both and see
    // which is faster
    glUseProgram(prog->id);
    glEnable(GL_BLEND);
    // TODO(Fermin): This is handmade heros blend func, test it
    // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glBindVertexArray(prog->vao);

    glUniformMatrix4fv(prog->view, 1, GL_TRUE, view->e);
    glUniformMatrix4fv(prog->proj, 1, GL_TRUE, projection->e);
    glUniform3f(prog->light_pos, light_pos.x, light_pos.y, light_pos.z);

    f32 meters_to_model = 1.0f / tile_size_in_meters;
    Rect *rects = (Rect *)render_buffer->buffer.data;
    for(u32 index = 0; index < render_buffer->count; index++)
    {
        Rect *rect = rects + index;

        f32 model_width = (rect->max_p.x - rect->min_p.x) * meters_to_model;
        f32 model_height = (rect->max_p.y - rect->min_p.y) * meters_to_model;
        f32 half_width = model_width * 0.5f;
        f32 half_height = model_height * 0.5f;

        // NOTE(Fermin): We want the origin of the rect to be its min_p, we need to
        // translate the rect by width/2 and half/2 in x and y
        M4 translation = translate(V3{
            rect->min_p.x * meters_to_model + half_width,
            rect->min_p.y * meters_to_model + half_height,
            0.0
        });
        M4 scale = scale_m4(V3{model_width, model_height, 1.0f});
        M4 rotation = rotate(rect->rotation, V3{0.0f, 0.0f, 1.0f});
        M4 model = translation * rotation * scale;

        glUniform4f(prog->color_trans, rect->color.r, rect->color.g, rect->color.b, rect->color.a);

        glUniformMatrix4fv(prog->model, 1, GL_TRUE, model.e);
        glBindTexture(GL_TEXTURE_2D, rect->texture_id);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glUseProgram(0);
    glDisable(GL_BLEND);
}
#endif

static void
cat_strings(size_t source_a_count, char *source_a,
            size_t source_b_count, char *source_b, 
            size_t dest_count, char *dest)
{
    for(int index = 0;
        index < source_a_count;
        ++index)
    {
        *dest++ = *source_a++;
    }

    for(int index = 0;
        index < source_b_count;
        ++index)
    {
        *dest++ = *source_b++;
    }

    *dest++ = 0;
}

static long
open_file(FILE **handle, char const *name)
{
    *handle = fopen(name, "rb");
    if (!*handle)
    {
        fprintf(stderr, "Error: Couldn't open file: %s\n", name);
        return -1;
    }

    fseek(*handle, 0, SEEK_END);
    long file_size = ftell(*handle);
    rewind(*handle);

    return file_size;
}

static Buffer
read_file(const char *file_name)
{
    Buffer result = {};

    FILE* file;
    long file_size = open_file(&file, file_name);

    result = allocate_buffer(file_size);

    if (result.data)
    {
        size_t bytes_read = fread(result.data, sizeof(char), result.count, file);
        if ((bytes_read) != result.count)
        {
            fprintf(stderr, "Error: Failed to read %s\nfile_size: %zu, bytes_read: %zu\n", file_name, result.count, bytes_read);

            free_buffer(&result);
            fclose(file);

            return result;
        }
    }
    else
    {
        fprintf(stderr, "Error: Memory allocation failed while reading file %s\n", file_name);
    }

    fclose(file);

    return result;
}
// NOTE(Fermin): read_file declaration should go in main.h, fix error when moving

// OpenGL
static u32
generate_texture(u8 *data, i32 width, i32 height, u32 format)
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
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
    }

    return result;
}

// OpenGL
static u32
generate_texture(char *path, u32 format)
{
    u32 result;

    Buffer buffer = read_file(path);

    i32 width =    *(i32 *)buffer.data;
    i32 height =   *(((i32 *)buffer.data) + 1);
    i32 channels = *(((i32 *)buffer.data) + 2);
    u8 *data = buffer.data + sizeof(i32)*3;

    result = generate_texture(data, width, height, format);

    return result;
}

inline FILETIME
win32_get_last_write_time(char *file_name)
{
    FILETIME result = {};

    WIN32_FIND_DATA find_data;
    HANDLE find_handle = FindFirstFileA(file_name, &find_data);
    if(find_handle != INVALID_HANDLE_VALUE)
    {
        result = find_data.ftLastWriteTime;
        FindClose(find_handle);
    }

    return result;
}

static Game_Code
win32_load_game_code(char *src_dll_name, char *tmp_dll_name)
{
    Game_Code result = {};

    result.dll_last_write_time = win32_get_last_write_time(src_dll_name);

    CopyFile(src_dll_name, tmp_dll_name, FALSE);
    result.game_code_dll = LoadLibraryA(tmp_dll_name);
    if(result.game_code_dll)
    {
        result.update_and_render = (Game_Update_And_Render *)GetProcAddress(result.game_code_dll, "game_update_and_render");

        result.is_valid = result.update_and_render && 1;
    }

    if(!result.is_valid)
    {
        // NOTE(Femin): If we call a function in a null pointer we crash
        result.update_and_render = game_update_and_render_stub;
    }

    return result;
}

static void
win32_unload_game_code(Game_Code *game_code)
{
    if(game_code->game_code_dll)
    {
        FreeLibrary(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }

    game_code->is_valid = false;
    game_code->update_and_render = game_update_and_render_stub;
}

int main()
{
    // NOTE(Fermin): Never use MAX_PATH in code that is user-facing since that is not the max size anymore
    char exe_file_name[MAX_PATH];
    DWORD size_of_file_name = GetModuleFileNameA(0, exe_file_name,
                                                 sizeof(exe_file_name));
    char *one_past_last_slash = exe_file_name;
    for(char *scan = exe_file_name;
        *scan;
        ++scan)
    {
        if(*scan == '\\')
        {
            one_past_last_slash = scan + 1;
        }
    }

    char src_game_code_dll_filename[] = "game.dll";
    char src_game_code_dll_full_path[MAX_PATH];
    cat_strings(one_past_last_slash - exe_file_name,
                exe_file_name,
                sizeof(src_game_code_dll_filename) - 1,
                src_game_code_dll_filename,
                sizeof(src_game_code_dll_full_path) - 1,
                src_game_code_dll_full_path);

    char tmp_game_code_dll_filename[] = "game_tmp.dll";
    char tmp_game_code_dll_full_path[MAX_PATH];
    cat_strings(one_past_last_slash - exe_file_name,
                exe_file_name,
                sizeof(tmp_game_code_dll_filename) - 1,
                tmp_game_code_dll_filename,
                sizeof(tmp_game_code_dll_full_path) - 1,
                tmp_game_code_dll_full_path);

    win32_resize_DIB_section(&global_back_buffer, 1280, 720);

    WNDCLASS window_class = {};
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = GetModuleHandle(0);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    //window_class.hIcon = ;
    window_class.lpszClassName = "windows_window";

    if(RegisterClassA(&window_class))
    {
        HWND window = CreateWindowExA(0,
                                      window_class.lpszClassName,
                                      "Windows window",
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      GetModuleHandle(0),
                                      0);

        if(window)
        {
            win32_init_opengl(window);

            /* CHECK THIS
            glViewport(0, 0, screen_width, screen_height);

            glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  
            glfwSetCursorPosCallback(window, mouse_callback);  
            glfwSetScrollCallback(window, scroll_callback); 

            glfwSwapInterval(1); // V sync (0 = off, 1 = on)
            */

            win32_running = 1;

            // TODO(Fermin): We'll need some sort of asset streaming
            glGenTextures(1, &texture_handle);

            // NOTE(Fermin) | Start | Textures
            u32 floor_texture_id     = generate_texture("src\\misc\\assets\\textures\\floor.texture",     GL_RGBA);
            u32 wall_texture_id      = generate_texture("src\\misc\\assets\\textures\\wall.texture",      GL_RGBA);
            u32 roof_texture_id      = generate_texture("src\\misc\\assets\\textures\\roof.texture",      GL_RGBA);
            u32 highlight_texture_id = generate_texture("src\\misc\\assets\\textures\\highlight.texture", GL_RGBA);
            dude_texture_id          = generate_texture("src\\misc\\assets\\textures\\dude.texture",      GL_RGBA);
            // NOTE(Fermin) | End | Texture

            // NOTE(Fermin): Test fonts start
#if 0
            char *font_vertex_code = R"FOO(
                #version 330 core

                layout (location = 0) in vec2 aPos;
                layout (location = 1) in vec2 aTexCoord;

                out vec2 TexCoord;

                uniform mat4 projection;

                void main()
                {
                    gl_Position = projection * vec4(aPos.x, aPos.y , 0.0, 1.0);
                    TexCoord = aTexCoord;
                }
            )FOO";

            char *font_fragment_code = R"FOO(
                #version 330 core

                in vec2 TexCoord;

                out vec4 FragColor;

                uniform sampler2D sampler;
                uniform vec3 textColor;

                void main()
                {
                    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(sampler, TexCoord).r);
                    FragColor = vec4(textColor, 1.0) * sampled;
                }
            )FOO";

            u32 font_program_id = build_program(font_vertex_code, font_fragment_code);

            u32 font_VBO, font_VAO;
            glGenVertexArrays(1, &font_VAO);  
            glGenBuffers(1, &font_VBO); 

            glBindVertexArray(font_VAO);

            glBindBuffer(GL_ARRAY_BUFFER, font_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(f32)*4*6, NULL, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
            glEnableVertexAttribArray(1);

            // NOTE(Fermin) THis unbinds the vbo and vao
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            glUniform1i(glGetUniformLocation(font_program_id, "sampler"), 0);

            init_font(&consola, "src\\misc\\assets\\consola.font");
#endif
            // NOTE(Fermin): Test fonts end

            // NOTE(Fermin): Game things start
            Game_Code game = win32_load_game_code(src_game_code_dll_full_path,
                                                  tmp_game_code_dll_full_path);

#if 0
            char *draw_rectangle_vertex_code = R"FOO(
                #version 330 core

                layout (location = 0) in vec3 pos;
                layout (location = 1) in vec2 in_tex_coord;

                out vec2 out_tex_coord;
                out vec3 out_frag_pos;

                uniform mat4 model;
                uniform mat4 view;
                uniform mat4 projection;

                void main()
                {
                   gl_Position = projection * view * model * vec4(pos, 1.0);
                   out_frag_pos = vec3(model * vec4(pos, 1.0));
                   out_tex_coord = in_tex_coord;
                }
            )FOO";

            char *draw_rectangle_fragment_code = R"FOO(
                #version 330 core

                in vec2 out_tex_coord;
                in vec3 out_frag_pos;

                out vec4 FragColor;

                uniform sampler2D sampler;
                uniform vec3 light_pos;
                uniform vec4 color_trans;

                void main()
                {
                   float light_strength = 5.0;
                   vec3 light_color = vec3(1.0f, 1.0f, 0.7f);
                   vec3 norm = vec3(0.0f, 0.0f, -1.0f); // TODO(Fermin): Normal maps?
                   vec3 light_dir = normalize(light_pos - out_frag_pos);  
                   float diff = max(dot(norm, light_dir), 0.0);
                   vec3 diffuse = light_strength * diff * light_color;

                   FragColor = color_trans * (vec4(diffuse, 1.0)) * texture(sampler, out_tex_coord);

                   //FragColor = texture(sampler, out_tex_coord);
                }
            )FOO";
            
            Program draw_rect_prog = {};
            draw_rect_prog.id = build_program(draw_rectangle_vertex_code,
                                              draw_rectangle_fragment_code);

            draw_rect_prog.model       = glGetUniformLocation(draw_rect_prog.id, "model");
            draw_rect_prog.view        = glGetUniformLocation(draw_rect_prog.id, "view");
            draw_rect_prog.proj        = glGetUniformLocation(draw_rect_prog.id, "projection");
            draw_rect_prog.color_trans = glGetUniformLocation(draw_rect_prog.id, "color_trans");
            draw_rect_prog.light_pos   = glGetUniformLocation(draw_rect_prog.id, "light_pos");

            float rectangle_vertices[] = {
                 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,  // top right
                 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
                -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom left
                -0.5f,  0.5f, 0.0f, 0.0f, 1.0f   // top left 
            };
            unsigned int rectangle_indices[] = {  // note that we start from 0!
                0, 1, 3,  // first Triangle
                1, 2, 3   // second Triangle
            };

            unsigned int draw_rectangle_VBO, draw_rectangle_EBO;
            glGenVertexArrays(1, &draw_rect_prog.vao);
            glGenBuffers(1, &draw_rectangle_VBO);
            glGenBuffers(1, &draw_rectangle_EBO);
            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
            glBindVertexArray(draw_rect_prog.vao);

            glBindBuffer(GL_ARRAY_BUFFER, draw_rectangle_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), rectangle_vertices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_rectangle_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indices), rectangle_indices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)0);
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)(3 * sizeof(f32)));
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0); 
            glBindVertexArray(0); 
#endif

            Game_State game_state = {};
            game_state.entropy.index = 666;
            //game_state.camera.pos = {3.0f, -1.0f, 9.0f};
            game_state.camera.pos   = {10.0f, 10.0f, 10.0f};
            game_state.camera.up    = { 0.0f,  1.0f,  0.0f};
            game_state.camera.front = { 0.0f,  0.0f, -1.0f};
            game_state.tile_size_in_meters = 1.0f;
            game_state.level_rows = 8 * 8;
            game_state.level_cols = 8 * 8;
            game_state.floor_texture_id     = floor_texture_id;
            game_state.wall_texture_id      = wall_texture_id;
            game_state.roof_texture_id      = roof_texture_id;
            game_state.highlight_texture_id = highlight_texture_id;

            M4 view = look_at(game_state.camera.pos,
                              game_state.camera.pos + game_state.camera.front,
                              game_state.camera.up);

            game_state.proj = &projection;
            game_state.view = &view;
            
            set_flag(&game_state, game_state_flag_prints);

            Rect dude = {};
            dude.min_p = V3{0.0f, 0.0f, 0.0f};
            dude.max_p = V3{
                game_state.tile_size_in_meters,
                game_state.tile_size_in_meters,
                0.0f
            };
            dude.texture_id = dude_texture_id;

            // NOTE(Fermin): Partition this into temporal(per frame) and persisten segments instead of using 'cached'
            Render_Buffer tiles_buffer = {};
            tiles_buffer.buffer = allocate_buffer(gigabytes(1));
            u32 rect_cap = gigabytes(1)/sizeof(Rect);
            // NOTE(Fermin): Game things end

            f32 delta_time = 0.0f;
            f32 last_frame = 0.0f;

            // NOTE(Fermin): This is the main loop
            while(win32_running)
            {
                assert(tiles_buffer.count == tiles_buffer.cached);

                FILETIME new_dll_write_time = win32_get_last_write_time(src_game_code_dll_full_path);
                if(CompareFileTime(&new_dll_write_time, &game.dll_last_write_time) != 0)
                {
                    win32_unload_game_code(&game);
                    game = win32_load_game_code(src_game_code_dll_full_path,
                                                tmp_game_code_dll_full_path);

                    // NOTE(Fermin): DEBUG We regenerate maze when reloading for easy testing
                    game_state.initialized = 0;
                    tiles_buffer.count = 0;
                    tiles_buffer.cached = 0;
                }

                // TODO(Fermin): Handle input with x input here and move to 
                // a function for ease of reading
                MSG message;
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                    {
                        win32_running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

#if 0
                f32 current_frame = glfwGetTime();
                delta_time = current_frame - last_frame;
                last_frame = current_frame;
                game_state.delta = delta_time;

                debug_print_line = 0.0f;

                // NOTE(Fermin): Hide and capture cursor
                if(!is_set(&game_state, game_state_flag_free_cam_mode))
                {
                    mouse_enabled = 1;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);  
                }
                else
                {
                    mouse_enabled = 0;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  

                    // TODO(Fermin): Feel like this should go in the game.
                    V3 camera_direction = {};
                    camera_direction.x = cos(radians(camera_yaw)) * cos(radians(camera_pitch));
                    camera_direction.y = sin(radians(camera_pitch));
                    camera_direction.z = sin(radians(camera_yaw)) * cos(radians(camera_pitch));
                    game_state.camera.front = normalize(camera_direction);
                }

                game_state.input_state.cursor.x = last_mouse_x/screen_width + (last_mouse_x - screen_width)/screen_width;
                game_state.input_state.cursor.y = -1.0 * (last_mouse_y/screen_height + (last_mouse_y - screen_height)/screen_height);

                // TODO(Fermin): No need for ifs, just assign check directly to key state?
                if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    game_state.input_state.w = 1;
                if(glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE)
                    game_state.input_state.w = 0;

                if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    game_state.input_state.s = 1;
                if(glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)
                    game_state.input_state.s = 0;

                if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    game_state.input_state.a = 1;
                if(glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE)
                    game_state.input_state.a = 0;

                if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    game_state.input_state.d = 1;
                if(glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)
                    game_state.input_state.d = 0;

                if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                    game_state.input_state.left_mouse = 1;
                if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
                    game_state.input_state.left_mouse = 0;

                if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
                    game_state.input_state.f1 = 1;
                if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
                    game_state.input_state.f1 = 0;

                if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
                    game_state.input_state.f2 = 1;
                if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE)
                    game_state.input_state.f2 = 0;

                if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
                    game_state.input_state.f3 = 1;
                if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE)
                    game_state.input_state.f3 = 0;

                if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                    glfwSetWindowShouldClose(window, true);

                // TODO(Fermin): Where is it convinient to update this matrix?
                // We need to see how ofter the camera state changes
                view = look_at(game_state.camera.pos,
                               game_state.camera.pos + game_state.camera.front,
                               game_state.camera.up);

                game.update_and_render(&tiles_buffer, &dude, &game_state);

                //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                // TODO(Fermin): Consider the parameters to this function and see if the current structs make sense
                V3 light_pos = {dude.min_p.x + game_state.tile_size_in_meters/2.0f,
                                dude.min_p.y + game_state.tile_size_in_meters/2.0f,
                                -0.5f};
                // TODO(Fermin): Instead of draw_rectangles "draw_buffer_to_output()" makes more sense. We would take the render
                // buffer with the rects AND the text to render and switch on the type of render object.
                draw_rectangles(&draw_rect_prog, &tiles_buffer, &view, &projection, game_state.tile_size_in_meters, light_pos);

                // NOTE(Fermin): START font render state
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_CULL_FACE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

                glUseProgram(font_program_id);

                // NOTE(Fermin): We can store this matrix outside the loop and recalculate when screen dimentions change
                M4 font_projection = orthogonal(0.0f, screen_width, 0.0f, screen_height, -1.0f, 1000.0f);
                u32 font_projection_loc = glGetUniformLocation(font_program_id, "projection");
                glUniformMatrix4fv(font_projection_loc, 1, GL_FALSE, font_projection.e);
                glUniform3f(glGetUniformLocation(font_program_id, "textColor"), 1.0f, 0.5f, 0.0f);

                glBindVertexArray(font_VAO);

                char text_buffer[256];
                _snprintf_s(text_buffer, sizeof(text_buffer), "rate: %.4fms/f %.4ff/s", delta_time*1000.0f, 1.0f/delta_time);
                print_debug_text(text_buffer, &consola, font_VBO, font_program_id);
                if(is_set(&game_state, game_state_flag_prints))
                {
                    // Stop passing all this buffer and program info. Globals for now? then manager
                    _snprintf_s(text_buffer, sizeof(text_buffer), "tiles_buffer capacity %i/%i)", tiles_buffer.count, rect_cap);
                    print_debug_text(text_buffer, &consola, font_VBO, font_program_id);

                    //f32 dude_x = dude.min_p.x + (dude.max_p.x - dude.min_p.x)/2.0f;
                    //f32 dude_y = dude.min_p.y + (dude.max_p.y - dude.min_p.y)/2.0f;
                    f32 dude_x = dude.min_p.x;
                    f32 dude_y = dude.min_p.y;
                    _snprintf_s(text_buffer, sizeof(text_buffer), "dude min (%.2f, %.2f)", dude_x, dude_y);
                    print_debug_text(text_buffer, &consola, font_VBO, font_program_id);

                    if(game_state.editing_tile)
                    {
                        // NOTE(Fermin): Rethink how get_tile should be used, these parameters seem inconvenient
                        Rect *editing;
                        if(get_tile(&tiles_buffer.buffer,
                                    game_state.level_cols,
                                    game_state.level_rows,
                                    game_state.editing_tile_x,
                                    game_state.editing_tile_y,
                                    &editing))
                        {
                            _snprintf_s(text_buffer, sizeof(text_buffer), "Editing tile:");
                            print_debug_text(text_buffer, &consola, font_VBO, font_program_id);

                            _snprintf_s(text_buffer, sizeof(text_buffer), "  x: %i, y: %i", game_state.editing_tile_x, game_state.editing_tile_y);
                            print_debug_text(text_buffer, &consola, font_VBO, font_program_id);

                            _snprintf_s(text_buffer, sizeof(text_buffer), "  texture_id: %i", editing->texture_id);
                            print_debug_text(text_buffer, &consola, font_VBO, font_program_id);
                        }
                    }
                }

                glDisable(GL_BLEND);
                glDisable(GL_CULL_FACE);
                // NOTE(Fermin): END font render state

                if(is_set(&game_state, game_state_flag_wireframe_mode))
                {
#if 0
                    // TODO(Fermin): We need a different color for the wireframe
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                    // NOTE(Fermin): sets the uniform on the CURRENTLY ACTIVE shader program.
                    //int vertexColorLocation = glGetUniformLocation(shaderProgram, "wireframeColor");
                    //glUniform4f(vertexColorLocation, 1.0f, 0.0f, 0.0f, 1.0f);

                    //glDrawArrays(GL_TRIANGLES, 0, 3);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
                }

                glBindVertexArray(0);

                glfwSwapBuffers(window);
                glfwPollEvents();    

                // TODO(Fermin): Double check this assignment makes sense here. (Theres an assert at the start of the loop)
                tiles_buffer.count = tiles_buffer.cached;
#endif

                render_gradient(global_back_buffer);

                HDC device_context = GetDC(window);
                Win32_Window_Dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(device_context,
                                               dimension.width,
                                               dimension.height,
                                               global_back_buffer);
                ReleaseDC(window, device_context);
            }
        }
        else
        {
        }
    }
    else
    {
    }

#if 0
    glDeleteVertexArrays(1, &draw_rect_prog.vao);
    glDeleteBuffers(1, &draw_rectangle_VBO);
    glDeleteBuffers(1, &draw_rectangle_EBO);

    glDeleteVertexArrays(1, &font_VAO);
    glDeleteBuffers(1, &font_VBO);

    glfwTerminate();
#endif
    return 666;
}
