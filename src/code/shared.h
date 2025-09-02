#if !defined(SHARED_H)

static long
open_file(FILE **handle, char const *name)
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

static Buffer
read_file(const char *file_name)
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

inline b32
strings_are_equal(umm a_length, char *a, char *b)
{
    char *at = b;
    for(umm index = 0;
        index < a_length;
        ++index, ++at)
    {
        if((*at == 0) ||
           (a[index] != *at))
        {
            return false;
        }
    }

    b32 result = (*at == 0);
    return result;
}

inline b32
is_end_of_line(char c)
{
    b32 result = ((c == '\n') ||
                  (c == '\r'));

    return result;
}

inline b32
is_whitespace(char c)
{
    b32 result = ((c == ' ') ||
                  (c == '\t') ||
                  (c == '\v') ||
                  (c == '\f') ||
                  is_end_of_line(c));

    return result;
}

static void
cat_strings(size_t source_a_count, char *source_a,
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

#define SHARED_H
#endif

