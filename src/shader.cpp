#include "shader.h"
#include <cstring>

char* debug_rect_vs = R"FOO(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)FOO";

char* debug_rect_fs = R"FOO(
#version 330 core
out vec4 FragColor;
uniform vec3 in_color;

void main()
{
    FragColor = vec4(in_color, 1.0);
}
)FOO";

char* volatile text_rendering_vs = R"FOO(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 tex_coord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    tex_coord = vec2(aTexCoord.x, aTexCoord.y);
}
)FOO";

char* volatile text_rendering_fs = R"FOO(
#version 330 core
out vec4 FragColor;

in vec2 tex_coord;

uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, tex_coord);
}
)FOO";

char* model_loading_vs = R"FOO(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in ivec4 aBoneIds;
layout (location = 6) in vec4 aWeights;

out vec2 TexCoords;

const int MAX_NUM_BONES = 128;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 final_bone_matrices[MAX_NUM_BONES];
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 total_position = vec4(0.0f);
    for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if(aBoneIds[i] == 0xFF)
        {
            continue;
        }
        if(aBoneIds[i] >= MAX_NUM_BONES)
        {
            total_position = vec4(aPos, 1.0f);
            break;
        }
        vec4 local_position = final_bone_matrices[aBoneIds[i]] * vec4(aPos, 1.0f);
        total_position += local_position * aWeights[i];
    }
    gl_Position = projection * view * model * total_position;
    TexCoords = aTexCoords;    
}
)FOO";

char* model_loading_fs = R"FOO(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{    
    FragColor = texture(texture_diffuse1, TexCoords);
}
)FOO";

void check_compile_errors(GLuint shader, char* type)
{
    GLint success;
    GLchar info_log[1024];

    if(strcmp(type, "PROGRAM") != 0)
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, info_log);
            printf("ERROR::SHADER_COMPILATION_ERROR of type:\n%s\n-----------------------------------------------------------\n", info_log);
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, info_log);
            printf("ERROR::SHADER_LINKING_ERROR of type:\n%s\n-----------------------------------------------------------\n", info_log);
        }
    }
}

shader create_text_shader(void)
{
    return create_shader(text_rendering_vs, text_rendering_fs, NULL);
}

shader create_debug_rect_shader(void)
{
    return create_shader(debug_rect_vs, debug_rect_fs, NULL);
}

shader create_default_shader(void)
{
    return create_shader(model_loading_vs, model_loading_fs, NULL);
}

shader create_shader(char* vertex_code, char* fragment_code, char* geometry_code)
{
    uint32_t vertex, fragment;
    shader result;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code, NULL);
    glCompileShader(vertex);
    check_compile_errors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code, NULL);
    glCompileShader(fragment);
    check_compile_errors(fragment, "FRAGMENT");

    uint32_t geometry;

    if(geometry_code != NULL)
    {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geometry_code, NULL);
        glCompileShader(geometry);
        check_compile_errors(geometry, "GEOMETRY");
    }
    uint32_t id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    if(geometry_code != NULL)
    {
        glAttachShader(id, geometry);
    }
    glLinkProgram(id);
    check_compile_errors(id, "PROGRAM");
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if(geometry_code != NULL)
    {
        glDeleteShader(geometry);
    }
    result.id = id;
    return result;
}


void set_bool(shader s, char* name, bool value)
{
    glUniform1i(glGetUniformLocation(s.id, name), (GLint)value);
}

void set_int(shader s, char* name, int32_t value)
{
    glUniform1i(glGetUniformLocation(s.id, name), value);
}

void set_float(shader s, char* name, float value)
{
    glUniform1f(glGetUniformLocation(s.id, name), value);
}

void set_vec2(shader s, char* name, glm::vec2& vec)
{
    glUniform2fv(glGetUniformLocation(s.id, name), 1, &vec[0]);
}
 
void set_vec2(shader s, char* name, float x, float y)
{
    glUniform2f(glGetUniformLocation(s.id, name), x, y);
}

void set_vec3(shader s, char* name, glm::vec3 &value) 
{ 
    glUniform3fv(glGetUniformLocation(s.id, name), 1, &value[0]); 
}
void set_vec3(shader s, char* name, float x, float y, float z) 
{ 
    glUniform3f(glGetUniformLocation(s.id, name), x, y, z); 
}

void set_vec4(shader s, char* name, glm::vec4 &value)
{ 
    glUniform4fv(glGetUniformLocation(s.id, name), 1, &value[0]); 
}

void set_vec4(shader s, char* name, float x, float y, float z, float w) 
{ 
    glUniform4f(glGetUniformLocation(s.id, name), x, y, z, w); 
}

void set_mat2(shader s, char* name, glm::mat2 &mat)
{
    glUniformMatrix2fv(glGetUniformLocation(s.id, name), 1, GL_FALSE, &mat[0][0]);
}

void set_mat3(shader s, char* name, glm::mat3 &mat)
{
    glUniformMatrix3fv(glGetUniformLocation(s.id, name), 1, GL_FALSE, &mat[0][0]);
}

void set_mat4(shader s, char* name, glm::mat4 &mat) 
{
    glUniformMatrix4fv(glGetUniformLocation(s.id, name), 1, GL_FALSE, &mat[0][0]);
}