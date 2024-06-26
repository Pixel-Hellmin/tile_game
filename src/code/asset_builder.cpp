/*
* TODO(Fermin): open_file and read_file exist in here and in main.cpp. FIX
*/
#include "main.h"
#include "buffer.cpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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
    Buffer tff_file = read_file("C:/windows/fonts/arial.ttf");

    stbtt_fontinfo font;
    stbtt_InitFont(&font, tff_file.data, stbtt_GetFontOffsetForIndex(tff_file.data, 0));

    FILE *out = fopen("arial.font", "wb");

    size_t offset = 0;
    i32 bytes_per_pixel = 1;
    i32 x_offset, y_offset, width, height;

    Buffer glyph_bitmaps = {};
    glyph_bitmaps = allocate_buffer(gigabytes(1));

    for(u32 character = '!';
        character <= '~';
        ++character)
    {
        u8 *mono_bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 64.0f), character, &width, &height, &x_offset, &y_offset);

        glyph_metadata glyph_data = {};
        glyph_data.offset = offset;
        glyph_data.width = width;
        glyph_data.height = height;
        fwrite(&glyph_data, sizeof(glyph_metadata), 1, out);

        i32 pitch = bytes_per_pixel * width;
        u8 *source = mono_bitmap;
        u8 *dest_row = glyph_bitmaps.data + offset + (height - 1) * pitch;
        for(i32 y = 0; y < height; y++)
        {
            u8 *dest = dest_row;
            for(i32 x = 0; x < width; x++)
            {
                //u8 alpha = *source++;
                *dest++ = *source++;
                /*
                *dest++ = ((alpha << 24) |
                           (alpha << 16) |
                           (alpha <<  8) |
                           (alpha <<  0));
                */

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

int main()
{
    init_fonts();

    return 666;
}
