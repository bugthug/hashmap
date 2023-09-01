#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <stdalign.h>

#define ALLOC_LIMIT 68719476736 // 64GB of memory

#define ArenaPush(arena, type, count) (type*)ArenaPushImpl(arena, count, sizeof(type), alignof(type))
#define ArenaPushZero(arena, type, count) (type*)ArenaPushZeroImpl(arena, count, sizeof(type), alignof(type))

#define ARR_SIZE 100010 
#define PAGESIZE 4096
#define PushArray(arena, type, count) (type *)ArenaPush((arena), sizeof(type)*(count))
#define PushArrayZero(arena, type, count) (type *)ArenaPushZero((arena), sizeof(type)*(count))
#define PushStruct(arena, type) PushArray((arena), (type), 1)
#define PushStructZero(arena, type) PushArrayZero((arena), (type), 1)
typedef struct Arena {
	uint64_t pos;
	void* memory;
	uint64_t size;
} Arena;

Arena* ArenaCreate(uint64_t mem_size) {
	if (mem_size == 0) return NULL;	
	Arena* arena = mmap(
			NULL, 
			ALLOC_LIMIT,
			PROT_NONE,
			MAP_ANON | MAP_PRIVATE,
			-1,
			0
			);				 
	
	mprotect(
			arena, 
			mem_size, 
			PROT_READ | PROT_WRITE
	);
	arena->pos = 0;
	arena->size = ((mem_size / PAGESIZE) + (mem_size % PAGESIZE != 0)) * 4096;
	arena->memory = (arena + 1);
	return arena;
} 

// NOTE: Use of macro ArenaPush is prefered
void* ArenaPushImpl(Arena *arena, uint64_t count, size_t size, size_t alignment) {	
	arena->pos = (arena->pos + (alignment - 1)) & ~(alignment - 1);
	size *= count;	
	if (((arena->pos += size) + sizeof(Arena)) >= (arena->size )) 
	{
		mprotect(
			arena,
			arena->pos + sizeof(Arena),
			PROT_READ | PROT_WRITE 	
		);
		arena->size = ((arena->pos / PAGESIZE) + (arena->pos % PAGESIZE != 0)) * 4096;
	}
	return (void*)((uint8_t*)arena->memory + (arena->pos - size));	
}

void ArenaPop(Arena *a, uint64_t size) {
	a->pos -= size;	
}

void ArenaSetPosBack(Arena *a, uint64_t pos) {
	a->pos = pos;	
}

void ArenaClear(Arena *a) {
	a->pos = 0;
}

uint64_t ArenaGetPos(Arena *a) {
	return a->pos;	
}

Arena* ArenaRealloc(Arena *a, uint64_t newsize); 

void* ArenaPushZeroImpl(Arena *a, uint64_t count, size_t size, size_t alignment) {
	// TODO change function signature to support multiple equivilent allocations
	void *res = (void*) ArenaPushImpl(a, count, size, alignment);
	memset(res, 0, size*count);
	return res;
}

void ArenaRelease(Arena *a) {
	munmap(a, ALLOC_LIMIT);
} 

int test_arena_implementation(void) {
	// TODO implement reallocations	
	struct a {
		long long c;
		char d;
		int e;
		uint8_t f;
		uint16_t g;	
	};
	
	uint64_t *arr[ARR_SIZE];
	Arena *arena = ArenaCreate(1);
	uint8_t *arr_byte[ARR_SIZE]; 
	// pushing to arena 
	for (uint64_t i = 0; i < ARR_SIZE; i++) {
		arr[i] = ArenaPush(arena, uint64_t, 1);
		arr_byte[i] = ArenaPush(arena, uint8_t, 1);
		*arr[i] = i;
		*arr_byte[i] = i;
	}
	
	//test if pushing worked, and pop	
	for (int64_t i = (ARR_SIZE - 1); i != -1; i--) {
		assert(*arr[i] == (uint64_t) i);
		ArenaPop(arena, sizeof(*arr[i]));
		assert(*arr_byte[i] == (uint8_t) i);
		ArenaPop(arena, sizeof(*arr_byte[i]));	
	}
	
	// test push zero
	for (uint64_t i = 0; i < ARR_SIZE; i++) {
		arr[i] = ArenaPushZero(arena, uint64_t, 1);
		assert(*arr[i] == 0);	
	}
	
	ArenaRelease(arena);
	puts("ok");
	return 0;
}
