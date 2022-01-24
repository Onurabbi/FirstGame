#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "mesh.h"
#include "math.h"
#include "shader.h"
#include "entity.h"

struct character : public entity
{
    joint*              m_skeleton;
    skeletal_animation* m_animations;
    glm::mat4*          m_final_transformations;
    glm::mat4*          m_local_transformations;
    uint32_t            m_num_transformations;
    uint32_t            m_num_joints;
    uint32_t            m_num_animations;
    glm::vec3           m_move_target;
    glm::quat           m_rotate_target;
    bool                m_should_move;    //get rid of this or combine into "flags"?
    bool                m_should_rotate;  //get rid of this or combine into "flags"?
};

void       characters_init(void);
character* put_character_in_storage(character* p_character);
character* create_character(character* p_character);

#endif

