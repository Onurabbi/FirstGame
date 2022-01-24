#include <SDL.h>
#include <SDL_main.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <stdio.h>
#include <glad/glad.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "entity.h"
#include "world.h"
#include "math.h"
#include "memory.h"
#include "asset.h"
#include "collision.h"
#include "mesh.h"
#include "camera.h"
#include "thread.h"
#include "character.h"

#include <stb/stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H  

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

enum buttons
{
    BUTTONS_W,
    BUTTONS_S,
    BUTTONS_A,
    BUTTONS_D,
    BUTTONS_Q,
    BUTTONS_E,
    BUTTONS_P,
    BUTTONS_O,
    BUTTONS_1,
    BUTTONS_2,
    BUTTONS_3,
    BUTTONS_4,
    BUTTONS_5,
    BUTTONS_COUNT
};

struct input
{
    uint8_t buttons_pressed[BUTTONS_COUNT];
    float mouse_x, mouse_y;
    bool  left_click;
};

struct debug_draw_info
{
    uint32_t vao, vbo, ebo;
    shader s;
};

GLuint VBO;

float g_player_vel;
bool g_running = false;
world* g_world = NULL;
camera* g_camera = NULL;
SDL_Window* g_window = NULL;
SDL_Renderer* g_sdl_renderer = NULL;
SDL_GLContext g_context = NULL;
Mix_Music* g_music = NULL;
Mix_Chunk* g_scratch = NULL;
Mix_Chunk* g_high = NULL;
Mix_Chunk* g_medium = NULL;
Mix_Chunk* g_low = NULL;
debug_draw_info g_debug_draw;
entity* obstacle = NULL;

extern uint32_t load_texture_from_file(const char* path, bool gamma);

static bool sdl_init(void)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        printf("SDL could not initialize! SDL_error: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    g_window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if(g_window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    g_context = SDL_GL_CreateContext(g_window);

    if(g_context == NULL)
    {
        printf("OpenGL context could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    if(!gladLoadGLLoader(SDL_GL_GetProcAddress))
    {
        printf("Could not initialize GLAD\n");
        return false;
    }

    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n", glGetString(GL_VERSION));

    SDL_GL_SetSwapInterval(1);

    // Disable depth test and face culling.
    glEnable(GL_DEPTH_TEST);

    int img_flags = IMG_INIT_PNG;

    if(!(IMG_Init(img_flags) & img_flags))
    {
        printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
        return false;       
    }

    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
        return false;
    }

    return true;
}

static sim_region get_chunks_to_simulate(glm::vec3& center, glm::vec2& size)
{
    sim_region result = {};

    glm::vec2 upper_left  = { center.x - size.x / 2.0f, center.z + size.y / 2.0f };
    glm::vec2 upper_right = { center.x + size.x / 2.0f, center.z + size.y / 2.0f };
    glm::vec2 lower_left  = { center.x - size.x / 2.0f, center.z - size.y / 2.0f };
    glm::vec2 lower_right = { center.x + size.x / 2.0f, center.z - size.y / 2.0f };

    //if upper left and lower right are equal, all are equal. 
    world_chunk* upper_left_chunk  = map_world_position_to_world_chunk(upper_left);
    world_chunk* lower_right_chunk = map_world_position_to_world_chunk(lower_right);
    world_chunk* upper_right_chunk = map_world_position_to_world_chunk(upper_right);
    world_chunk* lower_left_chunk  = map_world_position_to_world_chunk(lower_left);

    if(upper_left_chunk == lower_right_chunk)
    {
        result.chunks_to_simulate[0] = upper_left_chunk;
        result.num_chunks = 1;
    }
    else if((upper_left_chunk == upper_right_chunk) || (upper_left_chunk == lower_left_chunk))
    {
        result.chunks_to_simulate[0] = upper_left_chunk;
        result.chunks_to_simulate[1] = lower_right_chunk;
        result.num_chunks = 2;
    }
    else
    {
        result.chunks_to_simulate[0] = upper_left_chunk;
        result.chunks_to_simulate[1] = upper_right_chunk;
        result.chunks_to_simulate[2] = lower_right_chunk;
        result.chunks_to_simulate[3] = lower_left_chunk;
        result.num_chunks = 4;
    }
    return result;
}

static uint32_t simulate_world_chunk(world_chunk* p_chunk)
{
    uint32_t num_entities = p_chunk->m_num_entities;
    return num_entities;
}


static volatile bool g_pause = false;

//convert pixel coordinates to screen space coordinates -1...1
/*
*   top left:     -1, 1
*   bottom left:  -1,-1
*   bottom right:  1,-1
*   top right:     1, 1
*/
static float convert_pixels_to_screen_coordinates(int32_t pixels, int32_t screen_size)
{
    float result;
    result = (float)(pixels - screen_size / 2) / (screen_size/2);
    return result;
}

//returns mouse position in screen space -1..1, -1..1
static void get_mouse_state(input* inp)
{
    int32_t mouse_x_pixels;
    int32_t mouse_y_pixels;
    uint32_t buttons;

    buttons = SDL_GetMouseState(&mouse_x_pixels, &mouse_y_pixels);
    if ((buttons & SDL_BUTTON_LMASK) != 0)
    {
        inp->left_click = true;
    }

    inp->mouse_x = convert_pixels_to_screen_coordinates(mouse_x_pixels, SCREEN_WIDTH);
    inp->mouse_y = -1.0f * convert_pixels_to_screen_coordinates(mouse_y_pixels, SCREEN_HEIGHT);
}

static input handle_input(void)
{
    input inp = {};

    SDL_Event e;

    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT)
        {
            g_running = false;
            break;
        }
        else if(e.type == SDL_KEYDOWN)
        {
            switch(e.key.keysym.sym)
            {
                case SDLK_1:
                Mix_PlayChannel( -1, g_high, 0 );
                break;
                
                //Play medium sound effect
                case SDLK_2:
                Mix_PlayChannel( -1, g_medium, 0 );
                break;
                
                //Play low sound effect
                case SDLK_3:
                Mix_PlayChannel( -1, g_low, 0 );
                break;
                
                //Play scratch sound effect
                case SDLK_4:
                Mix_PlayChannel( -1, g_scratch, 0 );
                break;

                case SDLK_p:
                g_pause = !g_pause;
                break;
            }                                   
        }
    }

    const uint8_t* current_key_states = SDL_GetKeyboardState(NULL);

    if(current_key_states[SDL_SCANCODE_W])
    {
        inp.buttons_pressed[BUTTONS_W] = 1;
    }
    if(current_key_states[SDL_SCANCODE_S])
    {
        inp.buttons_pressed[BUTTONS_S] = 1;
    }
    if(current_key_states[SDL_SCANCODE_A])
    {
        inp.buttons_pressed[BUTTONS_A] = 1;
    }
    if(current_key_states[SDL_SCANCODE_D])
    {
        inp.buttons_pressed[BUTTONS_D] = 1;
    }
    
    get_mouse_state(&inp);

    return inp;
}

