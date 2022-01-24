#include "mesh.h"

#define  STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <unordered_map>
#include <queue>

#include <SDL.h>

#include "entity.h"
#include "character.h"

static char m_current_directory[256];
static uint32_t m_starting_time;
static volatile bool m_pause;

void set_pause_anim(bool pause)
{
    m_pause = pause;
}

bool get_pause_anim(void)
{
    return m_pause;
}

static void print_anim_data(character* p_character, skeletal_animation* p_anim)
{
    printf("Animation data\n");
    printf("-------------------------------------------\n");
    printf("Duration: %lf TPS: %lf Num channels: %d\n\n", p_anim->m_duration, p_anim->m_ticks_per_sec, p_anim->m_num_channels);
    for (uint32_t i = 0; i < p_character->m_num_joints; ++i)
    {
        anim_node* p_node = p_anim->m_channels + i;
        if (p_node->m_bone_id == 0xFF)
        {
            printf("No animation data for joint\n");
            continue;
        }
            
        printf("Channel[%d]\n", i);
        printf("-------------------------------------------\n");
        printf("Node name: %s, bone id: %d\n", p_node->node_name, p_node->m_bone_id);
        printf("Num scale keys: %d\n", p_node->m_num_scale_keys);
        printf("Num rotation keys: %d\n", p_node->m_num_rotation_keys);
        printf("Num position keys: %d\n\n", p_node->m_num_position_keys);
    }
}

void set_bone_transforms(character* p_character)
{
    char name[128];
    int len = 0;
    //first set all needed transforms
    for (uint32_t i = 0; i < p_character->m_num_joints; ++i)
    {
        memset(name, 0, len);
        len = snprintf(name, 128, "final_bone_matrices[%d]", i);
        set_mat4(p_character->s, name, p_character->m_final_transformations[i]);
    }
    //then set all un-needed transforms to zero
}

static float get_animation_running_time(skeletal_animation* p_anim, float dt)
{
    float running_time = p_anim->m_last_time + dt;
    return running_time;
}


static uint32_t get_animation_index(float anim_time, anim_node* p_anim_node, uint32_t last_index)
{
    if (anim_time < (float)p_anim_node->m_scale_keys[1].m_time && last_index > 1) //check wrap
    {
        last_index = 0;
    }

    for (uint32_t i = last_index; i < p_anim_node->m_num_scale_keys - 1; ++i)
    {
        if (anim_time < (float)p_anim_node->m_scale_keys[i + 1].m_time)
        {
            return i;
        }
    }

    assert(0); // shouldn't get here

    return 0;
}

static float get_keyframe_interp_factor(float anim_time, float last_time_index, float delta_time)
{
    float result = (anim_time - last_time_index) / delta_time;
    return result;
}

static glm::vec3 get_interpolated_position(float anim_time, anim_node* p_anim_node, uint32_t time_index)
{
    glm::vec3 result;

    if (p_anim_node->m_num_position_keys == 1)
    {
        result = p_anim_node->m_position_keys[0].m_value;
        return result;
    }
    uint32_t position_index = time_index;
    uint32_t next_position_index = position_index + 1;
    assert(next_position_index < p_anim_node->m_num_position_keys);
    float delta_time = (float)(p_anim_node->m_position_keys[next_position_index].m_time - p_anim_node->m_position_keys[position_index].m_time);
    float factor = get_keyframe_interp_factor(anim_time, (float)p_anim_node->m_position_keys[position_index].m_time, delta_time);
    assert(factor >= 0.0f && factor <= 1.0f);
    glm::vec3 start = p_anim_node->m_position_keys[position_index].m_value;
    glm::vec3 end = p_anim_node->m_position_keys[next_position_index].m_value;
    
    result = glm::mix(start, end, factor);
    return result;
}

