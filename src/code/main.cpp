/*
 * TODO(Fermin):
 * - Investigate FileSystem::getPath("resources/textures/container.jpg"
 * - Asset Manager?
 * - Need a way to formalize loaded characters from fonts so the game and
 *   the asset builder are always synced
*/

#include "main.h"
#include "buffer.cpp"
#include "math.cpp"

global_variable u32 screen_width = 1280;
global_variable u32 screen_height = 720;
global_variable f32 last_mouse_x = ((f32)screen_width) / 2.0f;
global_variable f32 last_mouse_y = ((f32)screen_height) / 2.0f;
global_variable f32 camera_pitch = 0.0f;
global_variable f32 camera_yaw = -90.0f;
global_variable f32 fov = 45.0f;
global_variable u32 bytes_per_pixel = 4;
global_variable const u32 font_character_count = '~' - '!' + 1;

// OpenGL
static void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height)
{
    screen_width = width;
    screen_height = height;
    glViewport(0, 0, width, height);
}  

// OpenGL
static void mouse_callback(GLFWwindow* window, double x_pos, double y_pos)
{
    static b32 first_mouse = 1;
    if(first_mouse)
    {
        last_mouse_x = x_pos;
        last_mouse_y = y_pos;
        first_mouse = 0;
    }

    f32 x_offset = x_pos - last_mouse_x;
    f32 y_offset = last_mouse_y - y_pos; // reversed since y-coord range from bot to top
    last_mouse_x = x_pos;
    last_mouse_y = y_pos;

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

// OpenGL
static void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    fov -= y_offset;

    if(fov < 1.0f)
        fov = 1.0f;
    if(fov > 45.0f)
        fov = 45.0f;
}

