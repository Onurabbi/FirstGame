#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "asset.h"
#include "math.h"
#include "memory.h"
#include "hash.h"
#include "common.h"
#include "shader.h"

#define MAX_BONE_INFLUENCE      4
#define MAX_BONE_NAME_LEN       64
#define MAX_NUM_BONES           128

struct entity;
struct character;

struct quat_key
{
    glm::quat m_value;
    double    m_time;
};

struct pos_key
{
    glm::vec3 m_value;
    double    m_time;
};

struct scale_key
{
    glm::vec3 m_value;
    double    m_time;
};

//transformations of a single bone
struct anim_node
{
    char* node_name;
    quat_key*  m_rotation_keys;
    pos_key*   m_position_keys;
    scale_key* m_scale_keys;
    uint32_t   m_num_rotation_keys;
    uint32_t   m_num_position_keys;
    uint32_t   m_num_scale_keys;
    uint8_t    m_bone_id; //index into the skeleton
};

struct skeletal_animation
{
    double     m_duration;
    double     m_ticks_per_sec;
    anim_node* m_channels;
    uint32_t   m_num_channels;
    uint32_t   m_last_time_index;
    float      m_last_time;
};

struct bone_info
{
    int32_t   id;
    glm::mat4 offset;
};

struct vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    int     m_bone_ids[MAX_BONE_INFLUENCE];
    float   m_weights[MAX_BONE_INFLUENCE];
};

struct texture
{
    char*    m_path;
    char*    m_type;
    uint32_t id;
};

struct joint
{
    glm::mat4   m_transformation;
    glm::mat4   m_offset;
    char*       m_name;
    uint8_t     m_parent;
};

struct mesh
{
    vertex*     m_vertices;
    texture*    m_textures;
    uint32_t*   m_indices;
    
    uint32_t    m_num_vertices;
    uint32_t    m_num_textures;
    uint32_t    m_num_indices;
    
    uint32_t    vao, vbo, ebo;

    glm::mat4 m_global_inv_transform;
};

struct skeleton_load_result
{
    joint*   m_skeleton;
    uint32_t m_num_joints;
};


bool      get_pause_anim(void);
void      set_pause_anim(bool pause);
void      set_bone_transforms(character* p_character);
void      get_bone_transforms(character* p_character, float dt, uint32_t anim_index_1, uint32_t anim_index_2, float blend_factor);
uint32_t  load_texture_from_file(const char* texture_name, bool gamma);
void      load_material_textures(mesh* p_mesh, aiMaterial* mat, aiTextureType type, const char* path);
void      get_directory_name(const char* in_buffer, char* out_buffer, uint8_t character);
void      load_model_from_file(entity* p_entity, const char* path, uint32_t num_animations);
void      load_animation_from_file(character* p_character, const char* path);
void      mesh_component_init(void);
void      setup_mesh(mesh* p_mesh);
void      draw_mesh(mesh* p_mesh, shader s);
glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from);

#endif
