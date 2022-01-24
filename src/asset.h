#ifndef ASSET_H
#define ASSET_H

#define MAX_NUM_ASSETS        1024
#define MAX_ASSET_PATH_LENGTH 256

#include <string.h>

#include "memory.h"
#include "hash.h"

typedef enum
{
    ASSET_TYPE_MESH,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_INVALID,
    NUM_ASSET_TYPES
}asset_type;

struct asset_tag
{
    asset_type type;
    void*      data;
    uint32_t   size;//size of the asset in bytes     
};

void      asset_storage_init();
asset_tag find_asset_in_storage(const char* asset_path, asset_tag tag);

#endif
