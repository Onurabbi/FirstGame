#include "memory.h"

static memory_arena game_memory;

bool game_memory_init(void)
{
    uint64_t memory_size = 4LL*1024LL*1024LL*1024LL;
    game_memory.base = (uint8_t*)malloc(memory_size);
    if (!game_memory.base)
    {
        return false;
    }
    game_memory.capacity = memory_size;
    game_memory.used = 0;
    memset(game_memory.base, 0, memory_size);
    return true;
}

void* push_size(uint64_t size)
{
    //round up to word aligned?
    void* result = (void*)game_memory.base;
    game_memory.base += size;
    game_memory.used += size;
    return result;
}

void  free_size(uint64_t size)
{
    game_memory.base -= size;
    game_memory.used -= size;
}

void fill_game_memory(void)
{
    if(!game_memory.initialized)
    {
        uint64_t remaining = game_memory.capacity - game_memory.used;
        memset(game_memory.base, 0, remaining);
        game_memory.initialized = true;
    }
}