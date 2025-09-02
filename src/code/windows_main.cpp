#include "windows.h"
#include "windows_main.h"
#include "shared.h"
#include "game.h"
#include <gl/gl.h>

#include "opengl.h"
global_variable Opengl opengl = {};
global_variable GLuint texture_handle; // TODO(Fermin) Check if we still need this
// NOTE(Fermin): I don't think these should be global. Think where they fit.
global_variable Render_Buffer tiles_buffer = {};
global_variable Render_Buffer ui_buffer = {};

#include "opengl.cpp"

global_variable Font consola = {};
global_variable Game_State game_state = {};
global_variable Win32_Offscreen_Buffer global_back_buffer;

typedef HGLRC WINAPI Wgl_Create_Context_Attribs_Arb(HDC hdc, HGLRC h_share_context, const int *attrib_list);
typedef BOOL WINAPI Wgl_Swap_Interval_Ext(int interval);
global_variable Wgl_Swap_Interval_Ext *wgl_swap_interval;

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
    desired_pixel_format.cDepthBits = 24;
    desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
    
    // TODO(Fermin): ChoosePixelFormatARB logic. Ep 364 min 27
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
win32_resize_DIB_section(i32 width, i32 height)
{
    if(global_back_buffer.memory)
    {
        VirtualFree(global_back_buffer.memory, 0, MEM_RELEASE);
    }

    global_back_buffer.width = width;
    global_back_buffer.height = height;
    global_back_buffer.bytes_per_pixel = bytes_per_pixel;
    global_back_buffer.pitch = width * global_back_buffer.bytes_per_pixel;
    global_back_buffer.info.bmiHeader.biSize = sizeof(global_back_buffer.info.bmiHeader);
    global_back_buffer.info.bmiHeader.biWidth = global_back_buffer.width;
    global_back_buffer.info.bmiHeader.biHeight = -global_back_buffer.height;
    global_back_buffer.info.bmiHeader.biPlanes = 1;
    global_back_buffer.info.bmiHeader.biBitCount = 32;
    global_back_buffer.info.bmiHeader.biCompression = BI_RGB;

    i32 bitmap_memory_size = (global_back_buffer.width * global_back_buffer.height) * global_back_buffer.bytes_per_pixel;
    global_back_buffer.memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

static void
win32_display_buffer_in_window(HDC device_context, i32 window_width, i32 window_height, Game_State* game_state)
{
    opengl_render(window_width, window_height, game_state);

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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_DESTROY:
        {
            win32_running = 0;
        } break;

            /*
        case WM_SETCURSOR:
        {
            result = DefWindowProcA(window, message, w_param, l_param);
        } break;
            */

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            Win32_Window_Dimension dimension = win32_get_window_dimension(window);
            win32_display_buffer_in_window(device_context, dimension.width, dimension.height, &game_state);
            EndPaint(window, &paint);
        } break;

        default:
        {
            result = DefWindowProc(window, message, w_param, l_param);
        } break;
    }

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

static void
win32_process_keyboard_message(b32 *button_state, b32 is_down)
{
    // TODO(Fermin): Transition states
    *button_state = is_down;
}

static void
win32_process_pending_messages(Game_State *game_state)
{
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                win32_running = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 vk_code = (u32)message.wParam;
                b32 alt_key_was_down = (message.lParam & (1 << 29));

                // NOTE: Since we are comparing was_down to is_down,
                // we MUST use == and != to convert these bit tests to actual
                // 0 or 1 values.
                b32 was_down = ((message.lParam & (1 << 30)) != 0);
                b32 is_down = ((message.lParam & (1 << 31)) == 0);
                if(was_down != is_down)
                {
                    if(vk_code == 'W')
                    {
                        win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == 'A')
                    {
                        win32_process_keyboard_message(&game_state->input_state.a, is_down);
                    }
                    else if(vk_code == 'S')
                    {
                        win32_process_keyboard_message(&game_state->input_state.s, is_down);
                    }
                    else if(vk_code == 'D')
                    {
                        win32_process_keyboard_message(&game_state->input_state.d, is_down);
                    }
                    else if(vk_code == 'Q')
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == 'E')
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_UP)
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_LEFT)
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_DOWN)
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_RIGHT)
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_ESCAPE)
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_SPACE)
                    {
                        //win32_process_keyboard_message(&game_state->input_state.w, is_down);
                    }
                    else if(vk_code == VK_F1)
                    {
                        win32_process_keyboard_message(&game_state->input_state.f1, is_down);
                    }
                    else if(vk_code == VK_F2)
                    {
                        win32_process_keyboard_message(&game_state->input_state.f2, is_down);
                    }
                    else if(vk_code == VK_F3)
                    {
                        win32_process_keyboard_message(&game_state->input_state.f3, is_down);
                    }
                    else if(vk_code == 'P')
                    {
                        if(is_down)
                        {
                        }
                    }
                    else if(vk_code == 'L')
                    {
                        if(is_down)
                        {
                        }
                    }
                    if(is_down)
                    {
                        if((vk_code == VK_F4) && alt_key_was_down)
                        {
                            win32_running = false;
                        }
                        if(vk_code == VK_ESCAPE)
                        {
                            win32_running = false;
                        }
                        if((vk_code == VK_RETURN) && alt_key_was_down)
                        {
                            if(message.hwnd)
                            {
                                //ToggleFullscreen(message.hwnd);
                            }
                        }
                    }
                }
            } break;

            default:
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }

    }
}