static void debug_draw_init()
{
    g_debug_draw.s = create_debug_rect_shader();

    float vertices[] = 
    {
        0.5f, 0.0f,  0.5f, 
        0.5f, 0.0f, -0.5f,
       -0.5f, 0.0f, -0.5f,
       -0.5f, 0.0f,  0.5f
    };

    uint32_t indices[] = 
    {
        0,1,
        1,2,
        2,3,
        3,0
    };

    glGenVertexArrays(1, &g_debug_draw.vao);
    glGenBuffers(1, &g_debug_draw.vbo);
    glGenBuffers(1, &g_debug_draw.ebo);

    glBindVertexArray(g_debug_draw.vao);

    glBindBuffer(GL_ARRAY_BUFFER, g_debug_draw.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_debug_draw.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

static void draw_debug_rect(entity* p_entity, shader s, glm::vec3& color)
{
    use_shader(s);
    
    glm::mat4 model = glm::mat4(1.0f);
    float scale = p_entity->m_collision.x/0.5f;
    model = glm::translate(model, p_entity->m_p);
    model = glm::scale(model, glm::vec3(scale));

    glm::mat4 projection = get_perspective_matrix();
    glm::mat4 view = get_view_matrix();

    set_mat4(s, "projection", projection);
    set_mat4(s, "view", view);
    set_mat4(s, "model", model);
    set_vec3(s, "in_color", color);

    glBindVertexArray(g_debug_draw.vao);
    glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, 0);
}

static void draw_debug_rect(glm::vec3& position, entity* p_entity, shader s, glm::vec3& color)
{
    use_shader(s);

    glm::mat4 model = glm::mat4(1.0f);
    float scale = p_entity->m_collision.x / 0.5f;
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(scale));

    glm::mat4 projection = get_perspective_matrix();
    glm::mat4 view = get_view_matrix();

    set_mat4(s, "projection", projection);
    set_mat4(s, "view", view);
    set_mat4(s, "model", model);
    set_vec3(s, "in_color", color);

    glBindVertexArray(g_debug_draw.vao);
    glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, 0);
}

