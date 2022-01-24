#ifndef SHADER_H_
#define SHADER_H_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stdio.h>

struct shader
{
    uint32_t id;
};

inline void   use_shader(shader s) { glUseProgram(s.id); }
void   check_compile_errors(GLuint shader, char* type);
shader create_default_shader(void);
shader create_debug_rect_shader(void);
shader create_text_shader(void);

shader create_shader(char* vertex_code, char* fragment_code, char* geometry_code);

void   use_shader(shader s);
void   set_bool(shader s, char* name, bool value);
void   set_int(shader s, char* name, int32_t value);
void   set_float(shader s, char* name, float value);
void   set_vec2(shader s, char* name, glm::vec2& vec);
void   set_vec2(shader s, char* name, float x, float y);
void   set_vec3(shader s, char* name, glm::vec3 &value);
void   set_vec3(shader s, char* name, float x, float y, float z);
void   set_vec4(shader s, char* name, glm::vec4 &value);
void   set_vec4(shader s, char* name, float x, float y, float z, float w);
void   set_mat2(shader s, char* name, glm::mat2 &mat);
void   set_mat3(shader s, char* name, glm::mat3 &mat);
void   set_mat4(shader s, char* name, glm::mat4 &mat);

#endif