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
#define GL_ARRAY_BUFFER								  0x8892
#define GL_STATIC_DRAW								  0x88E4
#define GL_ELEMENT_ARRAY_BUFFER						  0x8893

#define opengl_binding(return, name, type, ...) \
	typedef return name(__VA_ARGS__); \

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

opengl_binding(void,   Gl_Attach_Shader,              glAttachShader,            GLuint program, GLuint shader)
opengl_binding(void,   Gl_Compile_Shader,             glCompileShader,           GLuint shader)
opengl_binding(void,   Gl_Link_Program,               glLinkProgram,             GLuint program)
opengl_binding(void,   Gl_Shader_Source,              glShaderSource,            GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length)
opengl_binding(void,   Gl_Use_Program,                glUseProgram,              GLuint program)
opengl_binding(void,   Gl_Validate_Program,           glValidateProgram,         GLuint program)
opengl_binding(void,   Gl_Get_Programiv,              glGetProgramiv,            GLuint program, GLenum pname, GLint *params)
opengl_binding(void,   Gl_Get_Shader_Info_Log,        glGetShaderInfoLog,        GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
opengl_binding(void,   Gl_Get_Program_Info_Log,       glGetProgramInfoLog,       GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
opengl_binding(void,   Gl_Uniform_Matrix_4vf,         glUniformMatrix4fv,        GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
opengl_binding(void,   Gl_Uniform_1i,                 glUniform1i,               GLint location, GLint v0)
opengl_binding(void,   Gl_Tex_Image_2D_Multisample,   glTexImage2DMultisample,   GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
opengl_binding(void,   GL_Bind_Framebuffer,           glBindFramebuffer,         GLenum target, GLuint framebuffer)
opengl_binding(void,   GL_Gen_Framebuffers,           glGenFramebuffers,         GLsizei n, GLuint *ids)
opengl_binding(void,   GL_Framebuffer_Texture_2D,     glFramebufferTexture2D,    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
opengl_binding(void,   GL_Gen_Renderbuffers,          glGenRenderbuffers,        GLsizei n, GLuint *renderbuffers)
opengl_binding(void,   GL_Bind_Renderbuffer,          glBindRenderbuffer,        GLenum target, GLuint renderbuffer)
opengl_binding(void,   GL_Renderbuffer_Storage,       glRenderbufferStorage,     GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
opengl_binding(void,   GL_Framebuffer_Renderbuffer,   glFramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
opengl_binding(void,   GL_Delete_Framebuffers,        glDeleteFramebuffers,      GLsizei n, GLuint *framebuffers)
opengl_binding(void,   GL_Delete_Renderbuffers,       glDeleteRenderbuffers,     GLsizei n, GLuint *renderbuffers)
opengl_binding(void,   GL_Active_Texture,             glActiveTexture,           GLenum texture)
opengl_binding(GLuint, Gl_Create_Program,             glCreateProgram,           void)
opengl_binding(GLuint, Gl_Create_Shader,              glCreateShader,            GLenum type)
opengl_binding(GLint,  Gl_Get_Uniform_Location,       glGetUniformLocation,      GLuint program, const GLchar *name)
opengl_binding(GLenum, GL_Check_Framebuffer_Status,   glCheckFramebufferStatus,  GLenum target)
opengl_binding(void,   GL_Gen_Vertex_Arrays,          glGenVertexArrays,         GLsizei n, GLuint *arrays)
opengl_binding(void,   GL_Gen_Buffers,                glGenBuffers,			     GLsizei n, GLuint *buffers)
opengl_binding(void,   GL_Bind_Vertex_Array,          glBindVertexArray,		 GLuint array)
opengl_binding(void,   GL_Bind_Buffer,				  glBindBuffer,			     GLenum target, GLuint buffer)
opengl_binding(void,   GL_Buffer_Data,				  glBufferData,			     GLenum target, GLsizeiptr size, const void *data, GLenum usage)
opengl_binding(void,   GL_Vertex_Attrib_Pointer,      glVertexAttribPointer,	 GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
opengl_binding(void,   GL_Enable_Vertex_Attrib_Array, glEnableVertexAttribArray, GLuint index)
#undef opengl_binding

typedef HGLRC WINAPI Wgl_Create_Context_Attribs_Arb(HDC hdc, HGLRC h_share_context, const int *attrib_list);
typedef BOOL WINAPI Wgl_Swap_Interval_Ext(int interval);

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
	b32    post_processing_enabled;

	Gl_Attach_Shader              *glAttachShader;
	Gl_Compile_Shader             *glCompileShader;
	Gl_Create_Program             *glCreateProgram;
	Gl_Create_Shader              *glCreateShader;
	Gl_Link_Program               *glLinkProgram;
	Gl_Shader_Source              *glShaderSource;
	Gl_Use_Program                *glUseProgram;
	Gl_Validate_Program           *glValidateProgram;
	Gl_Get_Programiv              *glGetProgramiv;
	Gl_Get_Shader_Info_Log        *glGetShaderInfoLog;
	Gl_Get_Program_Info_Log       *glGetProgramInfoLog;
	Gl_Get_Uniform_Location       *glGetUniformLocation;
	Gl_Uniform_Matrix_4vf         *glUniformMatrix4fv;
	Gl_Uniform_1i                 *glUniform1i;
	Gl_Tex_Image_2D_Multisample   *glTexImage2DMultisample;
	GL_Bind_Framebuffer           *glBindFramebuffer;
	GL_Gen_Framebuffers           *glGenFramebuffers;
	GL_Framebuffer_Texture_2D     *glFramebufferTexture2D;
	GL_Gen_Renderbuffers          *glGenRenderbuffers;
	GL_Bind_Renderbuffer          *glBindRenderbuffer;
	GL_Renderbuffer_Storage       *glRenderbufferStorage;
	GL_Framebuffer_Renderbuffer   *glFramebufferRenderbuffer;
	GL_Check_Framebuffer_Status   *glCheckFramebufferStatus;
	GL_Delete_Framebuffers        *glDeleteFramebuffers;
	GL_Delete_Renderbuffers       *glDeleteRenderbuffers;
	GL_Active_Texture             *glActiveTexture;
	GL_Gen_Vertex_Arrays          *glGenVertexArrays;
	GL_Gen_Buffers                *glGenBuffers;
	GL_Bind_Vertex_Array          *glBindVertexArray;
	GL_Bind_Buffer				  *glBindBuffer;
	GL_Buffer_Data				  *glBufferData;
	GL_Vertex_Attrib_Pointer      *glVertexAttribPointer;
	GL_Enable_Vertex_Attrib_Array *glEnableVertexAttribArray;
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

global Opengl opengl;
global Wgl_Swap_Interval_Ext *wgl_swap_interval;

#define OPENGL_H
#endif
