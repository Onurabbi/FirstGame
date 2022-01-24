#include "collision.h"

static bool test_wall(float wall_x, float rel_x, float rel_y, float player_delta_x, float player_delta_y, 
                      float* t_min, float min_y, float max_y)
{
    bool hit = false;

    float t_epsilon = 0.01f;
    if(player_delta_x != 0.0f)
    {
        float t_result = (wall_x - rel_x)/player_delta_x;
        float y = rel_y + t_result*player_delta_y;
        if((t_result >= 0.0f) && (*t_min > t_result))
        {
            if((y >= min_y) && (y <= max_y))
            {
                *t_min = MAX(0.0f, t_result - t_epsilon);
                hit = true;
            }
        }
    }
    return hit;
}

void move_entity(entity* p_entity, sim_region* p_region, float dt, move_spec spec)
{
    if(spec.unit_max_accel_vector)
    {
        float ddp_length = length_sq(p_entity->m_ddp);
        if(ddp_length > 1.0f)
        {
            p_entity->m_ddp = glm::normalize(p_entity->m_ddp);
        }
    }

    p_entity->m_ddp = p_entity->m_ddp * spec.speed;
    p_entity->m_ddp = p_entity->m_ddp - p_entity->m_dp*spec.drag;

    glm::vec2 player_delta = p_entity->m_dp * dt + p_entity->m_ddp* 0.5f * dt * dt;
    p_entity->m_dp = p_entity->m_ddp * dt + p_entity->m_dp;
    glm::vec3 old_player_p = p_entity->m_p;
    glm::vec3 new_player_p = old_player_p + glm::vec3(player_delta.x, 0.0f, player_delta.y);
    
    for(uint32_t i = 0; i < p_region->num_chunks; ++i)
    {
        world_chunk* p_chunk = p_region->chunks_to_simulate[i];

        for(uint32_t iteration = 0; iteration < 4; ++iteration)
        {
            float t_min = 1.0f;
            glm::vec2 wall_normal = {};
            entity* hit_entity = NULL;
            glm::vec2 desired_position = glm::vec2(p_entity->m_p.x, p_entity->m_p.z) + player_delta;

            for(uint32_t index = 0; index < p_chunk->m_num_entities; ++index)
            {
                entity* test_entity = p_chunk->m_entities[index];
                float diameter_w = get_entity_width(test_entity) + get_entity_width(p_entity);
                float diameter_h = get_entity_height(test_entity) + get_entity_height(p_entity);
                glm::vec2 min_corner = {-0.5f*diameter_w, -0.5f*diameter_h};
                glm::vec2 max_corner = {0.5f*diameter_w, 0.5f*diameter_h};
                glm::vec3 rel3 = p_entity->m_p - test_entity->m_p;
                glm::vec2 rel = { rel3.x, rel3.z};

                if(test_wall(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                            &t_min, min_corner.y, max_corner.y))
                {
                    wall_normal = {-1, 0};
                    hit_entity = test_entity;
                }
                if(test_wall(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                            &t_min, min_corner.y, max_corner.y))
                {
                    wall_normal = {1,0};
                    hit_entity = test_entity;
                }
                if(test_wall(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, 
                            &t_min, min_corner.x, max_corner.x))
                {
                    wall_normal = {0, -1};
                    hit_entity = test_entity;
                }
                if(test_wall(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                            &t_min, min_corner.x, max_corner.x))
                {
                    wall_normal = {0, 1};
                    hit_entity = test_entity;
                }
            }
            
            p_entity->m_p = p_entity->m_p + glm::vec3(player_delta.x, 0.0f, player_delta.y) * glm::vec3(t_min);

            if(hit_entity)
            {
                p_entity->m_dp = p_entity->m_dp - wall_normal * glm::dot(p_entity->m_dp, wall_normal);
                player_delta = desired_position - glm::vec2(p_entity->m_p.x, p_entity->m_p.z);
                player_delta = player_delta - wall_normal * glm::dot(player_delta, wall_normal);
            }
            else
            {
                break;
            }
        }
    }
}
