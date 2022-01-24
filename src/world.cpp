#include "world.h"

static world* p_world;

world_chunk* map_world_position_to_world_chunk(glm::vec2& world_position)
{
    world_chunk* result = 0;
    //hash map
    int32_t chunk_x = world_position.x / WORLD_CHUNK_SIZE;
    int32_t chunk_y = world_position.y / WORLD_CHUNK_SIZE;

    int32_t chunk_hash = (19 * chunk_x + 17 * chunk_y) % MAX_NUM_WORLD_CHUNKS;

    if(chunk_hash < 0)
    {
        chunk_hash += MAX_NUM_WORLD_CHUNKS;
    }

    assert(chunk_hash >= 0);

    result = p_world->world_chunks + chunk_hash;
    
    if(result->m_is_initialized)
    {
        if(result->m_chunk_x != chunk_x || result->m_chunk_y != chunk_y)
        {
            //resolve collision
            while(result->m_is_initialized)
            {
                chunk_hash = (chunk_hash + 1) % MAX_NUM_WORLD_CHUNKS;
                result = p_world->world_chunks + (chunk_hash);  
            }
        }
        else
        {
            //printf("chunk initialized before\n");
        }
    }
    else
    {
        //Initialize chunk
        result->m_is_initialized = true;
        result->m_chunk_x = chunk_x;
        result->m_chunk_y = chunk_y;
        result->m_entities = (entity**)push_size(MAX_NUM_ENTITY_PER_CHUNK*sizeof(entity*));

        p_world->num_world_chunks++;
        printf("Initializing chunk %d\n", chunk_hash);
    }
    return result;
}

entity* map_entity_to_world_chunk(entity* p_entity)
{
    entity* result = NULL;
    
    if(p_world->num_world_chunks == MAX_NUM_WORLD_CHUNKS)
    {
        printf("Maximum number of world chunks reached.\n");
        return result;
    }
    glm::vec2 entity_pos = { p_entity->m_p.x, p_entity->m_p.y };

    //this initializes the chunk if needed
    world_chunk* p_chunk =  map_world_position_to_world_chunk(entity_pos); 
    if (p_chunk->m_num_entities == MAX_NUM_ENTITY_PER_CHUNK)
    {
        printf("Can't create new entity, storage full\n");
        return result;
    }

    result = put_entity_in_storage(p_entity);

    if (!result)
    {
        p_chunk->m_num_entities--;
        return result;
    }
    //store the pointer in chunk
    p_chunk->m_entities[p_chunk->m_num_entities++] = result;

    return result;
}

void world_init(void)
{
    p_world = (world*)push_size(sizeof(world));
    p_world->world_chunks = (world_chunk*)push_size(MAX_NUM_WORLD_CHUNKS * sizeof(world_chunk));
    memset(p_world->world_chunks, 0, MAX_NUM_WORLD_CHUNKS * sizeof(world_chunk));
    p_world->num_world_chunks = 0;
}
