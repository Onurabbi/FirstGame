#include "entity.h"
#include "character.h"
#include "world.h"

static uint32_t unique_entity_id;
static entity*  g_entity_storage;
static uint32_t g_num_entities;

void entities_init(void)
{
    unique_entity_id = 1;
    g_entity_storage = (entity*)push_size(MAX_NUM_STATIC_GEOMETRIES * sizeof(entity));
    g_num_entities = 0;
}

uint32_t get_next_unique_entity_id(void)
{
    return unique_entity_id++;
}

box3 get_entity_bounds(entity* p_entity)
{
    box3 result;

    glm::vec3 position = p_entity->m_p;
    glm::vec3 min = glm::vec3(p_entity->m_p.x - p_entity->m_collision.x, 0, p_entity->m_p.z - p_entity->m_collision.y);
    glm::vec3 max = glm::vec3(p_entity->m_p.x + p_entity->m_collision.x, p_entity->m_height, p_entity->m_p.z + p_entity->m_collision.y);
    result = { min, max };
    return result;
}

entity* put_static_geometry_in_storage(entity* p_entity)
{
    printf("Storing static geometry...\n");
    return NULL;
}

void draw_entity(entity* p_entity)
{
    for (uint32_t i = 0; i < p_entity->m_num_meshes; ++i)
    {
        mesh* p_mesh = p_entity->m_meshes + i;
        draw_mesh(p_mesh, p_entity->s);
    }
}

entity* put_entity_in_storage(entity* p_entity)
{
    entity* result = NULL;
    switch (p_entity->m_type)
    {
        case(ET_CHARACTER):
        {
            result = put_character_in_storage((character*)p_entity);
            break;
        }
        case(ET_STATIC_GEOMETRY):
        {
            result = put_static_geometry_in_storage(p_entity);
            break;
        }
        case(ET_INVALID):
        {
            printf("Raw entities can not be stored\n");
            break;
        }
        default:
            break;
    }
    return result;
}