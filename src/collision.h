#ifndef COLLISION_H_
#define COLLISION_H_

#include "entity.h"
#include "world.h"
#include "math.h"

struct move_spec
{
    bool unit_max_accel_vector;
    float speed;
    float drag;
};

static bool  test_wall(float wall_x, float rel_x, float rel_y, float player_delta_x, float player_delta_y, 
                      float* t_min, float min_y, float max_y);
                      
void         move_entity(entity* p_entity, sim_region* p_region, float dt, move_spec spec);
inline float get_entity_width(entity* p_entity) { return (p_entity->m_collision.x*2);}
inline float get_entity_height(entity* p_entity) { return (p_entity->m_collision.y*2);}

#endif