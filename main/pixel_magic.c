#include "stdint.h"
#include "math.h"
  
void gamma_correct(uint8_t* r, uint8_t* g, uint8_t* b){
    
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

  
    R = R * 255.0;
    G = G * 255.0;
    B = B * 255.0;

    *r = (uint8_t)R;
    *g = (uint8_t)G;
    *b = (uint8_t)B;
}