static glm::quat get_interpolated_rotation(float anim_time, anim_node* p_anim_node, uint32_t time_index)
{
    glm::quat result;
    if (p_anim_node->m_num_rotation_keys == 1)
    {
        result = p_anim_node->m_rotation_keys[0].m_value;
        return result;
    }

    uint32_t rotation_index = time_index;
    uint32_t next_rotation_index = rotation_index + 1;
    assert(next_rotation_index < p_anim_node->m_num_rotation_keys);
    float delta_time = (float)(p_anim_node->m_rotation_keys[next_rotation_index].m_time - p_anim_node->m_rotation_keys[rotation_index].m_time);
    float factor = get_keyframe_interp_factor(anim_time, (float)p_anim_node->m_rotation_keys[rotation_index].m_time, delta_time);
    assert(factor >= 0.0f && factor <= 1.0f);
    glm::quat start = p_anim_node->m_rotation_keys[rotation_index].m_value;
    glm::quat end = p_anim_node->m_rotation_keys[next_rotation_index].m_value;
    result = glm::slerp(start, end, factor);
    result = glm::normalize(result);
    return result;
}

static glm::vec3 get_interpolated_scale(float anim_time, anim_node* p_anim_node, uint32_t time_index)
{
    glm::vec3 result;

    if (p_anim_node->m_num_scale_keys == 1)
    {
        result = p_anim_node->m_scale_keys[0].m_value;
        return result;
    }
    uint32_t scaling_index = time_index;
    uint32_t next_scaling_index = scaling_index + 1;
    assert(next_scaling_index < p_anim_node->m_num_scale_keys);
    float delta_time = (float)(p_anim_node->m_scale_keys[next_scaling_index].m_time - p_anim_node->m_scale_keys[scaling_index].m_time);
    float factor = get_keyframe_interp_factor(anim_time, (float)p_anim_node->m_scale_keys[scaling_index].m_time, delta_time);
    assert(factor >= 0.0f && factor <= 1.0f);
    glm::vec3 start = p_anim_node->m_scale_keys[scaling_index].m_value;
    glm::vec3 end = p_anim_node->m_scale_keys[next_scaling_index].m_value;
    
    result = glm::mix(start, end, factor);

    return result;
}

struct anim_times
{
    float data[2];
};

static anim_times update_animation_times(skeletal_animation* animations[2], float dt)
{
    anim_times result;
    
    for (uint8_t i = 0; i < 2; ++i)
    {
        skeletal_animation* anim = animations[i];
        anim->m_last_time = get_animation_running_time(anim, dt);

        float ticks_per_second = (float)anim->m_ticks_per_sec;
        float time_in_ticks = anim->m_last_time * ticks_per_second;
        float anim_time = fmod(time_in_ticks, (float)anim->m_duration);

        anim->m_last_time_index = get_animation_index(anim_time, &anim->m_channels[0],
            anim->m_last_time_index);
        result.data[i] = anim_time;
    }
    return result;
}

void get_bone_transforms(character* p_character, float dt, uint32_t anim_index_1, uint32_t anim_index_2, float blend_factor)
{
    //glm::mat4 inverse_root_transform = p_character->m_meshes[0].m_global_inv_transform;
    //glm::mat4 root_transform = glm::inverse(inverse_root_transform);

    memset(p_character->m_final_transformations, 0, sizeof(glm::mat4) * MAX_NUM_BONES);
    memset(p_character->m_local_transformations, 0, sizeof(glm::mat4) * MAX_NUM_BONES);

    p_character->m_num_transformations = 0;

    skeletal_animation* animations[2] = { &p_character->m_animations[anim_index_1] ,
                                          &p_character->m_animations[anim_index_2] };

    anim_times times = update_animation_times(animations, dt);

    for (uint8_t j = 0; j < p_character->m_num_joints; ++j)
    {
        anim_node* p_anim_node_1 = animations[0]->m_channels + j;
        anim_node* p_anim_node_2 = animations[1]->m_channels + j;

        joint* bone = p_character->m_skeleton + j;

        glm::mat4 anim_transform = bone->m_transformation;
        glm::mat4 parent_transform = glm::mat4(1.0f);
        
        if (p_anim_node_1->m_bone_id != 0xFF
            && p_anim_node_2->m_bone_id != 0xFF) // if this bone has animation data
        {
            glm::vec3 scale_1 = get_interpolated_scale(times.data[0], p_anim_node_1, animations[0]->m_last_time_index);
            glm::vec3 scale_2 = get_interpolated_scale(times.data[1], p_anim_node_2, animations[1]->m_last_time_index);
            glm::vec3 scale = glm::mix(scale_1 , scale_2, blend_factor);
            glm::mat4 scale_m = glm::scale(glm::mat4(1.0f), scale);

            glm::quat rotation_1 = get_interpolated_rotation(times.data[0], p_anim_node_1, animations[0]->m_last_time_index);
            glm::quat rotation_2 = get_interpolated_rotation(times.data[1], p_anim_node_2, animations[1]->m_last_time_index);
            glm::quat rotation = glm::slerp(rotation_1, rotation_2, blend_factor);
            glm::mat4 rotation_m = glm::toMat4(rotation);

            glm::vec3 translation_1 = get_interpolated_position(times.data[0], p_anim_node_1, animations[0]->m_last_time_index);
            glm::vec3 translation_2 = get_interpolated_position(times.data[1], p_anim_node_2, animations[1]->m_last_time_index);
            glm::vec3 translation = glm::mix(translation_1, translation_2, blend_factor);
            glm::mat4 translation_m = glm::translate(glm::mat4(1.0f), translation);

            anim_transform = translation_m * rotation_m * scale_m;
        }

        if (j > 0)
        {
            uint8_t parent_index = bone->m_parent;
            parent_transform = p_character->m_local_transformations[parent_index];
        }

        p_character->m_local_transformations[j] = parent_transform * anim_transform;
        p_character->m_final_transformations[j] = p_character->m_local_transformations[j] * bone->m_offset;

        p_character->m_num_transformations++;
    }
}

