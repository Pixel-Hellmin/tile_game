#if !defined(OPENGL_H)

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

typedef char   GLchar;
typedef void   Gl_Attach_Shader(GLuint program, GLuint shader);
typedef void   Gl_Compile_Shader(GLuint shader);
typedef void   Gl_Link_Program(GLuint program);
typedef void   Gl_Shader_Source(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void   Gl_Use_Program(GLuint program);
typedef void   Gl_Validate_Program(GLuint program);
typedef void   Gl_Get_Programiv(GLuint program, GLenum pname, GLint *params);
typedef void   Gl_Get_Shader_Info_Log(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   Gl_Get_Program_Info_Log(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   Gl_Uniform_Matrix_4vf(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void   Gl_Uniform_1i(GLint location, GLint v0);
typedef void   Gl_Tex_Image_2D_Multisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
typedef GLuint Gl_Create_Program(void);
typedef GLuint Gl_Create_Shader(GLenum type);
typedef GLint  Gl_Get_Uniform_Location(GLuint program, const GLchar *name);

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
 
struct Opengl {
    GLuint program;
    GLuint transform_id;
    GLuint texture_sampler_id;
    GLuint default_internal_texture_format;
    GLint max_multisample_count;
};

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

#define OPENGL_H
#endif
