#include "camera.h"
#include <stdio.h>

#include <glm/gtc/type_ptr.hpp>

static int32_t last_x;
static int32_t last_y;
static volatile bool first_mouse;

static const float camera_yaw         = -90.0f;
static const float camera_pitch       = -45.0f;
static const float camera_speed       = 2.5f;
static const float camera_sensitivity = 0.1f;
static const float camera_zoom        = 45.0f;
static const float aspect_ratio       = 16.0f/9.0f;
static const float near               = 0.1f;
static const float far                = 100.0f;

static camera game_camera;

static glm::mat4 perspective;
static glm::mat4 view;

void update_perspective_matrix(void)
{
    perspective = glm::perspective(glm::radians(game_camera.zoom), aspect_ratio, near, far);
}

void update_view_matrix(void)
{
    view = glm::lookAt(game_camera.position, game_camera.position + game_camera.front, game_camera.up);
}

float get_camera_near(void)
{
    return near;
}

float get_camera_far(void)
{
    return far;
}

void camera_init(void)
{
    game_camera.position          = glm::vec3(0.0f, 10.0f, 10.0f);
    game_camera.up                = glm::vec3(0.0f, 1.0f, 0.0f);
    game_camera.world_up          = game_camera.up;
    game_camera.yaw               = camera_yaw;
    game_camera.pitch             = camera_pitch;
    game_camera.front             = glm::vec3(0.0f, 0.0f, -1.0f);
    game_camera.movement_speed    = camera_speed;
    game_camera.mouse_sensitivity = camera_sensitivity;
    game_camera.zoom              = camera_zoom;

    last_x = 0;
    last_y = 0;
    first_mouse = true;

    update_camera_vectors();
}

void update_camera_vectors(void)
{
    glm::vec3 front;
    front.x = cos(glm::radians(game_camera.yaw))*cos(glm::radians(game_camera.pitch));
    front.y = sin(glm::radians(game_camera.pitch));
    front.z = sin(glm::radians(game_camera.yaw))*cos(glm::radians(game_camera.pitch));
    game_camera.front = glm::normalize(front);

    game_camera.right = glm::normalize(glm::cross(game_camera.front, game_camera.world_up));
    game_camera.up    = glm::normalize(glm::cross(game_camera.right, game_camera.front));
}


glm::mat4 get_view_matrix(void)
{
    return view;
}

glm::mat4 get_perspective_matrix(void)
{
    return perspective;
}

void process_mouse_movement(int32_t x, int32_t y)
{
    if(first_mouse)
    {
        last_x = x;
        last_y = y;
        first_mouse = false;
    }

    int32_t x_offset = x - last_x;
    int32_t y_offset = y - last_y;

    x_offset *= game_camera.mouse_sensitivity;
    y_offset *= game_camera.mouse_sensitivity;

    last_x = x;
    last_y = y;

    game_camera.yaw   += x_offset;
    game_camera.pitch += y_offset;

    if(game_camera.pitch > 89.0f)
    {
        game_camera.pitch = 89.0f;
    }
    if(game_camera.pitch < -89.0f)
    {
        game_camera.pitch = -89.0f;
    }
    update_camera_vectors();
}

void process_keyboard(camera_movement movement, float dt)
{
    float velocity = game_camera.movement_speed*dt;
    switch(movement)
    {
        case(FORWARD):
        {
            game_camera.position += game_camera.front*velocity;
        }break;
        case(BACKWARD):
        {
            game_camera.position -= game_camera.front*velocity;
        }break;
        case(LEFT):
        {
            game_camera.position -= game_camera.right*velocity;
        }break;
        case(RIGHT):
        {
            game_camera.position += game_camera.right*velocity;
        }break;
        default:
            break;
    }
}

void set_camera_position(glm::vec3& p)
{
    game_camera.position = p;
}

glm::vec3 get_camera_position(void)
{
    return game_camera.position;
}