void mesh_component_init(void)
{   
    stbi_set_flip_vertically_on_load(false);
    m_starting_time = SDL_GetTicks();
    m_pause = false;
}

/*
    fill the buffer with all the characters but not including the last "character
    out buffer should be large enough, otherwise UB.
*/
void get_directory_name(const char* in_buffer, char* out_buffer, uint8_t character)
{
    uint32_t found_index = 0;

    size_t i = 0;
    size_t last_found = 0;

    while(in_buffer[i] != '\0')
    {
        if(in_buffer[i] == character)
        {
            last_found = i;
        }
        ++i;
    }

    if(last_found == 0)
    {
        //not found at all, out_buffer is garbage
        return;
    }

    memcpy(out_buffer, in_buffer, last_found);
    out_buffer[last_found + 1] = '\0';
}


//need to find directory first and then pass to this function
uint32_t load_texture_from_file(const char* path, bool gamma)
{
    uint32_t texture_id;
    glGenTextures(1, &texture_id);
    int32_t width, height, num_components;
    uint8_t* data = stbi_load(path, &width, &height, &num_components, 0);

    if(data)
    {
        GLenum format;
        switch(num_components)
        {
            case(1):
                format = GL_RED;
                break;
            case(3):
                format = GL_RGB;
                break;
            case(4):
                format = GL_RGBA;
                break;
            default:
                break;
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        printf("texture failed to load at path: %s\n", path);
    }
    return texture_id;
}

void load_material_textures(mesh* p_mesh, aiMaterial* mat, aiTextureType type, const char* type_name)
{
    uint32_t texture_count = mat->GetTextureCount(type);

    for(uint32_t i = 0; i < texture_count; ++i)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        asset_tag _tag;
        _tag.size = sizeof(asset_tag);
        _tag.type = ASSET_TYPE_TEXTURE;
        _tag.data = p_mesh->m_textures + p_mesh->m_num_textures;

        //construct full path
        char path[MAX_ASSET_PATH_LENGTH];
        char* slash = "/";
        memset(path, 0, MAX_ASSET_PATH_LENGTH);
        strcpy(path, m_current_directory);
        strcat(path, slash);
        strcat(path, str.C_Str());

        asset_tag tag_in_storage = find_asset_in_storage(path, _tag);

        if(_tag.data != tag_in_storage.data)
        {
            printf("No need to load again texture, found %s\n", path);
            //just copy the texture info
            memcpy(p_mesh->m_textures + (p_mesh->m_num_textures++), tag_in_storage.data, sizeof(texture));
            continue;
        }

        texture text;
        text.id   = load_texture_from_file(path, false);
        text.m_type = (char*)push_size(MAX_ASSET_PATH_LENGTH);
        text.m_path = (char*)push_size(MAX_ASSET_PATH_LENGTH);
        memset(text.m_type, 0, MAX_ASSET_PATH_LENGTH);
        memset(text.m_path, 0, MAX_ASSET_PATH_LENGTH);
        strcpy(text.m_type, type_name);
        strcpy(text.m_path, path);
        p_mesh->m_textures[p_mesh->m_num_textures++] = text;
    }
}

