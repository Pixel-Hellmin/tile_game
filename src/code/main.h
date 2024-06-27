/*
 * TODO(Fermin):
 * - Investigate FileSystem::getPath("resources/textures/container.jpg"
 * - Global Managers?
 * - Investigate why are the boxes deformed when rotated?
 * - Recalculate projection matrices only when parameters change instead
 *   of every frame
 * - Fix font bearings
*/

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

global_variable f32 debug_print_line = 0.0f;
global_variable const f32 font_point_size = 64.0f;
global_variable const u32 font_first_character = '!';
global_variable const u32 font_last_character = '~';
global_variable const u32 font_character_count = font_last_character - font_first_character + 1;

struct glyph_metadata
{
    // NOTE(Fermin): advance has left side bearing calculated already
    size_t offset;
    i32 width;
    i32 height;
    i32 y_offset;
    i32 advance;
};

#define MAIN_H
#endif
