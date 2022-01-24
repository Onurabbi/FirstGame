#ifndef MEMORY_H
#define MEMORY_H

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)


#include <stdint.h>
#include <stdlib.h>
#include <cstring>

struct memory_arena
{
    uint8_t* base;
    uint64_t used;
    uint64_t capacity;
    bool initialized;
};

void   fill_game_memory(void);
bool   game_memory_init(void);
void*  push_size(uint64_t size);
void   free_size(uint64_t size);
#endif