void setup_mesh(mesh* p_mesh)
{
    glGenVertexArrays(1, &p_mesh->vao);
    glGenBuffers(1, &p_mesh->vbo);
    glGenBuffers(1, &p_mesh->ebo);

    glBindVertexArray(p_mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p_mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, p_mesh->m_num_vertices * sizeof(vertex), p_mesh->m_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, p_mesh->m_num_indices*sizeof(uint32_t), p_mesh->m_indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tex_coords));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, bitangent));
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(vertex), (void*)offsetof(vertex, m_bone_ids));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, m_weights));
    glBindVertexArray(0);
}

static uint32_t get_mesh_texture_count(aiMaterial* mat)
{
    uint32_t result = mat->GetTextureCount(aiTextureType_DIFFUSE) +
                      mat->GetTextureCount(aiTextureType_SPECULAR) +
                      mat->GetTextureCount(aiTextureType_HEIGHT) +
                      mat->GetTextureCount(aiTextureType_AMBIENT);
    return result;
}

glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
    glm::mat4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

static inline glm::vec3 GetGLMVec(const aiVector3D& vec) 
{ 
    return glm::vec3(vec.x, vec.y, vec.z); 
}

static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
{
    return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

static uint8_t find_bone_by_name(character* p_character, const char* name)
{
    uint8_t result = 0xFF;

    for (uint8_t i = 0; i < p_character->m_num_joints; ++i)
    {
        char* joint_name = p_character->m_skeleton[i].m_name;
        if (strcmp(joint_name, name) == 0)
        {
            result = i;
            break;
        }
    }
    return result;
}

static void load_bones(character* p_character, mesh* p_mesh, aiMesh* ai_mesh)
{
    for (uint32_t i = 0; i < ai_mesh->mNumBones; ++i)
    {
        const char* bone_name = ai_mesh->mBones[i]->mName.data;
        int bone_index = find_bone_by_name(p_character, bone_name);
        //get offset
        joint* bone = p_character->m_skeleton + bone_index;
        bone->m_offset = ConvertMatrixToGLMFormat(ai_mesh->mBones[i]->mOffsetMatrix);
        if (bone_index != 0xFF) //found the bone
        {
            for (uint32_t j = 0; j < ai_mesh->mBones[i]->mNumWeights; ++j)
            {
                uint32_t vertex_id = ai_mesh->mBones[i]->mWeights[j].mVertexId;
                float weight = ai_mesh->mBones[i]->mWeights[j].mWeight;

                for (uint32_t k = 0; k < MAX_BONE_INFLUENCE; ++k)
                {
                    if (p_mesh->m_vertices[vertex_id].m_weights[k] == 0.0f)
                    {
                        p_mesh->m_vertices[vertex_id].m_weights[k]  = weight;
                        p_mesh->m_vertices[vertex_id].m_bone_ids[k] = bone_index;
                        break;
                    }
                }
            }
        }
    }
}

bool has_any_meshes(aiNode* p_node)
{
    if (p_node->mNumMeshes != 0)
    {
        return true;
    }
    else
    {
        if (p_node->mNumChildren == 0)
        {
            return false;
        }
        else
        {
            for (uint32_t i = 0; i < p_node->mNumChildren; ++i)
            {
                if (has_any_meshes(p_node->mChildren[i]) == true)
                {
                    return true;
                }
            }
            return false;
        }
    }
}

aiNode* get_root_joint(const aiScene* scene)
{
    aiNode* result = NULL;

    aiNode* root_node = scene->mRootNode;

    for (uint32_t i = 0; i < root_node->mNumChildren; ++i)
    {
        aiNode* child_node = root_node->mChildren[i];

        if (has_any_meshes(child_node) == false)
        {
            result = child_node;
            break;
        }
    }

    return result;
}

uint32_t count_nodes(aiNode* p_node)
{
    uint32_t count = 1;

    for (uint32_t i = 0; i < p_node->mNumChildren; ++i)
    {
        count += count_nodes(p_node->mChildren[i]);
    }

    return count;
}

void convert_skeleton_to_array(aiNode* root_node, joint* p_skeleton)
{
    uint32_t index = 0;

    std::queue<aiNode*>node_queue;
    std::queue<uint32_t>parent_indices;

    node_queue.push(root_node);
    parent_indices.push(0xFF);

    while (!node_queue.empty())
    {
        //add children to queue
        aiNode* node = node_queue.front();
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            node_queue.push(node->mChildren[i]);
            parent_indices.push(index);
        }

        //do top node
        joint cur_joint = {};
        cur_joint.m_parent = parent_indices.front();
        cur_joint.m_name = (char*)push_size(MAX_BONE_NAME_LEN);
        const char* node_name = node->mName.C_Str();
        strcpy(cur_joint.m_name, node_name);
        cur_joint.m_transformation = ConvertMatrixToGLMFormat(node->mTransformation);

        p_skeleton[index++] = cur_joint;

        //pop
        node_queue.pop();
        parent_indices.pop();
    }
}

