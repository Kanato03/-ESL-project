#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>

#define HUE_MAX        360
#define SATURATION_MAX 100
#define BRIGHTNESS_MAX 100
#define RGB_MAX        255

typedef struct
{
    uint16_t h;
    uint8_t  s;
    uint8_t  v;
} hsv_t;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

rgb_t color_hsv_to_rgb(hsv_t hsv);

#endif
