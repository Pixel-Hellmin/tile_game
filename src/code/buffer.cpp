#ifndef BUFFER_CPP
#define BUFFER_CPP

#include <Windows.h>

struct Buffer
{
    size_t count;
    u8 *data;
};

static Buffer allocate_buffer(size_t count)
{
    Buffer result = {};
    result.data = (u8 *)VirtualAlloc(0, count, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if(result.data)
    {
        result.count = count;
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to allocate %llu bytes.\n", count);
    }

    return result;
}

static void free_buffer(Buffer *buffer)
{
    if (buffer->data)
    {
        VirtualFree(buffer->data, 0, MEM_RELEASE);
    }
    *buffer = {};
}

inline b32 is_valid(Buffer buffer)
{
    b32 result = (buffer.data != 0);
    return result;
}

#endif
