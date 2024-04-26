/*
 * TODO(Fermin):
 * Import Buffer.cpp
 * Stop malloc
 *
 * FIX why shader compilation fails sometimes
 *
*/
#include <cstdio>
#include <cmath>

// NOTE(Fermin): Include glad before glfw3
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glad.c"

typedef int32_t  i32;

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  b32;

typedef float    f32;
typedef double   f64;


static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}  

static u32 compile_shader(i32 type, char** source)
{
    u32 id;
    id = glCreateShader(type);
    glShaderSource(id, 1, source, NULL);
    glCompileShader(id);

    i32 success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::%s::COMPILATION_FAILED\n%s", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
    }

    return id;
}

static long open_file(FILE **handle, char const *name)
{
    *handle = fopen(name, "rb");
    if (!handle)
    {
        fprintf(stderr, "Error: Couldn't open file: %s\n", name);
        return -1;
    }

    fseek(*handle, 0, SEEK_END);
    long file_size = ftell(*handle);
    rewind(*handle);

    return file_size;
}

static void read_file(char **buffer, const char *source)
{
    FILE* file;
    long file_size = open_file(&file, source);

    *buffer = (char*)malloc(file_size);
    if (*buffer)
    {
        size_t bytes_read = fread(*buffer, sizeof(char), file_size, file);
        if (bytes_read != file_size)
        {
            fprintf(stderr, "Error: Failed to read %s\nfile_size: %lu, bytes_read: %zu", source, file_size, bytes_read);
            free(*buffer);
            fclose(file);
        }
        fclose(file);
    }
    else
    {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(file);
    }
}

/*
#include <stdio.h>
#define WINDOWS 
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif


char *buf;
buf=(char *)malloc(100*sizeof(char));
getcwd(buf,100);
printf("\n %s \n",buf);
*/

struct Program
{
    u32 id;
    char const* vertex_shader;
    char const* fragment_shader;
};

static void build_program(Program *program)
{
    char* vertex_shader_source;
    read_file(&vertex_shader_source, program->vertex_shader);

    char* fragment_shader_source;
    read_file(&fragment_shader_source, program->fragment_shader);

    u32 vertex_shader_id = compile_shader(GL_VERTEX_SHADER, &vertex_shader_source);
    u32 fragment_shader_id = compile_shader(GL_FRAGMENT_SHADER, &fragment_shader_source);

    free(vertex_shader_source);
    free(fragment_shader_source);

    u32 program_id;
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    i32 success;
    char infoLog[512];
    glGetProgramiv(program->id, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program_id, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::LINKING_FAILED\n%s", infoLog);
    } 
    glUseProgram(program_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id); 

    program->id = program_id;
}

int main()
{
    // NOTE(Fermin) | Start | Init window and opengl
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to create GLFW window" );
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to initialize GLAD" );
        return -1;
    } 

    glViewport(0, 0, 800, 600);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  
    // NOTE(Fermin) | End | Init window and opengl

    // NOTE(Fermin) | Start | Shaders and Program
    Program test_program = {};
    test_program.vertex_shader = "src\\code\\vertex.vert";
    test_program.fragment_shader = "src\\code\\fragment.frag";

    build_program(&test_program);
    // NOTE(Fermin) | End | Shaders and Program

    // NOTE(Fermin) | Start | VAO, VBO. EBO
    float vertices[] = {
        // positions         // colors
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
    }; 
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };  

    // NOTE(Fermin): When we create and bind a VAO, every VBO generation and 
    // attribute property specification we create is saved on the current binded
    // VAO. When we switch between drawn objects we just bind the VAO and use the 
    // program.
    u32 VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);  
    glGenBuffers(1, &VBO); 
    glGenBuffers(1, &EBO);
    
    // NOTE(Fermin): bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    // NOTE(Fermin) | Warning | glBufferData allocates memory and stores data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);

    // NOTE(Fermin): that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // NOTE(Fermin): | Warning | do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 

    glBindVertexArray(0); 
    // NOTE(Fermin) | End | VAO, VBO. EBO

    b32 wireframe_mode = 0;
    // NOTE(Fermin): OFC we need an actual key struct for these...
    b32 f1_key_state = 0; // NOTE(Fermin): 0 released else pressed
    while(!glfwWindowShouldClose(window))
    {
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1_key_state)
            wireframe_mode = !wireframe_mode;
            f1_key_state = 1;

        if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
            f1_key_state = 0;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // NOTE(Fermin): This is what we need to set to specify what and how to draw
        glUseProgram(test_program.id);
        glBindVertexArray(VAO);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        if(wireframe_mode)
        {
            // TODO(Fermin): We need a different color for the wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            // NOTE(Fermin): sets the uniform on the CURRENTLY ACTIVE shader program.
            //int vertexColorLocation = glGetUniformLocation(shaderProgram, "wireframeColor");
            //glUniform4f(vertexColorLocation, 1.0f, 0.0f, 0.0f, 1.0f);

            glDrawArrays(GL_TRIANGLES, 0, 3);
            //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glfwTerminate();

    return 666;
}
