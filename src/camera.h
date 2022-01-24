#ifndef CAMERA_H_
#define CAMERA_H_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct camera
{
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    float yaw;
    float pitch;
    float movement_speed;
    float mouse_sensitivity;
    float zoom;
};

enum camera_movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    NONE
};

glm::vec3  get_camera_position(void);
float      get_camera_near(void);
float      get_camera_far(void);
void       update_camera_vectors(void);
void       camera_init(void);
void       set_camera_position(glm::vec3& p);
glm::mat4  get_view_matrix(void);
glm::mat4  get_perspective_matrix(void);
void       update_perspective_matrix(void);
void       update_view_matrix(void);
void       process_mouse_movement(int32_t x_offset, int32_t y_offset);
void       process_keyboard(camera_movement movement, float dt);

#endif
