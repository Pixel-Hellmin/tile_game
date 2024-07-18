#include "windows_main.h"
#include "math.cpp"
#include "game.h"
#include "buffer.cpp"
#include "Windows.h"

global_variable u32 screen_width = 1280;
global_variable u32 screen_height = 720;
global_variable f32 last_mouse_x = ((f32)screen_width) / 2.0f;
global_variable f32 last_mouse_y = ((f32)screen_height) / 2.0f;
global_variable f32 camera_pitch = 0.0f;
global_variable f32 camera_yaw = -90.0f;
global_variable f32 fov = 45.0f;
global_variable u32 bytes_per_pixel = 4;
global_variable Font consola = {};


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
static u32 generate_texture(u8 *data, i32 width, i32 height, u32 format, u32 internal = GL_RGB)
{

    u32 result;
    glGenTextures(1, &result);  
    glBindTexture(GL_TEXTURE_2D, result);  

    // set the texture wrapping/filtering options (on the currently bound texture object)
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
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

static void get_character_metadata(char character, Glyph_Metadata *out)
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

static void init_font(Font *font, char *source)
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

        font->glyph_texture_ids[index] = generate_texture(character_buffer.data, width, height, GL_RED, GL_RED);

        free_buffer(&character_buffer);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    free_buffer(&data);
}
static void print_debug_text(char *string, Font *font, u32 VBO, u32 program_id)
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

struct Game_Code
{
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;
    Game_Update_And_Render *update_and_render;

