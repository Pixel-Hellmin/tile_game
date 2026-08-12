#if !defined(OPENGL_H)

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

// NOTE(Fermin): Got these from https://registry.khronos.org/OpenGL/api/GL/glcorearb.h
#define GL_FRAMEBUFFER_SRGB							  0x8DB9
#define GL_FRAMEBUFFER								  0x8D40
#define GL_SRGB8_ALPHA8                   			  0x8C43
#define GL_SHADING_LANGUAGE_VERSION       			  0x8B8C
#define GL_VERTEX_SHADER                  			  0x8B31
#define GL_FRAGMENT_SHADER                			  0x8B30
#define GL_COMPILE_STATUS                 			  0x8B81
#define GL_LINK_STATUS                    			  0x8B82
#define GL_VALIDATE_STATUS                			  0x8B83
#define GL_MAX_COLOR_TEXTURE_SAMPLES      			  0x910E
#define GL_TEXTURE_2D_MULTISAMPLE         			  0x9100
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_COLOR_ATTACHMENT0						  0x8CE0
#define GL_RENDERBUFFER								  0x8D41
#define GL_DEPTH_ATTACHMENT							  0x8D00
#define GL_FRAMEBUFFER_COMPLETE						  0x8CD5
#define GL_TEXTURE0									  0x84C0

#define Opengl_Binding(return, name, type, ...) \
	typedef return name(__VA_ARGS__); \
	global_variable name *type;

typedef char GLchar;
Opengl_Binding(void,   Gl_Attach_Shader,            glAttachShader,            GLuint program, GLuint shader)
Opengl_Binding(void,   Gl_Compile_Shader,           glCompileShader,           GLuint shader)
Opengl_Binding(void,   Gl_Link_Program,             glLinkProgram,             GLuint program)
Opengl_Binding(void,   Gl_Shader_Source,            glShaderSource,            GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length)
Opengl_Binding(void,   Gl_Use_Program,              glUseProgram,              GLuint program)
Opengl_Binding(void,   Gl_Validate_Program,         glValidateProgram,         GLuint program)
Opengl_Binding(void,   Gl_Get_Programiv,            glGetProgramiv,            GLuint program, GLenum pname, GLint *params)
Opengl_Binding(void,   Gl_Get_Shader_Info_Log,      glGetShaderInfoLog,        GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
Opengl_Binding(void,   Gl_Get_Program_Info_Log,     glGetProgramInfoLog,       GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
Opengl_Binding(void,   Gl_Uniform_Matrix_4vf,       glUniformMatrix4fv,        GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
Opengl_Binding(void,   Gl_Uniform_1i,               glUniform1i,               GLint location, GLint v0)
Opengl_Binding(void,   Gl_Tex_Image_2D_Multisample, glTexImage2DMultisample,   GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
Opengl_Binding(void,   GL_Bind_Framebuffer,         glBindFramebuffer,         GLenum target, GLuint framebuffer)
Opengl_Binding(void,   GL_Gen_Framebuffers,         glGenFramebuffers,         GLsizei n, GLuint *ids)
Opengl_Binding(void,   GL_Framebuffer_Texture_2D,   glFramebufferTexture2D,    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
Opengl_Binding(void,   GL_Gen_Renderbuffers,        glGenRenderbuffers,        GLsizei n, GLuint *renderbuffers)
Opengl_Binding(void,   GL_Bind_Renderbuffer,        glBindRenderbuffer,        GLenum target, GLuint renderbuffer)
Opengl_Binding(void,   GL_Renderbuffer_Storage,     glRenderbufferStorage,     GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
Opengl_Binding(void,   GL_Framebuffer_Renderbuffer, glFramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
Opengl_Binding(void,   GL_Delete_Framebuffers,      glDeleteFramebuffers,      GLsizei n, GLuint *framebuffers)
Opengl_Binding(void,   GL_Delete_Renderbuffers,     glDeleteRenderbuffers,     GLsizei n, GLuint *renderbuffers)
Opengl_Binding(void,   GL_Active_Texture,           glActiveTexture,           GLenum texture)
Opengl_Binding(GLuint, Gl_Create_Program,           glCreateProgram,           void)
Opengl_Binding(GLuint, Gl_Create_Shader,            glCreateShader,            GLenum type)
Opengl_Binding(GLint,  Gl_Get_Uniform_Location,     glGetUniformLocation,      GLuint program, const GLchar *name)
Opengl_Binding(GLenum, GL_Check_Framebuffer_Status, glCheckFramebufferStatus,  GLenum target)
#undef Opengl_Binding

struct Opengl {
    GLuint program;
    GLuint transform_id;
    GLuint texture_sampler_id;
    GLuint default_internal_texture_format;
	GLuint fbo;
	GLuint fbo_texture;
	GLuint fbo_depth;
    GLuint filter_program;
    GLuint filter_texture_sampler_id;
    GLint  max_multisample_count;
	b32 post_processing_enabled;
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
