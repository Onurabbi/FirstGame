#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "mesh.h"
#include "math.h"
#include "shader.h"

#define MAX_NUM_STATIC_GEOMETRIES 1024
#define MAX_NUM_CHARACTERS        1024
#define MAX_MESHES_PER_ENTITY     4


enum entity_type
{
    ET_CHARACTER,
    ET_STATIC_GEOMETRY,
    ET_INVALID,
};

struct box3
{
    glm::vec3 min;
    glm::vec3 max;
};

struct entity
{
    mesh*       m_meshes; 
    shader      s;

    uint32_t    m_num_meshes;
    uint32_t    m_id;
    
    glm::vec2   m_dp;
    glm::vec2   m_ddp;
    glm::vec2   m_collision; //halfwidth, halfheight
    glm::vec3   m_p;
    glm::quat   m_rot;
    float       m_height;
    entity_type m_type;
};

struct      world_chunk;

void        draw_entity(entity* p_entity);
entity*     put_entity_in_storage(entity* p_entity);
box3        get_entity_bounds(entity* p_entity);
inline bool is_entity_alive(entity* p_entity) { return (p_entity->m_id != 0);}
uint32_t    get_next_unique_entity_id(void);
void        entities_init(void);

#endif