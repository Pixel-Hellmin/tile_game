#include "Windows.h"
#include "windows_main.h"
#include "game.h"

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


// OpenGL
static void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height)
{
    screen_width = width;
    screen_height = height;
    aspect_ratio = ((f32)screen_width)/((f32)screen_height);
    projection = perspective(radians(fov), aspect_ratio, 1.0f, 100.0f);

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

struct Program
{
    GLuint id;
    GLuint model;
    GLuint view;
    GLuint proj;
    GLuint vao;
};
// OpenGL
static GLuint build_program(char *vertex_code, char *fragment_code)
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

// OpenGL
static u32 generate_texture(u8 *data, i32 width, i32 height, u32 format)
{

    u32 result;
    glGenTextures(1, &result);  
    glBindTexture(GL_TEXTURE_2D, result);  

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
    }

    return result;
}

// OpenGL
static u32 generate_texture(char *path, u32 format)
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

        font->glyph_texture_ids[index] = generate_texture(character_buffer.data, width, height, GL_RED);

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

void draw_rectangles(Program *prog, Render_Buffer *render_buffer, M4 *view, M4 *projection, f32 tile_size_in_meters)
{
    // TODO(Fermin): Currently we use a dummy model of a rectangle and
    // transformations to draw the rects. Another option is to upload
    // vertices directly using a dynamic buffer object. Test both and see
    // which is faster
    glUseProgram(prog->id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glBindVertexArray(prog->vao);

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

        glUniformMatrix4fv(prog->model, 1, GL_TRUE, model.e);
        glUniformMatrix4fv(prog->view, 1, GL_TRUE, view->e);
        glUniformMatrix4fv(prog->proj, 1, GL_TRUE, projection->e);
        glBindTexture(GL_TEXTURE_2D, rect->texture_id);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glUseProgram(0);
    glDisable(GL_BLEND);
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

    // NOTE(Fermin) | End | Init window and opengl

    // NOTE(Fermin) | Start | Textures
    u32 floor_texture_id = generate_texture("src\\misc\\assets\\textures\\floor.texture", GL_RGBA);
    u32 wall_texture_id = generate_texture("src\\misc\\assets\\textures\\wall.texture", GL_RGBA);
    u32 roof_texture_id = generate_texture("src\\misc\\assets\\textures\\roof.texture", GL_RGBA);
    u32 highlight_texture_id = generate_texture("src\\misc\\assets\\textures\\highlight.texture", GL_RGBA);
    u32 dude_texture_id = generate_texture("src\\misc\\assets\\textures\\dude.texture", GL_RGBA);
    // NOTE(Fermin) | End | Texture

    // NOTE(Fermin): Test fonts start
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
    // NOTE(Fermin): Test fonts end

    // NOTE(Fermin): Game things start
    Game_Code game = load_game_code(src_game_code_dll_full_path,
                                    tmp_game_code_dll_full_path);

    char *draw_rectangle_vertex_code = R"FOO(
        #version 330 core

        layout (location = 0) in vec3 pos;
        layout (location = 1) in vec2 in_tex_coord;

        out vec2 out_tex_coord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
           gl_Position = projection * view * model * vec4(pos, 1.0);
           out_tex_coord = in_tex_coord;
        }
    )FOO";

    char *draw_rectangle_fragment_code = R"FOO(
        #version 330 core

        in vec2 out_tex_coord;

        out vec4 FragColor;

        uniform sampler2D sampler;

        void main()
        {
           FragColor = texture(sampler, out_tex_coord);
        }
    )FOO";
    
    Program draw_rect_prog = {};
    draw_rect_prog.id = build_program(draw_rectangle_vertex_code,
                                      draw_rectangle_fragment_code);

    draw_rect_prog.model = glGetUniformLocation(draw_rect_prog.id, "model");
    draw_rect_prog.view = glGetUniformLocation(draw_rect_prog.id, "view");
    draw_rect_prog.proj = glGetUniformLocation(draw_rect_prog.id, "projection");

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
    // NOTE(Fermin): Game things end


    // NOTE(Fermin): Game stuff start
    Game_State game_state = {};
    //game_state.camera.pos = {3.0f, -1.0f, 9.0f};
    game_state.camera.pos = {10.0f, 10.0f, 25.0f};
    game_state.camera.up = {0.0f, 1.0f, 0.0f};
    game_state.camera.front = {0.0f, 0.0f, -1.0f};
    game_state.tile_size_in_meters = 1.0f;
    game_state.level_rows = 41;
    game_state.level_cols = 41;
    game_state.floor_texture_id = floor_texture_id;
    game_state.wall_texture_id = wall_texture_id;
    game_state.roof_texture_id = roof_texture_id;
    game_state.highlight_texture_id = highlight_texture_id;
    set_flag(&game_state, game_state_flag_prints);

    Rect dude = {};
    dude.min_p = V3{0.0f, 0.0f, 0.0f};
    dude.max_p = V3{
        game_state.tile_size_in_meters,
        game_state.tile_size_in_meters,
        0.0f
    };
    dude.texture_id = dude_texture_id;

    M4 view = look_at(game_state.camera.pos, game_state.camera.pos + game_state.camera.front, game_state.camera.up);

    game_state.proj = &projection;
    game_state.view = &view;
    
    Render_Buffer tiles_buffer = {};
    tiles_buffer.buffer = allocate_buffer(gigabytes(1));
    u32 rect_cap = gigabytes(1)/sizeof(Rect);
    // NOTE(Fermin): Game stuff end

    f32 delta_time = 0.0f;
    f32 last_frame = 0.0f;

    // NOTE(Fermin): Main Loop
    while(!glfwWindowShouldClose(window))
    {
        assert(tiles_buffer.count == tiles_buffer.cached);

        FILETIME new_dll_write_time = get_last_write_time(src_game_code_dll_full_path);
        if(CompareFileTime(&new_dll_write_time, &game.dll_last_write_time) != 0)
        {
            unload_game_code(&game);
            game = load_game_code(src_game_code_dll_full_path,
                                  tmp_game_code_dll_full_path);

            // NOTE(Fermin): We regenerate maze when reloading for easy testing
            game_state.initialized = 0;
            tiles_buffer.count = 0;
            tiles_buffer.cached = 0;
        }

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

            // TODO(Fermin): Feel like this should go in the game
            V3 camera_direction = {};
            camera_direction.x = cos(radians(camera_yaw)) * cos(radians(camera_pitch));
            camera_direction.y = sin(radians(camera_pitch));
            camera_direction.z = sin(radians(camera_yaw)) * cos(radians(camera_pitch));
            game_state.camera.front = normalize(camera_direction);
        }

        game_state.input_state.cursor.x = last_mouse_x/screen_width + (last_mouse_x - screen_width)/screen_width;
        game_state.input_state.cursor.y = -1.0 * (last_mouse_y/screen_height + (last_mouse_y - screen_height)/screen_height);

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
        view = look_at(game_state.camera.pos, game_state.camera.pos + game_state.camera.front, game_state.camera.up);

        game.update_and_render(&tiles_buffer, &dude, &game_state);

        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        draw_rectangles(&draw_rect_prog, &tiles_buffer, &view, &projection, game_state.tile_size_in_meters);

        // NOTE(Fermin): START font render state
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

        glUseProgram(font_program_id);

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
                Rect *editing;
                Rect *tiles = (Rect *)tiles_buffer.buffer.data;
                if(get_tile(tiles,
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

        tiles_buffer.count = tiles_buffer.cached;
    }

    glDeleteVertexArrays(1, &draw_rect_prog.vao);
    glDeleteBuffers(1, &draw_rectangle_VBO);
    glDeleteBuffers(1, &draw_rectangle_EBO);

    glDeleteVertexArrays(1, &font_VAO);
    glDeleteBuffers(1, &font_VBO);

    glfwTerminate();

    return 666;
}
