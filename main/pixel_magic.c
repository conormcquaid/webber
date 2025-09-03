#include "stdint.h"
#include "math.h"
#include "main.h"

RGBColor hslToRgb(float h, float s, float l) {
    float r, g, b;
    float c, x, m;

    h = fmod(h, 360.0f); // Ensure hue is within 0-360
    if (h < 0) {
        h += 360.0f;
    }
    s /= 100.0f;
    l /= 100.0f;

    c = (1.0f - fabs(2.0f * l - 1.0f)) * s;
    x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    m = l - c / 2.0f;

    if (h >= 0.0f && h < 60.0f) {
        r = c, g = x, b = 0.0f;
    } else if (h >= 60.0f && h < 120.0f) {
        r = x, g = c, b = 0.0f;
    } else if (h >= 120.0f && h < 180.0f) {
        r = 0.0f, g = c, b = x;
    } else if (h >= 180.0f && h < 240.0f) {
        r = 0.0f, g = x, b = c;
    } else if (h >= 240.0f && h < 300.0f) {
        r = x, g = 0.0f, b = c;
    } else {
        r = c, g = 0.0f, b = x;
    }

    RGBColor color;
    color.r = (int)((r + m) * 255.0f);
    color.g = (int)((g + m) * 255.0f);
    color.b = (int)((b + m) * 255.0f);

    return color;
}


void gamma_bright_correct(uint8_t* r, uint8_t* g, uint8_t* b, float bright){
    
    float R = (float)*r / 255.0;
    float G = (float)*g / 255.0;
    float B = (float)*b / 255.0;

    // Apply gamma correction (transfer function for sRGB)
    if (R > 0.04045) R = pow((R + 0.055) / 1.055, 2.4);
    else R = R / 12.92;
    if (G > 0.04045) G = pow((G + 0.055) / 1.055, 2.4);
    else G = G / 12.92;
    if (B > 0.04045) B = pow((B + 0.055) / 1.055, 2.4);
    else B = B / 12.92;

  
    R = R * 255.0 * bright;
    G = G * 255.0 * bright;
    B = B * 255.0 * bright;

    *r = (uint8_t)R;
    *g = (uint8_t)G;
    *b = (uint8_t)B;
}