struct ray
{
    glm::vec3 origin;
    glm::vec3 dir;

    glm::vec3 invdir;
    uint8_t   sign[3];
};

static void create_ray(ray* p_ray, glm::vec3& origin, glm::vec3 dir)
{
    p_ray->origin = origin;
    p_ray->dir = dir;
   
    p_ray->invdir = 1.0f / dir;
    p_ray->sign[0] = (p_ray->invdir.x < 0);
    p_ray->sign[1] = (p_ray->invdir.y < 0);
    p_ray->sign[2] = (p_ray->invdir.z < 0);
}

static ray cast_ray_to_mouse_cursor(input* inp)
{
    ray result;

    glm::mat4 inv_pv = glm::inverse(get_perspective_matrix() * get_view_matrix());
    glm::vec4 near = glm::vec4(inp->mouse_x, inp->mouse_y, -1.0f, 1.0f);
    glm::vec4 far = glm::vec4(inp->mouse_x, inp->mouse_y, 1.0f, 1.0f);
    glm::vec4 near_res = inv_pv * near;
    glm::vec4 far_res = inv_pv * far;
    near_res /= near_res.w;
    far_res /= far_res.w;
    glm::vec3 ray_dir = glm::vec3(far_res - near_res);
    ray_dir = glm::normalize(ray_dir);

    create_ray(&result, get_camera_position(), ray_dir);

    return result;
}

bool ray_vs_plane(glm::vec3& origin, glm::vec3& normal, ray* r, float* t)
{
    float denom = glm::dot(normal, r->dir);
    if (glm::abs(denom) >  0.000001f)
    {
        glm::vec3 p0l0 = origin - r->origin;
        *t = glm::dot(p0l0, normal)/denom;
        return (*t >= 0.0f);
    }
    return false;
}

