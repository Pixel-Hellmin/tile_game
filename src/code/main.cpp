#include <cstdio>

// NOTE(Fermin): Include glad before glfw3
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glad.c"

typedef int32_t i32;

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  b32;

typedef float    f32;
typedef double   f64;


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}  

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\0";

const char *fragmentShaderSource2 = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);\n"
    "}\0";


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
    u32 vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    i32 v_success;
    char v_infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &v_success);
    if(!v_success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, v_infoLog);
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", v_infoLog);
    }

    u32 fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    i32 f_success;
    char f_infoLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &f_success);
    if(!f_success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, f_infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", f_infoLog);
    }

    u32 fragmentShader2;
    fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader2, 1, &fragmentShaderSource2, NULL);
    glCompileShader(fragmentShader2);
    i32 f_success2;
    char f_infoLog2[512];
    glGetShaderiv(fragmentShader2, GL_COMPILE_STATUS, &f_success2);
    if(!f_success2)
    {
        glGetShaderInfoLog(fragmentShader2, 512, NULL, f_infoLog2);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT2::COMPILATION_FAILED\n%s", f_infoLog2);
    }

    u32 shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    i32 p_success;
    char p_infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &p_success);
    if(!p_success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, p_infoLog);
        fprintf(stderr, "ERROR::SHADER::LINKING_FAILED\n%s", p_infoLog);
    } 
    glUseProgram(shaderProgram);

    u32 shaderProgram2;
    shaderProgram2 = glCreateProgram();
    glAttachShader(shaderProgram2, vertexShader);
    glAttachShader(shaderProgram2, fragmentShader2);
    glLinkProgram(shaderProgram2);
    i32 p_success2;
    char p_infoLog2[512];
    glGetProgramiv(shaderProgram2, GL_LINK_STATUS, &p_success2);
    if(!p_success2)
    {
        glGetProgramInfoLog(shaderProgram2, 512, NULL, p_infoLog2);
        fprintf(stderr, "ERROR::SHADER::LINKING_FAILED\n%s", p_infoLog2);
    } 
    glUseProgram(shaderProgram2);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader); 
    glDeleteShader(fragmentShader2); 
    // NOTE(Fermin) | End | Shaders and Program

    // NOTE(Fermin) | Start | VAO, VBO. EBO
    float vertices[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left 
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    // NOTE(Fermin): that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // NOTE(Fermin): | Warning | do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 

    glBindVertexArray(0); 
    // NOTE(Fermin) | End | VAO, VBO. EBO

    while(!glfwWindowShouldClose(window))
    {
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // NOTE(Fermin): This is what we need to set to specify what and how to draw
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        if(true) //NOTE(Fermin): Check for wireframe mode
        {
            glUseProgram(shaderProgram2);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glfwTerminate();

    return 666;
}