static skeleton_load_result create_skeleton(const aiScene* scene)
{
    skeleton_load_result result = {};

    aiNode* root_joint = get_root_joint(scene);

    uint32_t num_joints = count_nodes(root_joint);
    
    printf("Num children root joint: %d\n", num_joints);

    result.m_num_joints = num_joints;
    result.m_skeleton = (joint*)push_size(num_joints * sizeof(joint));

    convert_skeleton_to_array(root_joint, result.m_skeleton);

    return result;
}

void print_skeleton(joint* p_skeleton, uint32_t num_joints)
{
    for (uint32_t i = 0; i < num_joints; ++i)
    {
        joint cur_joint = p_skeleton[i];
        printf("Index: %d, Parent Index: %d, Joint Name: %s\n", i, cur_joint.m_parent, cur_joint.m_name);
    }
}

static void load_animation(const aiScene* scene, character* p_character)
{
    const aiAnimation* p_ai_anim = scene->mAnimations[0];
    skeletal_animation* p_anim = p_character->m_animations + p_character->m_num_animations;

    p_anim->m_ticks_per_sec = p_ai_anim->mTicksPerSecond;
    p_anim->m_duration = p_ai_anim->mDuration;
    p_anim->m_num_channels = p_ai_anim->mNumChannels;

    //this needs to be properly updated when switching animations
    p_anim->m_last_time_index = 0;
    p_anim->m_last_time = 0.0f;
    //need to allocate enough memory for all bones. will check for != 0xFF while animating
    p_anim->m_channels = (anim_node*)push_size(p_character->m_num_joints * sizeof(anim_node));

    //nullify all bone_ids first
    for (uint32_t j = 0; j < p_character->m_num_joints; ++j)
    {
        p_anim->m_channels[j].m_bone_id = 0xFF;
    }

    for (uint32_t j = 0; j < p_anim->m_num_channels; ++j)
    {
        aiNodeAnim* p_ai_anim_node = p_ai_anim->mChannels[j];
        const char* bone_name = p_ai_anim_node->mNodeName.C_Str();
        uint32_t bone_index = find_bone_by_name(p_character, bone_name);

        anim_node* p_anim_node = p_anim->m_channels + bone_index;

        p_anim_node->m_bone_id = bone_index;

        p_anim_node->node_name = (char*)push_size(MAX_BONE_NAME_LEN);
        strcpy(p_anim_node->node_name, bone_name);

        p_anim_node->m_num_position_keys = p_ai_anim_node->mNumPositionKeys;
        p_anim_node->m_num_rotation_keys = p_ai_anim_node->mNumRotationKeys;
        p_anim_node->m_num_scale_keys = p_ai_anim_node->mNumScalingKeys;

        p_anim_node->m_position_keys = (pos_key*)push_size(p_anim_node->m_num_position_keys * sizeof(pos_key));
        p_anim_node->m_rotation_keys = (quat_key*)push_size(p_anim_node->m_num_rotation_keys * sizeof(quat_key));
        p_anim_node->m_scale_keys = (scale_key*)push_size(p_anim_node->m_num_scale_keys * sizeof(scale_key));

        for (uint32_t k = 0; k < p_anim_node->m_num_position_keys; ++k)
        {
            //get position keys
            pos_key* position = p_anim_node->m_position_keys + k;
            aiVectorKey* ai_position = p_ai_anim_node->mPositionKeys + k;

            position->m_time = ai_position->mTime;
            position->m_value = glm::vec3(ai_position->mValue.x, ai_position->mValue.y, ai_position->mValue.z);
            //get scale keys
            scale_key* scale = p_anim_node->m_scale_keys + k;
            aiVectorKey* ai_scale = p_ai_anim_node->mScalingKeys + k;

            scale->m_time = ai_scale->mTime;
            scale->m_value = glm::vec3(ai_scale->mValue.x, ai_scale->mValue.y, ai_scale->mValue.z);
            //get rotation keys
            quat_key* rotation = p_anim_node->m_rotation_keys + k;
            aiQuatKey* ai_rotation = p_ai_anim_node->mRotationKeys + k;

            rotation->m_time = ai_rotation->mTime;
            rotation->m_value = GetGLMQuat(ai_rotation->mValue);
        }
    }
    p_character->m_num_animations++;
}