    b32 is_valid;
};
inline FILETIME get_last_write_time(char *file_name)
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
static Game_Code load_game_code(char *src_dll_name, char *tmp_dll_name)
{
    Game_Code result = {};

    result.dll_last_write_time = get_last_write_time(src_dll_name);

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
static void unload_game_code(Game_Code *game_code)
{
    if(game_code->game_code_dll)
    {
        FreeLibrary(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }

    game_code->is_valid = false;
    game_code->update_and_render = game_update_and_render_stub;
}

static void cat_strings(size_t source_a_count, char *source_a,
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

void draw_rectangle(Program *program, u32 VAO, Rect *rect,
                    M4 *view, M4 *projection)
{
    // NOTE(Fermin): Can we pass a function pointer of the render functions
    // to the game and render from there?
    glUseProgram(program->id);

    // NOTE(Fermin): I decided that 1 unit in model space is equial to:
    f32 px_to_model_space = 100.0f;
    f32 width_scale = (rect->max_p.x - rect->min_p.x)/px_to_model_space;
    f32 height_scale = (rect->max_p.y - rect->min_p.y)/px_to_model_space;
    f32 x_scale = rect->min_p.x/px_to_model_space;
    f32 y_scale = rect->min_p.y/px_to_model_space;

    M4 scale = scale_m4(V3{width_scale, height_scale, 1.0f});
    M4 translation = translate(V3{x_scale, y_scale, 0.0});
    M4 model = translation * scale;

    u32 model_loc = glGetUniformLocation(program->id, "model");
    glUniformMatrix4fv(model_loc, 1, GL_TRUE, model.e);

    u32 view_loc = glGetUniformLocation(program->id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view->e);

    u32 proj_loc = glGetUniformLocation(program->id, "projection");
    glUniformMatrix4fv(proj_loc, 1, GL_TRUE, projection->e);

    u32 color_loc = glGetUniformLocation(program->id, "in_color");
    glUniform3f(color_loc, rect->color.r, rect->color.g, rect->color.b);

    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int main()
{

    // NOTE(Fermin): Never use MAX_PATH in code that is user-facing since that is not the max size anymore
    char exe_file_name[MAX_PATH];
    DWORD size_of_file_name = GetModuleFileNameA(0, exe_file_name, sizeof(exe_file_name));
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
    cat_strings(one_past_last_slash - exe_file_name, exe_file_name,
                sizeof(src_game_code_dll_filename) - 1, src_game_code_dll_filename,
                sizeof(src_game_code_dll_full_path) - 1, src_game_code_dll_full_path);

    char tmp_game_code_dll_filename[] = "game_tmp.dll";
    char tmp_game_code_dll_full_path[MAX_PATH];
    cat_strings(one_past_last_slash - exe_file_name, exe_file_name,
                sizeof(tmp_game_code_dll_filename) - 1, tmp_game_code_dll_filename,
                sizeof(tmp_game_code_dll_full_path) - 1, tmp_game_code_dll_full_path);

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

    glfwSwapInterval(1); // V sync (0 = off, 1 = on)

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
    
    // NOTE(Fermin): This tells OpenGL to which texture unit (GL_TEXTURE0, etc) each shader sampler belongs to 
    glUniform1i(glGetUniformLocation(test_program.id, "texture1"), 0);
    glUniform1i(glGetUniformLocation(test_program.id, "texture2"), 1);
    // NOTE(Fermin) | End | VAO, VBO. EBO

    // NOTE(Fermin) | Start | Textures
    stbi_set_flip_vertically_on_load(true);

    u32 container_id = generate_texture("src\\misc\\assets\\textures\\container.jpg", GL_RGB);
    u32 awesome_face_id = generate_texture("src\\misc\\assets\\textures\\awesomeface.png", GL_RGBA);

    // NOTE(Fermin) | End | Texture

    // NOTE(Fermin): Test fonts start
    Program font_program = {};
    font_program.vertex_shader = "src\\code\\font.vert";
    font_program.fragment_shader = "src\\code\\font.frag";

    build_program(&font_program);

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

    glUniform1i(glGetUniformLocation(font_program.id, "sampler"), 0);

    init_font(&consola, "src\\misc\\assets\\consola.font");
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
    b32 show_debug_prints = 1; // default on
    // NOTE(Fermin): OFC we need an actual key struct for these...
    b32 f1_key_state = 0; // NOTE(Fermin): 0 released else pressed
    b32 f2_key_state = 0; // NOTE(Fermin): 0 released else pressed

    f32 delta_time = 0.0f;
    f32 last_frame = 0.0f;

    // NOTE(Fermin): Game things start
    Game_Code game = load_game_code(src_game_code_dll_full_path,
                                    tmp_game_code_dll_full_path);

    Program draw_rectangle_program = {};
    draw_rectangle_program.vertex_shader = "src\\code\\draw_rectangle.vert";
    draw_rectangle_program.fragment_shader = "src\\code\\draw_rectangle.frag";
    build_program(&draw_rectangle_program);

    float rectangle_vertices[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left 
    };
    unsigned int rectangle_indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    unsigned int draw_rectangle_VBO, draw_rectangle_VAO, draw_rectangle_EBO;
    glGenVertexArrays(1, &draw_rectangle_VAO);
    glGenBuffers(1, &draw_rectangle_VBO);
    glGenBuffers(1, &draw_rectangle_EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(draw_rectangle_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, draw_rectangle_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), rectangle_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_rectangle_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indices), rectangle_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    // NOTE(Fermin): Game things end


    // NOTE(Fermin): Main Loop
    Buffer render_rectangles = {};
    render_rectangles = allocate_buffer(17*9*sizeof(Rect));
    while(!glfwWindowShouldClose(window))
    {
        FILETIME new_dll_write_time = get_last_write_time(src_game_code_dll_full_path);
        if(CompareFileTime(&new_dll_write_time, &game.dll_last_write_time) != 0)
        {
            unload_game_code(&game);
            game = load_game_code(src_game_code_dll_full_path,
                                  tmp_game_code_dll_full_path);
        }

        f32 current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        debug_print_line = 0.0f;

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
            show_debug_prints = !show_debug_prints;
            f1_key_state = 1;
        }

        if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2_key_state)
        {
            wireframe_mode = !wireframe_mode;
            f2_key_state = 1;
        }

        if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
            f1_key_state = 0;

        if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE)
            f2_key_state = 0;

        /*
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // NOTE(Fermin): START cubes render state
        glEnable(GL_DEPTH_TEST);

        glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
        glBindTexture(GL_TEXTURE_2D, container_id);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, awesome_face_id);

        glUseProgram(test_program.id);

        f32 aspect_ratio = ((f32)screen_width)/((f32)screen_height);
        M4 projection = perspective(radians(fov), aspect_ratio, 0.1f, 100.0f);
        u32 projection_loc = glGetUniformLocation(test_program.id, "projection");
        glUniformMatrix4fv(projection_loc, 1, GL_TRUE, projection.e);

        M4 view = look_at(camera_pos, camera_pos + camera_front, camera_up);
        u32 view_loc = glGetUniformLocation(test_program.id, "view");
        // NOTE-IMPORTANT(FERMIN): WHY THIS WORKS ONLY IF TRANSPOSE IS FALSE!!!!!!!!!!!!!!!
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.e);

        u32 model_loc = glGetUniformLocation(test_program.id, "model");
        glBindVertexArray(VAO);
        for(unsigned int i = 0; i < 10; i++)
        {
            M4 translation = translate(cube_positions[i]);
            f32 angle = 20.0f * i; 
            M4 rotation = rotate((f32)glfwGetTime() * radians(angle), {1.0f, 0.3f, 0.5f});
            M4 model = translation * rotation;
            glUniformMatrix4fv(model_loc, 1, GL_TRUE, model.e);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glDisable(GL_DEPTH_TEST);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //glDrawArrays(GL_TRIANGLES, 0, 36);
        // NOTE(Fermin): END cubes render state
        */

        game.update_and_render(&render_rectangles);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // NOTE(Fermin): Compute this once, and then only when screen dimensions change
        f32 aspect_ratio = ((f32)screen_width)/((f32)screen_height);
        M4 projection = perspective(radians(fov), aspect_ratio, 0.1f, 100.0f);

        M4 view = look_at(camera_pos, camera_pos + camera_front, camera_up);

        u32 rect_count = 0;
        Rect *tiles = (Rect *)render_rectangles.data;
        for(u32 row = 0; row < 9; row++)
        {
            for(u32 col = 0; col < 17; col++)
            {
                Rect *rect = tiles + rect_count++;
                draw_rectangle(&draw_rectangle_program, draw_rectangle_VAO, rect, &view, &projection);
            }
        }

        // NOTE(Fermin): START font render state
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

        glUseProgram(font_program.id);

        M4 font_projection = orthogonal(0.0f, screen_width, 0.0f, screen_height, -1.0f, 1000.0f);
        u32 font_projection_loc = glGetUniformLocation(font_program.id, "projection");
        glUniformMatrix4fv(font_projection_loc, 1, GL_FALSE, font_projection.e);
        glUniform3f(glGetUniformLocation(font_program.id, "textColor"), 1.0f, 0.5f, 0.0f);

        glBindVertexArray(font_VAO);

        char text_buffer[256];
        _snprintf_s(text_buffer, sizeof(text_buffer), "rate: %.4fms/f %.4ff/s", delta_time*1000.0f, 1.0f/delta_time);
        print_debug_text(text_buffer, &consola, font_VBO, font_program.id);
        if(show_debug_prints)
        {
            // Stop passing all this buffer and program info. Globals for now? then manager
            print_debug_text("memory allocated: 3.420 Gbs", &consola, font_VBO, font_program.id);
            print_debug_text("Tomorrow", &consola, font_VBO, font_program.id);
        }

        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        // NOTE(Fermin): END font render state

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

    glDeleteVertexArrays(1, &font_VAO);
    glDeleteBuffers(1, &font_VBO);

    glfwTerminate();

    return 666;
}
