/*
* TODO(Fermin): open_file and read_file exist in here and in main.cpp. FIX
*/

#include "windows_main.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


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

static void init_fonts()
{
    Buffer tff_file = read_file("C:/windows/fonts/consola.ttf");

    stbtt_fontinfo font;
    stbtt_InitFont(&font, tff_file.data, stbtt_GetFontOffsetForIndex(tff_file.data, 0));

    FILE *out = fopen("src\\misc\\assets\\consola.font", "wb");

    size_t offset = 0;
    i32 bytes_per_pixel = 1;
    i32 x_offset, y_offset, width, height;

    Buffer glyph_bitmaps = {};
    glyph_bitmaps = allocate_buffer(gigabytes(1));

    for(u32 character = font_first_character;
        character <= font_last_character;
        ++character)
    {
        f32 scale = stbtt_ScaleForPixelHeight(&font, font_point_size);
        u8 *mono_bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, character, &width, &height, &x_offset, &y_offset);


        int y1, advance, lsb;
        stbtt_GetCodepointBitmapBox(&font, character, scale, scale, 0, 0, 0, &y1);
        stbtt_GetCodepointHMetrics(&font, character, &advance, &lsb);

        Glyph_Metadata glyph_data = {};
        glyph_data.offset = offset;
        glyph_data.width = width;
        glyph_data.height = height;
        glyph_data.y_offset = y1;
        glyph_data.advance = (advance - lsb) * scale;
        fwrite(&glyph_data, sizeof(Glyph_Metadata), 1, out);

        i32 pitch = bytes_per_pixel * width;
        u8 *source = mono_bitmap;
        u8 *dest_row = glyph_bitmaps.data + offset + (height - 1) * pitch;
        for(i32 y = 0; y < height; y++)
        {
            u8 *dest = dest_row;
            for(i32 x = 0; x < width; x++)
            {
                *dest++ = *source++;

                offset+= bytes_per_pixel;
            }
            dest_row -= pitch;
        }

        stbtt_FreeBitmap(mono_bitmap, 0);
    }

    // NOTE(Fermin): In the last iteration offset = total size of bitmaps
    fwrite(glyph_bitmaps.data, offset, 1, out);

    free_buffer(&tff_file);
    free_buffer(&glyph_bitmaps);

    fclose(out);
}

static void generate_texture(char *raw_path, char *out_path)
{
    FILE *out = fopen(out_path, "wb");

    // NOTE(Fermin): Output expects RGBA
    i32 width, height, nr_channels;
    u8 *data = stbi_load(raw_path, &width, &height, &nr_channels, 0); 

    // NOTE(Fermin): File format:
    // width(i32)height(i32)nr_channers(i32)data
    fwrite(&width, sizeof(width), 1, out);
    fwrite(&height, sizeof(height), 1, out);
    fwrite(&nr_channels, sizeof(nr_channels), 1, out);
    fwrite(data, width*height*nr_channels, 1, out);

    stbi_image_free(data);
    fclose(out);
}

int main()
{
    stbi_set_flip_vertically_on_load(true);

    init_fonts();

    generate_texture("src\\misc\\assets\\textures\\raw\\awesomeface.png",
                     "src\\misc\\assets\\textures\\awesomeface.texture");

    return 666;
}