void load_animation_from_file(character* p_character, const char* path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |
        aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        printf("ERROR::ASSIMP:: %s\n", importer.GetErrorString());
        return;
    }
    load_animation(scene, p_character);
}

static void load_vertices(aiMesh* ai_mesh, mesh* p_mesh, uint32_t mesh_vertex_count)
{
    for (uint32_t j = 0; j < mesh_vertex_count; ++j)
    {
        vertex vert;
        glm::vec3 vec;
        vec.x = ai_mesh->mVertices[j].x;
        vec.y = ai_mesh->mVertices[j].y;
        vec.z = ai_mesh->mVertices[j].z;
        vert.position = vec;

        if (ai_mesh->HasNormals())
        {
            vec.x = ai_mesh->mNormals[j].x;
            vec.y = ai_mesh->mNormals[j].y;
            vec.z = ai_mesh->mNormals[j].z;
            vert.normal = vec;
        }

        if (ai_mesh->mTextureCoords[0])
        {
            glm::vec2 vec2;
            vec2.x = ai_mesh->mTextureCoords[0][j].x;
            vec2.y = ai_mesh->mTextureCoords[0][j].y;
            vert.tex_coords = vec2;

            vec.x = ai_mesh->mTangents[j].x;
            vec.y = ai_mesh->mTangents[j].y;
            vec.z = ai_mesh->mTangents[j].z;
            vert.tangent = vec;

            vec.x = ai_mesh->mBitangents[j].x;
            vec.y = ai_mesh->mBitangents[j].y;
            vec.z = ai_mesh->mBitangents[j].z;
            vert.bitangent = vec;
        }
        else
        {
            vert.tex_coords = glm::vec2(0.0f, 0.0f);
        }

        //default bone values
        for (uint32_t k = 0; k < MAX_BONE_INFLUENCE; ++k)
        {
            vert.m_bone_ids[k] = 0xFF;
            vert.m_weights[k] = 0.0f;
        }

        p_mesh->m_vertices[j] = vert;
    }
}

static void load_indices(aiMesh* ai_mesh, mesh* p_mesh)
{
    //assume every face is a triangle
    uint32_t num_faces = ai_mesh->mNumFaces;
    uint32_t num_indices = 0;
    uint32_t index_count = num_faces * 3;
    p_mesh->m_indices = (uint32_t*)push_size(index_count * sizeof(uint32_t));
    p_mesh->m_num_indices = index_count;

    for (uint32_t j = 0; j < num_faces; ++j)
    {
        aiFace face = ai_mesh->mFaces[j];
        for (uint32_t k = 0; k < face.mNumIndices; ++k)
        {
            p_mesh->m_indices[num_indices++] = face.mIndices[k];
        }
    }
}

static void load_materials(const aiScene* scene, aiMesh* ai_mesh, mesh* p_mesh)
{
    aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];
    uint32_t num_textures = get_mesh_texture_count(material);

    p_mesh->m_textures = (texture*)push_size(num_textures * sizeof(texture));
    p_mesh->m_num_textures = 0;

    load_material_textures(p_mesh, material, aiTextureType_DIFFUSE, "texture_diffuse");
    load_material_textures(p_mesh, material, aiTextureType_SPECULAR, "texture_specular");
    load_material_textures(p_mesh, material, aiTextureType_HEIGHT, "texture_normal");
    load_material_textures(p_mesh, material, aiTextureType_AMBIENT, "texture_height");

    setup_mesh(p_mesh);
}

