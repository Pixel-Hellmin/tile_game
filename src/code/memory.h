#if !defined(MEMORY_H)

struct Memory_Arena
{
	size_t size;
	size_t used;
	size_t cached; // NOTE(Fermin): Not convinced about this cached thingy
	u8* base;
    i32 tmp_count;
};

struct Tmp_Memory
{
	Memory_Arena *arena;
	size_t used;
};

static void
initialize_arena(Memory_Arena *arena, size_t size, u8 *base)
{
	arena->size = size;
	arena->base = base;
	arena->used = 0;
	arena->cached = 0;
}

inline Tmp_Memory
begin_tmp_memory(Memory_Arena *arena)
{
    Tmp_Memory result;
    
    result.arena = arena;
    result.used = arena->used;
    
    ++arena->tmp_count;
    
    return result;
}

inline void
end_tmp_memory(Tmp_Memory tmp_memory)
{
	Memory_Arena *arena = tmp_memory.arena;

	assert(arena->used >= tmp_memory.used);
	assert(arena->tmp_count > 0);

	arena->used = tmp_memory.used;
	--arena->tmp_count;
}

inline size_t
get_alignment_offset(Memory_Arena *arena, size_t alignment)
{
	size_t alignment_offset = 0;
	size_t alignment_mask = alignment - 1;

    size_t ptr_value = (size_t)(arena->base + arena->used);

	if(ptr_value & alignment_mask)
	{
		alignment_offset = alignment - (ptr_value & alignment_mask);
	}

	return alignment_offset;
}

/*
    ## is the token pasting operator in the preprocessor.
	Normally it glues two tokens together (e.g. foo##bar becomes foobar),
	but when used specifically before __VA_ARGS__ it has a special meaning
	— if __VA_ARGS__ is empty, eat the preceding comma.
*/
#define push_struct(arena, Type, ...) (Type *)push_size_(arena, sizeof(Type), ## __VA_ARGS__)
#define push_array(arena, count, Type, ...) (Type *)push_size_(arena, (count)*sizeof(Type), ## __VA_ARGS__)
static void *
push_size_(Memory_Arena *arena, size_t size, size_t alignment = 4)
{
	void *cached = arena->base + arena->cached;
	void *at = arena->base + arena->used;

	if((size_t)at < (size_t)cached)
	{
		arena->used = arena->cached;
	}

	size_t alignment_offset = get_alignment_offset(arena, 4);
	size += alignment_offset;

	assert((arena->used + size) <= arena->size);

	void *result = arena->base + arena->used + alignment_offset;
	arena->used += size;

	return result;
}

#define MEMORY_H
#endif
