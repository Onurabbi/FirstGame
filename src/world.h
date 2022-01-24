#ifndef WORLD_H
#define WORLD_H

#include <cassert>

#include "memory.h"
#include "entity.h"

#define MAX_NUM_WORLD_CHUNKS        128
#define WORLD_CHUNK_SIZE            40.0f
#define MAX_NUM_ENTITY_PER_CHUNK    1024

struct world_chunk
{
    entity**  m_entities;
    uint32_t  m_num_entities;
    int32_t   m_chunk_x;
    int32_t   m_chunk_y;
    bool      m_is_initialized;
};

struct world
{
    world_chunk* world_chunks;
    uint32_t num_world_chunks;
};

struct sim_region
{
    world_chunk* chunks_to_simulate[4];
    uint8_t num_chunks;
};

void         world_init(void);
entity*      map_entity_to_world_chunk(entity* p_entity);
world_chunk* map_world_position_to_world_chunk(glm::vec2& world_position);

#endif