#include "flash_storage.h"
#include "app_utils.h"
#include "nrfx_nvmc.h"

#define ERASED_WORD   0xFFFFFFFFUL
#define RECORD_COUNT  (FLASH_STORAGE_PAGE_SIZE / sizeof(record_t))

typedef struct
{
    uint32_t data;
    uint32_t magic;
} record_t;

static const record_t * const m_records = (const record_t *)FLASH_STORAGE_PAGE_ADDR;

static uint32_t hsv_pack(hsv_t hsv)
{
    return ((uint32_t)hsv.h << 16) | ((uint32_t)hsv.s << 8) | hsv.v;
}

static hsv_t hsv_unpack(uint32_t data)
{
    hsv_t hsv;

    hsv.h = (uint16_t)(data >> 16);
    hsv.s = (uint8_t)(data >> 8);
    hsv.v = (uint8_t)data;
    return hsv;
}

static bool hsv_is_valid(hsv_t hsv)
{
    return hsv.h < HUE_MAX && hsv.s <= SATURATION_MAX && hsv.v <= BRIGHTNESS_MAX;
}

static bool record_is_free(uint32_t index)
{
    return m_records[index].data == ERASED_WORD && m_records[index].magic == ERASED_WORD;
}

static uint32_t free_index_find(void)
{
    uint32_t index = 0;

    while (index < RECORD_COUNT && !record_is_free(index))
    {
        index++;
    }
    return index;
}

static bool last_record_find(hsv_t * p_hsv)
{
    bool found = false;
    uint32_t end = free_index_find();

    for (uint32_t i = 0; i < end; i++)
    {
        if (m_records[i].magic != FLASH_STORAGE_MAGIC) continue;

        hsv_t hsv = hsv_unpack(m_records[i].data);
        if (hsv_is_valid(hsv))
        {
            *p_hsv = hsv;
            found = true;
        }
    }
    return found;
}

bool flash_storage_load(hsv_t * p_hsv)
{
    return last_record_find(p_hsv);
}

bool flash_storage_save(hsv_t hsv)
{
    hsv_t stored;

    if (last_record_find(&stored) && hsv_pack(stored) == hsv_pack(hsv)) return false;

    uint32_t index = free_index_find();

    if (index >= RECORD_COUNT)
    {
        app_error_check(nrfx_nvmc_page_erase(FLASH_STORAGE_PAGE_ADDR));
        index = 0;
    }

    uint32_t addr = (uint32_t)&m_records[index];

    nrfx_nvmc_word_write(addr, hsv_pack(hsv));
    while (!nrfx_nvmc_write_done_check()) {}

    nrfx_nvmc_word_write(addr + sizeof(uint32_t), FLASH_STORAGE_MAGIC);
    while (!nrfx_nvmc_write_done_check()) {}

    return true;
}


void flash_storage_erase(void)
{
    app_error_check(nrfx_nvmc_page_erase(FLASH_STORAGE_PAGE_ADDR));
}
