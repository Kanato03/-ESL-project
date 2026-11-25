#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdbool.h>
#include "color.h"

#define FLASH_STORAGE_PAGE_ADDR  0xDF000
#define FLASH_STORAGE_PAGE_SIZE  0x1000
#define FLASH_STORAGE_MAGIC      0xC0105EEDUL

bool flash_storage_load(hsv_t * p_hsv);
bool flash_storage_save(hsv_t hsv);
void flash_storage_erase(void);

#endif