static void load_meshes(const aiScene* scene, entity* p_entity, uint32_t mesh_count, uint32_t animation_count)
{
    //now need to extract data from assimp data structure
    for (uint32_t i = 0; i < mesh_count; ++i)
    {
        aiMesh* ai_mesh = scene->mMeshes[i];

        mesh* p_mesh = p_entity->m_meshes + i;
        p_mesh->m_global_inv_transform = glm::inverse(ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation));

        //initiate mesh and increment numbers
        uint32_t mesh_vertex_count = ai_mesh->mNumVertices;
        p_mesh->m_vertices = (vertex*)push_size(mesh_vertex_count * sizeof(vertex));
        p_mesh->m_num_vertices = mesh_vertex_count;

        load_vertices(ai_mesh, p_mesh, mesh_vertex_count);
        /* extract bone information */
        if (p_entity->m_type == ET_CHARACTER)
        {
            character* p_character = (character*)p_entity;
            load_bones(p_character, p_mesh, ai_mesh);
            //allocate animations
            p_character->m_num_animations = 0;
            p_character->m_animations = (skeletal_animation*)push_size(animation_count * sizeof(skeletal_animation));

            load_animation(scene, p_character);
        }
        load_indices(ai_mesh, p_mesh);
        load_materials(scene, ai_mesh, p_mesh);
    }
}

static void load_skeleton(const aiScene* scene, character* p_character)
{
    //check skeleton
    skeleton_load_result anim_skeleton = create_skeleton(scene);
    print_skeleton(anim_skeleton.m_skeleton, anim_skeleton.m_num_joints);

    p_character->m_skeleton = anim_skeleton.m_skeleton;
    p_character->m_num_joints = anim_skeleton.m_num_joints;
}

void load_model_from_file(entity* p_entity, const char* path, uint32_t num_animations)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | 
                                                   aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
                                                   aiProcess_CalcTangentSpace);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        printf("ERROR::ASSIMP:: %s\n", importer.GetErrorString());
        return;
    }
    //clear directory buffer
    memset(m_current_directory, 0, sizeof(m_current_directory));
    //get the current working directory
    get_directory_name(path, m_current_directory, '/');

    uint32_t mesh_count = scene->mNumMeshes;
    p_entity->m_num_meshes = mesh_count;
    p_entity->m_meshes = (mesh*)push_size(p_entity->m_num_meshes * sizeof(mesh));
    
    if (p_entity->m_type == ET_CHARACTER)
    {
        load_skeleton(scene, (character*)p_entity);
    }

    load_meshes(scene, p_entity, mesh_count, num_animations);
}

void draw_mesh(mesh* p_mesh, shader s)
{
    uint32_t diffuse_nr  = 1;
    uint32_t specular_nr = 1;
    uint32_t normal_nr   = 1;
    uint32_t height_nr   = 1;

    texture* mesh_textures = p_mesh->m_textures;

    for(uint32_t i = 0; i < p_mesh->m_num_textures; ++i)
    {
        texture* text = mesh_textures + i;

        glActiveTexture(GL_TEXTURE0 + i);

        char texture_name[MAX_ASSET_PATH_LENGTH];
        char number[2];
        memset(texture_name, 0, MAX_ASSET_PATH_LENGTH);
        memset(number, 0, 2);
        strcpy(texture_name, text->m_type);

        if(strcmp(texture_name, "texture_diffuse") == 0)
        {
            number[0] = (diffuse_nr++) + '0';
        }
        else if(strcmp(texture_name, "texture_specular") == 0)
        {
            number[0] = (specular_nr++) + '0';
        }
        else if(strcmp(texture_name, "texture_normal") == 0)
        {
            number[0] = (normal_nr++) + '0';
        }
        else if(strcmp(texture_name, "texture_height") == 0)
        {
            number[0] = (height_nr++) + '0';
        }
        number[1] = '\0';
        strcat(texture_name, number);
        glUniform1i(glGetUniformLocation(s.id, texture_name), i);
        glBindTexture(GL_TEXTURE_2D, text->id);
    }

    glBindVertexArray(p_mesh->vao);
    glDrawElements(GL_TRIANGLES, p_mesh->m_num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}
