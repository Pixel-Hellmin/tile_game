#if !defined(MAIN_H)

#include <cstdio>
#include <cmath>

// NOTE(Fermin): Include glad before glfw3
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glad.c"

// NOTE(Fermin): For reading files. In this case textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef int32_t  i32;

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  b32;

typedef float    f32;
typedef double   f64;

#define global_variable static

#define Pi32 3.14159265359f
#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)

struct glyph_metadata
{
    size_t offset;
    u32 width;
    u32 height;
};

#define MAIN_H
#endif
