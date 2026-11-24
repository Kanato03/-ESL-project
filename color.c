#include "color.h"

rgb_t color_hsv_to_rgb(hsv_t hsv)
{
    uint32_t h = hsv.h % HUE_MAX;
    uint32_t s = hsv.s;
    uint32_t v = (uint32_t)hsv.v * RGB_MAX / BRIGHTNESS_MAX;

    uint32_t sector = h / 60;
    uint32_t rest   = h % 60;

    uint32_t p = v * (SATURATION_MAX - s) / SATURATION_MAX;
    uint32_t q = v * (SATURATION_MAX - s * rest / 60) / SATURATION_MAX;
    uint32_t t = v * (SATURATION_MAX - s * (60 - rest) / 60) / SATURATION_MAX;

    rgb_t rgb;

    switch (sector)
    {
        case 0:  rgb = (rgb_t){ v, t, p }; break;
        case 1:  rgb = (rgb_t){ q, v, p }; break;
        case 2:  rgb = (rgb_t){ p, v, t }; break;
        case 3:  rgb = (rgb_t){ p, q, v }; break;
        case 4:  rgb = (rgb_t){ t, p, v }; break;
        default: rgb = (rgb_t){ v, p, q }; break;
    }

    return rgb;
}
