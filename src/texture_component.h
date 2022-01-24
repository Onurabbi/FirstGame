#ifndef TEXT_COMP_H
#define TEXT_COMP_H

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

#include "entity.h"
#include "math.h"
#include "memory.h"
#include "hash.h"
#include "asset.h"
#include "common.h"

#define MAX_NUM_TEXTURE_COMPONENTS 1024

struct old_camera
{
    v2f world_pos;
    v2f size;
};

struct texture_rendering_info
{
    camera* p_camera;
    SDL_Renderer* p_renderer;
    uint32_t screen_width;
    uint32_t screen_height;
};

struct sdl_texture
{
    SDL_Texture* texture;
    entity* p_entity;
    uint32_t width, height;
    float scale;
};

void         destroy_texture_component(entity* p_entity);
sdl_texture* create_texture_component(entity* p_entity, const char* path, float scale);
void         texture_component_init(texture_rendering_info* p_info);
v2i          world_pos_to_pixels(v2f world_pos);
bool         load_texture_from_file(sdl_texture* p_texture, const char* path);
void         render_sdl_texture(sdl_texture* texture, int x, int y, float scale, 
                               SDL_Rect* clip, float angle, SDL_Point* center, SDL_RendererFlip flip);
void render_entity(entity* p_entity);
#endif