static LARGE_INTEGER
win32_get_wallclock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

static f32
win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    f32 result = ((f32)(end.QuadPart - start.QuadPart) /
                  (f32)global_perf_count_frequency);

    return result;
}

static void
init_font(Font *font, char *source)
{
    Buffer data = read_file(source);

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
        
        //u32 font_bytes_per_pixel = 1;
        u32 font_bytes_per_pixel = 4;
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
                // NOTE(Fermin): Font files store mono bitmaps, which we expand to 4 channels here
                u8 result = *bitmap_source++;
                *bitmap_dest++ = result;
                *bitmap_dest++ = result;
                *bitmap_dest++ = result;
                *bitmap_dest++ = result;
            }
        }

        font->glyph_texture_ids[index] = opengl_generate_texture(character_buffer.data, width, height, GL_RGBA);

        free_buffer(&character_buffer);
    }

    free_buffer(&data);
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
print_debug_text(char *string, Font *font)
{
    f32 print_font_size = 24.0f;
    debug_print_line -= print_font_size;

    f32 x = 10.0f;
    f32 line_y = debug_print_line;
    f32 max_scale = print_font_size / font_point_size;
    f32 space_width = font_point_size / 2.0f * max_scale;

    for(char *c = string; *c; c++)
    {
        if(*c != ' ')
        {
            Glyph_Metadata glyph_info = {};
            get_character_metadata(*c, &glyph_info);

            f32 y_scale = glyph_info.height / font_point_size;
            f32 x_scale = glyph_info.width / font_point_size;
            f32 h = print_font_size * y_scale;
            f32 w = print_font_size * x_scale;
            f32 y = line_y - glyph_info.y_offset * max_scale;
            f32 advance = glyph_info.advance * max_scale;


            Rect glyph = {};
            glyph.pos_in_screen.xy = V2{x, y};
            glyph.dim_in_px = V2{w, h};
            glyph.color = V4{0.0f, 1.0f, 0.0f, 1.0f};
            glyph.texture_id = font->glyph_texture_ids[*c - font_first_character];

            push_rectangle(&ui_buffer, &glyph, glyph.color);

            x += advance;
        }
        else
        {
            x += space_width;
        }

    }
}

