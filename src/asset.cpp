#include "asset.h"


/*
    I'll allocate 1 large memory buffer for all asset paths. I will limit asset paths to 256 bytes for now.
    So, a new string starts every 256 bytes. first string starts at byte 0, second at byte 256 and so on.
*/
static char*      asset_storage;
static asset_tag* asset_tags;
static uint32_t   num_assets;

void asset_storage_init()
{
    num_assets = 0;
    uint64_t storage_size = MAX_NUM_ASSETS * MAX_ASSET_PATH_LENGTH;

    asset_storage = (char*)push_size(storage_size);
    asset_tags    = (asset_tag*)push_size(MAX_NUM_ASSETS*sizeof(asset_tag));

    memset(asset_storage, 0, storage_size);
    memset(asset_tags, 0, MAX_NUM_ASSETS*sizeof(asset_tag));
}

/*
    searches the asset storage for the asset. If not found, adds the asset to storage, and return the same index
    otherwise, returns the index of the component which has already loaded the data (for data sharing).
*/
asset_tag find_asset_in_storage(const char* asset_path, asset_tag tag)
{
    if(num_assets == MAX_NUM_ASSETS)
    {
        return asset_tag{ASSET_TYPE_INVALID, 0, 0};
    }

    //first get the index of the path
    uint32_t index = murmur3_32((const uint8_t*)asset_path, strlen(asset_path), SEED) % MAX_NUM_ASSETS;
    index *= MAX_ASSET_PATH_LENGTH;

    while(asset_storage[index] != 0)
    {
        if(strcmp(asset_storage + index, asset_path) == 0)
        {
            //same type and same index
            return asset_tags[index/MAX_ASSET_PATH_LENGTH];
        }
        index = (index + MAX_ASSET_PATH_LENGTH) % MAX_NUM_ASSETS;
    }

    //found the correct slot
    strcpy(asset_storage + index, asset_path);
    asset_tags[index/MAX_ASSET_PATH_LENGTH] = tag;
    
    return tag;
}