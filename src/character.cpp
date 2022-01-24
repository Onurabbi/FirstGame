#include "character.h"
#include "world.h"

static character* m_character_storage;
static uint32_t   m_num_characters;

static void copy_character_info(character* to, character* from)
{
	to->m_p         = from->m_p;
	to->m_height    = from->m_height;
	to->m_collision = from->m_collision;
	to->m_type      = from->m_type;
	to->m_rot       = from->m_rot;
}

void characters_init(void)
{
	m_character_storage = (character*)push_size((MAX_NUM_CHARACTERS) * sizeof(character));
	m_num_characters = 0;
}

character* put_character_in_storage(character* p_character)
{
	character* result = NULL;
	//check if space exists
	if (m_num_characters == MAX_NUM_CHARACTERS)
	{
		return result;
	}

	result = m_character_storage + m_num_characters++;
	return result;
}

character* create_character(character* p_character)
{
	character* result = NULL;
	
	result = (character*)map_entity_to_world_chunk(p_character);
	memcpy(result, p_character, sizeof(character));
	result->m_id   = get_next_unique_entity_id();
	result->m_final_transformations = (glm::mat4*)push_size(sizeof(glm::mat4) * MAX_NUM_BONES);
	result->m_local_transformations = (glm::mat4*)push_size(sizeof(glm::mat4) * MAX_NUM_BONES);
	result->m_num_transformations = 0;

	return result;
}

character* get_character_by_index(uint32_t index)
{
	character* result = NULL;
	result = m_character_storage + index;
	return result;
}