// OpenGL
static u32 compile_shader(i32 type, char* source)
{
    u32 id;
    id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
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

static Buffer read_file(const char *file_name)
{
    Buffer result = {};

    FILE* file;
    long file_size = open_file(&file, file_name);

    // NOTE(Fermin): Do we need the +1 always?
    result = allocate_buffer(file_size + 1);

    if (result.data)
    {
        size_t bytes_read = fread(result.data, sizeof(char), result.count, file);
        if ((bytes_read + 1) != result.count)
        {
            fprintf(stderr, "Error: Failed to read %s\nfile_size: %zu, bytes_read: %zu\n", file_name, result.count, bytes_read);

            free_buffer(&result);
            fclose(file);

            return result;
        }

        // Null-terminate the buffer
        result.data[result.count] = '\0';
    }
    else
    {
        fprintf(stderr, "Error: Memory allocation failed while reading file %s\n", file_name);
    }

    fclose(file);

    return result;
}
// NOTE(Fermin): read_file declaration should go in main.h, fix error when moving

/* NOTE(Fermin): Print cwd
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

// OpenGL
struct Program
{
    u32 id;
    char const *vertex_shader;
    char const *fragment_shader;
};

// OpenGL
static void build_program(Program *program)
{
    Buffer vertex_shader_buffer = read_file(program->vertex_shader);
    Buffer fragment_shader_buffer = read_file(program->fragment_shader);

    u32 vertex_shader_id = compile_shader(GL_VERTEX_SHADER, (char *)vertex_shader_buffer.data);
    u32 fragment_shader_id = compile_shader(GL_FRAGMENT_SHADER, (char *)fragment_shader_buffer.data);

    free_buffer(&vertex_shader_buffer);
    free_buffer(&fragment_shader_buffer);

    u32 program_id;
    program_id = glCreateProgram();
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
    } 
    glUseProgram(program_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id); 

    program->id = program_id;
}

// OpenGL
static u32 generate_texture(u8 *data, i32 width, i32 height, u32 format)
{
    u32 result;
    glGenTextures(1, &result);  
    glBindTexture(GL_TEXTURE_2D, result);  

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return result;
}

// OpenGL
static u32 generate_texture(char *path, u32 format)
{
    u32 result;

    i32 width, height, nr_channels;
    u8 *data = stbi_load(path, &width, &height, &nr_channels, 0); 

    result = generate_texture(data, width, height, format);

    stbi_image_free(data);

    return result;
}

static void generate_font_glyphs(u32 *glyph_ids)
{
    Buffer font_data = read_file("arial.font");

    for(char character = '!';
        character <= '~';
        character++)
    {
        size_t metadata_offset = sizeof(glyph_metadata) * (character - '!');
        glyph_metadata *character_info = (glyph_metadata *)(font_data.data + metadata_offset);
        u32 character_bitmap_size = character_info->width * bytes_per_pixel * character_info->height;

        // TODO(Fermin): This is local so try to dont use buffer and 
        // just create a local var
        Buffer character_buffer = {};
        character_buffer = allocate_buffer(character_bitmap_size);

        u32 *source = (u32 *)(font_data.data + sizeof(glyph_metadata) * font_character_count + character_info->offset);
        u32 *dest = (u32 *)character_buffer.data;
        for(u32 y = 0;
            y < character_info->height;
            y++)
        {
            for(u32 x = 0;
                x < character_info->width;
                x++)
            {
                *dest++ = *source++;
            }
        }

        *glyph_ids++ = generate_texture((u8 *)character_buffer.data, character_info->width, character_info->height, GL_RGBA);

        free_buffer(&character_buffer);
    }

    free_buffer(&font_data);
}

int main()
{
    // NOTE(Fermin) | Start | Init window and opengl
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Window", NULL, NULL);
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

    glViewport(0, 0, screen_width, screen_height);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  
    glfwSetCursorPosCallback(window, mouse_callback);  
    glfwSetScrollCallback(window, scroll_callback); 

    // NOTE(Fermin): Hide and capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    // NOTE(Fermin) | End | Init window and opengl

    // NOTE(Fermin) | Start | Shaders and Program
    Program test_program = {};
    test_program.vertex_shader = "src\\code\\vertex.vert";
    test_program.fragment_shader = "src\\code\\fragment.frag";

    build_program(&test_program);
    // NOTE(Fermin) | End | Shaders and Program

    // NOTE(Fermin) | Start | VAO, VBO. EBO
    float vertices[] = {
        // positions          // colors           // texture coords
        //0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        //0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
        //-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
        //-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 

        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // NOTE(Fermin): that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // NOTE(Fermin): | Warning | do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 

    glBindVertexArray(0); 
    // NOTE(Fermin) | End | VAO, VBO. EBO

    // NOTE(Fermin) | Start | Textures
    stbi_set_flip_vertically_on_load(true);

    u32 container_id = generate_texture("src\\misc\\assets\\textures\\container.jpg", GL_RGB);
    u32 awesome_face_id = generate_texture("src\\misc\\assets\\textures\\awesomeface.png", GL_RGBA);

    // NOTE(Fermin): This tells OpenGL to which texture unit (GL_TEXTURE0, etc) each shader sampler belongs to 
    glUseProgram(test_program.id);
    glUniform1i(glGetUniformLocation(test_program.id, "texture1"), 0);
    glUniform1i(glGetUniformLocation(test_program.id, "texture2"), 1);
    // NOTE(Fermin) | End | Texture

    // NOTE(Fermin): Test fonts start
    u32 font_glyphs_textures_ids[font_character_count] = {};
    generate_font_glyphs(font_glyphs_textures_ids);
    //print_debug_text('DEBUG text PRINT);
    // NOTE(Fermin): Test fonts end

    // NOTE(Fermin): Testing maths
    // NOTE(Fermin): We're translating the scene in the reverse direction of where we want to move
    V3 cube_positions[] = {
        V3{ 0.0f,  0.0f,  0.0f}, 
        V3{ 2.0f,  5.0f, -15.0f}, 
        V3{-1.5f, -2.2f, -2.5f},  
        V3{-3.8f, -2.0f, -12.3f},  
        V3{ 2.4f, -0.4f, -3.5f},  
        V3{-1.7f,  3.0f, -7.5f},  
        V3{ 1.3f, -2.0f, -2.5f},  
        V3{ 1.5f,  2.0f, -2.5f}, 
        V3{ 1.5f,  0.2f, -1.5f}, 
        V3{-1.3f,  1.0f, -1.5f}  
    };
    // NOTE(Fermin): Testing maths

    // NOTE(Fermin): Testing camera logic, todo struct
    V3 camera_pos = {0.0f, 0.0f, 3.0f};
    V3 camera_up = {0.0f, 1.0f, 0.0f};
    V3 camera_front = {0.0f, 0.0f, -1.0f};
    // NOTE(Fermin): Testing camera logic

    b32 wireframe_mode = 0;
    // NOTE(Fermin): OFC we need an actual key struct for these...
    b32 f1_key_state = 0; // NOTE(Fermin): 0 released else pressed

    f32 delta_time = 0.0f;
    f32 last_frame = 0.0f;

    // NOTE(Fermin): Main Loop
    while(!glfwWindowShouldClose(window))
    {
        f32 current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;


        f32 camera_speed = 2.5f * delta_time; // adjust accordingly

        V3 camera_direction = {};
        camera_direction.x = cos(radians(camera_yaw)) * cos(radians(camera_pitch));
        camera_direction.y = sin(radians(camera_pitch));
        camera_direction.z = sin(radians(camera_yaw)) * cos(radians(camera_pitch));
        camera_front = normalize(camera_direction);

        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera_pos += camera_speed * camera_front;

        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera_pos -= camera_speed * camera_front;

        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera_pos -= normalize(cross(camera_front, camera_up)) * camera_speed;

        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera_pos += normalize(cross(camera_front, camera_up)) * camera_speed;

        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1_key_state)
        {
            wireframe_mode = !wireframe_mode;
            f1_key_state = 1;
        }

        if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
            f1_key_state = 0;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // NOTE(Fermin): This is what we need to set to specify what and how to draw
        glUseProgram(test_program.id);

        glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
        //glBindTexture(GL_TEXTURE_2D, container_id);
        glBindTexture(GL_TEXTURE_2D, font_glyphs_textures_ids['6' - '!']);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, awesome_face_id);

        glBindVertexArray(VAO);

        glEnable(GL_DEPTH_TEST);

        f32 aspect_ratio = ((f32)screen_width)/((f32)screen_height);
        M4 projection = perspective(radians(fov), aspect_ratio, 0.1f, 100.0f);
        u32 projection_loc = glGetUniformLocation(test_program.id, "projection");
        glUniformMatrix4fv(projection_loc, 1, GL_TRUE, projection.e);

        M4 view = look_at(camera_pos, camera_pos + camera_front, camera_up);
        u32 view_loc = glGetUniformLocation(test_program.id, "view");
        // NOTE-IMPORTANT(FERMIN): WHY THIS WORKS ONLY IF TRANSPOSE IS FALSE!!!!!!!!!!!!!!!
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.e);

        u32 model_loc = glGetUniformLocation(test_program.id, "model");
        for(unsigned int i = 0; i < 10; i++)
        {
            M4 translation = translate(cube_positions[i]);
            f32 angle = 20.0f * i; 
            M4 rotation = rotate((f32)glfwGetTime() * radians(angle), {1.0f, 0.3f, 0.5f});
            M4 model = translation * rotation;
            glUniformMatrix4fv(model_loc, 1, GL_TRUE, model.e);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //glDrawArrays(GL_TRIANGLES, 0, 36);

        glDisable(GL_DEPTH_TEST);

        if(wireframe_mode)
        {
            // TODO(Fermin): We need a different color for the wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            // NOTE(Fermin): sets the uniform on the CURRENTLY ACTIVE shader program.
            //int vertexColorLocation = glGetUniformLocation(shaderProgram, "wireframeColor");
            //glUniform4f(vertexColorLocation, 1.0f, 0.0f, 0.0f, 1.0f);

            //glDrawArrays(GL_TRIANGLES, 0, 3);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();

    return 666;
}
