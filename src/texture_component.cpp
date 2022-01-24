#include "texture_component.h"

static camera* p_camera;
static SDL_Renderer* p_renderer;
static uint32_t screen_width;
static uint32_t screen_height;
static sdl_texture* textures;
static uint32_t num_texture_components;

void texture_component_init(texture_rendering_info* p_info)
{
    p_camera = p_info->p_camera;
    p_renderer = p_info->p_renderer;
    screen_width = p_info->screen_width;
    screen_height = p_info->screen_height;
    num_texture_components = 0;
    textures = (sdl_texture*)push_size(MAX_NUM_TEXTURE_COMPONENTS * sizeof(sdl_texture));
}

void destroy_texture_component(entity* p_entity)
{
    //if the component deleted is not the last
    sdl_texture* p_texture    = p_entity->m_texture;
    sdl_texture* last_texture = textures + (num_texture_components - 1);
    entity*      last_entity  = last_texture->p_entity;
    //modify the last entity to point to correct texture
    last_entity->m_texture = p_texture;
    //replace the deleted component with the last one
    memcpy(p_texture, last_texture, sizeof(sdl_texture));
    
    num_texture_components--;
}


v2i world_pos_to_pixels(v2f world_pos)
{
    float x_diff = (float)(world_pos.x - p_camera->world_pos.x)/p_camera->size.x;
    float y_diff = (float)(world_pos.y - p_camera->world_pos.y)/p_camera->size.y;

    int32_t screen_x = (int32_t)(screen_width / 2 + screen_width*x_diff);
    int32_t screen_y = (int32_t)(screen_height / 2 + screen_height*y_diff);

    v2i result = {screen_x, screen_y};
    return result;
}

bool load_texture_from_file(sdl_texture* p_texture, const char* path)
{
    SDL_Surface* loaded_surface = IMG_Load(path);
    SDL_Texture* loaded_texture = NULL;

    if(loaded_surface == NULL)
    {
        printf("Unable to load image %s!, SDL_image Error: %s\n", path, IMG_GetError());
        return false;
    }
    SDL_SetColorKey(loaded_surface, SDL_TRUE, SDL_MapRGB(loaded_surface->format, 0, 0xFF, 0xFF));

    loaded_texture = SDL_CreateTextureFromSurface(p_renderer, loaded_surface);
    if(loaded_texture == NULL)
    {
        printf( "Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError() );
        return false;
    }

    p_texture->texture = loaded_texture;
    p_texture->width   = loaded_surface->w;
    p_texture->height  = loaded_surface->h;
    
    SDL_FreeSurface(loaded_surface);

    return true;
}

void render_entity(entity* p_entity)
{
    sdl_texture* p_texture = p_entity->m_texture;
    v2f p = p_entity->p;
    v2i screen_pos = world_pos_to_pixels(p);    
    return render_sdl_texture(p_texture, screen_pos.x, screen_pos.y, p_texture->scale, NULL, 0, NULL, SDL_FLIP_NONE);
}

void render_sdl_texture(sdl_texture* texture, int x, int y, float scale, SDL_Rect* clip, float angle, SDL_Point* center, SDL_RendererFlip flip)
{
    int32_t width = texture->width*scale;
    int32_t height = texture->height*scale;

    SDL_Rect render_quad = {x - width / 2, y - height / 2, width, height};

    if(clip != NULL)
    {
        render_quad.w = clip->w;
        render_quad.h = clip->h;
    }

    SDL_RenderCopyEx(p_renderer, texture->texture, clip, &render_quad, angle, center, flip);

}