int main()
{
    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    global_perf_count_frequency = perf_count_frequency_result.QuadPart;

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

    //win32_resize_DIB_section(960, 540);
    win32_resize_DIB_section(1920, 1080);

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

            i32 monitor_refresh_hz = 60;
            HDC refresh_dc = GetDC(window);
            i32 win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
            ReleaseDC(window, refresh_dc);
            if(win32_refresh_rate > 1)
            {
                monitor_refresh_hz = win32_refresh_rate;
            }
            f32 game_update_hz = (f32)(monitor_refresh_hz);

            win32_running = 1;

            // TODO(Fermin): We'll need some sort of asset streaming
            glGenTextures(1, &texture_handle);

            // NOTE(Fermin) | Start | Textures
            u32 floor_texture_id     = opengl_generate_texture("src\\misc\\assets\\textures\\floor.texture",     GL_RGBA);
            u32 wall_texture_id      = opengl_generate_texture("src\\misc\\assets\\textures\\wall.texture",      GL_RGBA);
            u32 roof_texture_id      = opengl_generate_texture("src\\misc\\assets\\textures\\roof.texture",      GL_RGBA);
            u32 highlight_texture_id = opengl_generate_texture("src\\misc\\assets\\textures\\highlight.texture", GL_RGBA);
            u32 dude_texture_id      = opengl_generate_texture("src\\misc\\assets\\textures\\dude.texture",      GL_RGBA);
            // NOTE(Fermin) | End | Texture

            init_font(&consola, "src\\misc\\assets\\consola.font");

            // NOTE(Fermin): Game things start
            Game_Code game = win32_load_game_code(src_game_code_dll_full_path,
                                                  tmp_game_code_dll_full_path);

            game_state.entropy.index = 666;
            //game_state.camera.pos = {3.0f, -1.0f, 9.0f};
            game_state.camera.pos   = {10.0f, 10.0f, 10.0f};
            game_state.camera.up    = { 0.0f,  1.0f,  0.0f};
            game_state.camera.front = { 0.0f,  0.0f, -1.0f};
            game_state.level_rows = 8 * 8;
            game_state.level_cols = 8 * 8;
            game_state.floor_texture_id     = floor_texture_id;
            game_state.wall_texture_id      = wall_texture_id;
            game_state.roof_texture_id      = roof_texture_id;
            game_state.highlight_texture_id = highlight_texture_id;
            game_state.tile_size_in_px = 64.0f;

            M4 view = look_at(game_state.camera.pos,
                              game_state.camera.pos + game_state.camera.front,
                              game_state.camera.up);

            set_flag(&game_state, game_state_flag_prints);

            Rect dude = {};
            dude.world_index = V3{0.0f, 0.0f, 0.0f};
            dude.dim_in_tiles = V2{1.0f, 1.0f};
            dude.texture_id = dude_texture_id;

            // NOTE(Fermin): Partition this into temporal(per frame) and persisten segments instead of using 'cached'
            tiles_buffer.buffer = allocate_buffer(gigabytes(1));
            u32 rect_cap = gigabytes(1)/sizeof(Rect);

            ui_buffer.buffer = allocate_buffer(gigabytes(1));
            // NOTE(Fermin): Game things end

            u32 expected_frames_per_update = 1;
            LARGE_INTEGER last_counter = win32_get_wallclock();
            f32 target_seconds_per_frame = (f32)expected_frames_per_update / (f32)game_update_hz;
            // NOTE(Fermin): This is the main loop
            while(win32_running)
            {

                game_state.dt_in_seconds = target_seconds_per_frame;

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

                Win32_Window_Dimension dimension = win32_get_window_dimension(window);
                game_state.window_width = dimension.width;
                game_state.window_height = dimension.height;
                win32_process_pending_messages(&game_state);

                POINT mouse_p;
                GetCursorPos(&mouse_p);
                ScreenToClient(window, &mouse_p);
                f32 mouse_y = (f32)(dimension.height  - mouse_p.y);
                f32 mouse_x = (f32)mouse_p.x;

                // NOTE(Fermin): Map from screen coords to normalize device coords (-1, 1)
                game_state.input_state.cursor.x = mouse_x/dimension.width + (mouse_x - dimension.width)/dimension.width;
                game_state.input_state.cursor.y = (mouse_y/dimension.height + (mouse_y - dimension.height)/dimension.height);

                win32_process_keyboard_message(&game_state.input_state.left_mouse, GetKeyState(VK_LBUTTON) & (1 << 15));
                // SetCursor(0); // To disable cursor
                
                game.update_and_render(&tiles_buffer, &dude, &game_state);

                debug_print_line = dimension.height;
                char text_buffer[256];

                _snprintf_s(text_buffer, sizeof(text_buffer), "[f]   [ms]");
                print_debug_text(text_buffer, &consola);

                _snprintf_s(text_buffer, sizeof(text_buffer), "%i  %.3f", round_f32_to_i32(1.0f/target_seconds_per_frame), target_seconds_per_frame*1000.0f);
                print_debug_text(text_buffer, &consola);

                if(is_set(&game_state, game_state_flag_prints))
                {
                    _snprintf_s(text_buffer, sizeof(text_buffer), "tiles_buffer capacity:");
                    print_debug_text(text_buffer, &consola);
                    _snprintf_s(text_buffer, sizeof(text_buffer), "   %i/%i", tiles_buffer.count, rect_cap);
                    print_debug_text(text_buffer, &consola);

                    _snprintf_s(text_buffer, sizeof(text_buffer), "dude world_index:");
                    print_debug_text(text_buffer, &consola);
                    _snprintf_s(text_buffer, sizeof(text_buffer), "   %.2f, %.2f", dude.world_index.x, dude.world_index.y);
                    print_debug_text(text_buffer, &consola);

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
                            print_debug_text(text_buffer, &consola);

                            _snprintf_s(text_buffer, sizeof(text_buffer), "   x: %i, y: %i", game_state.editing_tile_x, game_state.editing_tile_y);
                            print_debug_text(text_buffer, &consola);

                            _snprintf_s(text_buffer, sizeof(text_buffer), "   texture_id: %i", editing->texture_id);
                            print_debug_text(text_buffer, &consola);
                        }
                    }
                }


                HDC device_context = GetDC(window);
                win32_display_buffer_in_window(device_context, dimension.width, dimension.height, &game_state);
                ReleaseDC(window, device_context);
                tiles_buffer.count = tiles_buffer.cached;
                ui_buffer.count = 0;

                LARGE_INTEGER end_counter = win32_get_wallclock();
                f32 measured_seconds_for_frame = win32_get_seconds_elapsed(last_counter, end_counter);
                target_seconds_per_frame = measured_seconds_for_frame;
                last_counter = end_counter;
            }
        }
        else
        {
            // TODO
            invalid_code_path
        }
    }
    else
    {
        // TODO
        invalid_code_path
    }

    return 666;
}
