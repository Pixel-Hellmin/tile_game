#if !defined(SHARED_H)

static u16
read_u16(u8 *p)
{
	return (u16)(p[0] | (p[1] << 8));
}

static i16
read_i16(u8 *p)
{
	return (i16)read_u16(p);
}

static u32
read_u32(u8 *p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static i32
read_i32(u8 *p)
{
	return (i32)read_u32(p);
}

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
        size_t bytes_read = fread(result.data, sizeof(char), result.size, file);
        if ((bytes_read) != result.size)
        {
            fprintf(stderr, "Error: Failed to read %s\nfile_size: %zu, bytes_read: %zu\n", file_name, result.size, bytes_read);

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

inline umm
null_terminated_string_length(char *a)
{
	umm result = 0;

    while(*a++) { result++; }

	return result;
}

static inline char
to_upper_ascii(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - ('a' - 'A')) : c;
}

// @Cleanup: Can we consolidate string compares?
inline b32
strings_are_equal(char *b, char *a, umm a_length)
{
	/*
	 * a is the fixed-width field and b is the query, and a full-length match of a counts as equal even if b continues.
	*/

    char *at = b;
	umm index = 0;
    for(; index < a_length;
        ++index, ++at)
    {
		if(a[index] == 0) { break; }

        if((*at == 0) ||
           (a[index] != *at))
        {
            return false;
        }
    }

	/* NOTE: This works for null terminated strings only
	while(*a)
	{
        if((*at == 0) || (*a++ != *at++))
        {
            return false;
        }
	}
	*/

	if(index != a_length)
	{
		// broke early
		return (*at == 0);
	}
	else
	{
		return 1;
	}
}
inline b32
strings_are_equal_case_insensitive(char *b, char *a, umm a_length)
{
	/*
	 * a is the fixed-width field and b is the query, and a full-length match of a counts as equal even if b continues.
	*/

    char *at = b;
	umm index = 0;
    for(; index < a_length;
        ++index, ++at)
    {
		if(a[index] == 0) { break; }

        if((*at == 0) ||
           (to_upper_ascii(a[index]) != to_upper_ascii(*at)))
        {
            return false;
        }
    }

	if(index != a_length)
	{
		// broke early
		return (*at == 0);
	}
	else
	{
		return 1;
	}
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

static void
copy_string(char *dest, char *src, size_t count)
{
	for(int i = 0; i < count; ++i)
	{
		*dest++ = *src++;
	}
}

#define SHARED_H
#endif