bool ray_vs_box(ray* r, box3* b)
{
    glm::vec3 min = b->min;
    glm::vec3 max = b->max;
    
    float tmin = (min.x - r->origin.x) * r->invdir.x;
    float tmax = (max.x - r->origin.x) * r->invdir.x;

    if (tmin > tmax) swap_f(tmin, tmax);

    float tymin = (min.y - r->origin.y) * r->invdir.y;
    float tymax = (max.y - r->origin.y) * r->invdir.y;

    if (tymin > tymax) swap_f(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (min.z - r->origin.z) * r->invdir.z;
    float tzmax = (max.z - r->origin.z) * r->invdir.z;

    if (tzmin > tzmax) swap_f(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    return true;
}

static void move_and_rotate_player_character(character* controlled_character, input* inp, sim_region* region, float dt)
{
    //
    controlled_character->m_ddp = {};
    camera_movement movement;
    movement = NONE;

    if (inp->buttons_pressed[BUTTONS_W])
    {
        controlled_character->m_ddp.y = -g_player_vel;
    }
    if (inp->buttons_pressed[BUTTONS_S])
    {
        controlled_character->m_ddp.y = +g_player_vel;
    }
    if (inp->buttons_pressed[BUTTONS_A])
    {
        controlled_character->m_ddp.x = -g_player_vel;
    }
    if (inp->buttons_pressed[BUTTONS_D])
    {
        controlled_character->m_ddp.x = +g_player_vel;
    }

    move_spec player_movement;
    player_movement.drag = 8.0f;
    player_movement.speed = 100.0f;
    player_movement.unit_max_accel_vector = true;

    if (inp->left_click)
    {
        ray mouse_ray = cast_ray_to_mouse_cursor(inp);
        float t;
        if (ray_vs_plane(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.f, 1.f, 0.f), &mouse_ray, &t))
        {
            glm::vec3 mouse_pos = mouse_ray.origin + mouse_ray.dir * t;

            controlled_character->m_move_target = mouse_pos;
            controlled_character->m_should_move = true;

            glm::vec3 forward = glm::normalize(controlled_character->m_rot * glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 new_forward = glm::normalize(mouse_pos - controlled_character->m_p);
            float theta = acos(glm::dot(forward, new_forward));
            glm::vec3 rot_axis = glm::normalize(glm::cross(forward, new_forward));

            if (theta > 0.01f)
            {
                glm::quat rot = glm::angleAxis(theta, rot_axis);
                glm::quat final_rot = rot * controlled_character->m_rot;
                controlled_character->m_rotate_target = final_rot;
                controlled_character->m_should_rotate = true;
            }
        }
    }

    float rotate_time = 0.2f; //seconds

    if (controlled_character->m_should_rotate)
    {
        controlled_character->m_rot = glm::slerp(controlled_character->m_rot, controlled_character->m_rotate_target, dt/rotate_time);
        //check angle
        glm::vec3 forward = glm::normalize(controlled_character->m_rot * glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 new_forward = glm::normalize(controlled_character->m_rotate_target * glm::vec3(0.0f, 0.0f, 1.0f));
        float theta = acos(glm::dot(forward, new_forward));
        if (theta < 0.1f)
        {
            controlled_character->m_should_rotate = false;
        }
    }

    if (controlled_character->m_should_move)
    {
        glm::vec3 move_dir = g_player_vel * glm::normalize(controlled_character->m_move_target - controlled_character->m_p);

        controlled_character->m_ddp.x = move_dir.x;
        controlled_character->m_ddp.y = move_dir.z;

        glm::vec3 remaining = controlled_character->m_move_target - controlled_character->m_p;

        if (glm::length(remaining) < 2.0f)
        {
            controlled_character->m_ddp *= 0.0f;
        }

        move_entity(controlled_character, region, dt, player_movement);

        if (glm::length(remaining) < 0.5f)
        {
            controlled_character->m_should_move = false;
            controlled_character->m_dp = { 0.0f,0.0f };
        }
    }
}

static void simulate_and_render_game(input* inp, character* controlled_character, float dt)
{
    //get world chunks covered by camera
    update_perspective_matrix();
    update_view_matrix();

    glm::mat4 projection = get_perspective_matrix();
    glm::mat4 view = get_view_matrix();

    sim_region region = get_chunks_to_simulate(controlled_character->m_p, controlled_character->m_collision);

    move_and_rotate_player_character(controlled_character, inp, &region, dt);

    glm::vec3 new_camera_p = controlled_character->m_p + glm::vec3(0, 10, 10);
    set_camera_position(new_camera_p);

    glClearColor(0.05f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //debug rects
    //draw_debug_rect(obstacle, g_debug_draw.s, glm::vec3(1.0f, 0.0f, 0.0f));
    draw_debug_rect(controlled_character, g_debug_draw.s, glm::vec3(1.0f, 0.0f, 0.0f));

    //play animation
    float anim_time = g_pause ? 0 : dt;

    for (uint32_t i = 0; i < region.num_chunks; ++i)
    {
        world_chunk* p_chunk = region.chunks_to_simulate[i];
        for (uint32_t j = 0; j < p_chunk->m_num_entities; ++j)
        {
            entity* p_entity = p_chunk->m_entities[j];
            use_shader(p_entity->s);
            set_mat4(p_entity->s, "projection", projection);
            set_mat4(p_entity->s, "view", view);
            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, p_entity->m_p);
            glm::mat4 rotation_m = glm::mat4_cast(p_entity->m_rot);
            model *= rotation_m;
            model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));
            set_mat4(p_entity->s, "model", model);
            float blend_factor = glm::clamp(glm::length(p_entity->m_dp) / 5.0f, 0.0f, 1.0f);
            get_bone_transforms((character*)p_entity, anim_time,0,1, blend_factor);
            set_bone_transforms((character*)p_entity);
            draw_entity(p_entity);
        }
    }
    //render entities in the world chunks
    SDL_GL_SwapWindow(g_window);
}

void load_sounds()
{
    //Load music
    g_music = Mix_LoadMUS( "Assets/Sounds/beat.wav" );
    if( g_music == NULL )
    {
        printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() );
    }
    
    //Load sound effects
    g_scratch = Mix_LoadWAV( "Assets/Sounds/scratch.wav" );
    if( g_scratch == NULL )
    {
        printf( "Failed to load scratch sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }
    
    g_high = Mix_LoadWAV( "Assets/Sounds/high.wav" );
    if( g_high == NULL )
    {
        printf( "Failed to load high sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }

    g_medium = Mix_LoadWAV( "Assets/Sounds/medium.wav" );
    if( g_medium == NULL )
    {
        printf( "Failed to load medium sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }

    g_low = Mix_LoadWAV( "Assets/Sounds/low.wav" );
    if( g_low == NULL )
    {
        printf( "Failed to load low sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }    
}

struct character_info
{
    int16_t  x;
    int16_t  y;
    uint16_t width;
    uint16_t height;
    int16_t  x_offset;
    int16_t  y_offset;
    uint16_t x_advance;
};

struct text_vertex
{
    float x, y, s, t;
};

character_info g_character_infos[128];

#define MAX_TEXT_RENDER_LEN 64

float sx = 2.0f / SCREEN_WIDTH;
float sy = 2.0f / SCREEN_HEIGHT;

void render_text(const char* text, float x, float y)
{
    g_character_infos['a'].x = 37;
    g_character_infos['a'].y = 273;
    g_character_infos['a'].width = 33;
    g_character_infos['a'].height = 38;
    g_character_infos['a'].x_offset = 0;
    g_character_infos['a'].y_offset = 23;
    g_character_infos['a'].x_advance = 34;

    text_vertex vertices[MAX_TEXT_RENDER_LEN * 6];
    uint32_t num_vertices = 0;
    for (uint32_t i = 0; i < strlen(text); ++i)
    {
        character_info info = g_character_infos[text[i]];
        
        float x2 = x + info.x * sx;
        float y2 = - y - info.y * sy;
        float w  = info.width * sx;
        float h = info.height * sy;

        x += info.x_advance * sx;
        //should advance y?
        int atlas_width = 512;
        int atlas_height = 512;

        vertices[num_vertices++] = { x2, -y2, (float)info.x/(float)atlas_width, (float)info.y/(float)atlas_height };
        vertices[num_vertices++] = { x2 + w, -y2, ((float)info.x + (float)info.width) / (float)atlas_width,  (float)info.y / (float)atlas_height };
        vertices[num_vertices++] = { x2, -y2 - h, (float)info.x + (float)info.width, ((float)info.y + (float)info.height) / (float)atlas_height };
        vertices[num_vertices++] = { x2 + w, -y2, ((float)info.x + (float)info.width) / (float)atlas_width, (float)info.y / (float)atlas_height };
        vertices[num_vertices++] = { x2, -y2 - h, (float)info.x / (float)atlas_width, ((float)info.y + (float)info.height) / (float)atlas_height };
        vertices[num_vertices++] = { x2 + w, -y2 - h, (float)info.x / (float)atlas_width, ((float)info.y + (float)info.height) / (float)atlas_height };
    }

    glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(text_vertex), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    glDisableVertexAttribArray(0);
}

int main(int argc, char* args[])
{
    if(!sdl_init())
    {
        printf( "Failed to initialize!\n" );
        return 1;
    }
    if (!game_memory_init())
    {
        printf("Can not allocate memory, quitting!\n");
        return 2;
    }

    camera_init();
    world_init();
    asset_storage_init();
    entities_init();
    characters_init();
    mesh_component_init();
    debug_draw_init();
    job_system_init();

    g_player_vel = 1.f;
    
    character _player;
    memset(&_player, 0, sizeof(character));
    _player.m_collision = {1,1};
    _player.m_p = {0, 0, 0};
    _player.m_height = 2.0f;
    _player.m_type = ET_CHARACTER;
    _player.m_rot = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
    
    character* player = create_character(&_player);
    load_model_from_file(player, "Assets/Meshes/Paladin/Sword_and_shield_idle.dae", 2);
    load_animation_from_file(player, "Assets/Meshes/Paladin/Sword_and_shield_walk.dae");
    player->s = create_default_shader();
    /*
    character _obstacle;
    memset(&_obstacle, 0, sizeof(character));
    _obstacle.m_collision = {1,1};
    _obstacle.m_p = {5, 0, 0};
    _obstacle.m_height = 2.0f;
    obstacle = create_character(&_obstacle);
    load_model_from_file(obstacle, "Assets/Meshes/Skeletal_temp/Breakdance_1990.dae", 1);
    obstacle->s = player->s;
    */

    uint32_t last_time  = SDL_GetTicks();
    uint32_t frame_time = 16;

    g_running = true;
   
    while(g_running)
    {
        fill_game_memory();
        input game_input = handle_input();
        float dt = frame_time * 0.001f; //seconds
        
        simulate_and_render_game(&game_input, player, dt);

        uint32_t current_time = SDL_GetTicks();
        frame_time = current_time - last_time;

        SDL_GL_SwapWindow(g_window);
        /*
        if(frame_time < 16)
        {
            SDL_Delay(16 - frame_time);
        }
        */
        //printf("frame time: %d\n", frame_time);
        last_time = current_time;
    }

    return 0;
}