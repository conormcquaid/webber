struct LabColor {
  float l;
  float a;
  float b;
};

// Function to convert RGB to CIELAB
LabColor rgbToCielab(int r, int g, int b) {
  // 1. Normalize and linearize sRGB
  float R_s = r / 255.0f;
  float G_s = g / 255.0f;
  float B_s = b / 255.0f;

  float R_lin, G_lin, B_lin;
  R_lin = (R_s > 0.04045f) ? pow((R_s + 0.055f) / 1.055f, 2.4f) : R_s / 12.92f;
  G_lin = (G_s > 0.04045f) ? pow((G_s + 0.055f) / 1.055f, 2.4f) : G_s / 12.92f;
  B_lin = (B_s > 0.04045f) ? pow((B_s + 0.055f) / 1.055f, 2.4f) : B_s / 12.92f;

  // 2. Convert to XYZ (D65 white point)
  float X = (R_lin * 0.4124f + G_lin * 0.3576f + B_lin * 0.1805f) * 100.0f;
  float Y = (R_lin * 0.2126f + G_lin * 0.7152f + B_lin * 0.0722f) * 100.0f;
  float Z = (R_lin * 0.0193f + G_lin * 0.1192f + B_lin * 0.9505f) * 100.0f;

  // 3. Normalize XYZ relative to white point (D65)
  float X_n = 95.047f;
  float Y_n = 100.000f;
  float Z_n = 108.883f;

  float X_r = X / X_n;
  float Y_r = Y / Y_n;
  float Z_r = Z / Z_n;

  // 4. Apply transformation function
  auto f = [](float c) {
    if (c > 0.008856f) {
      return pow(c, 1.0f/3.0f);
    } else {
      return (7.787f * c) + (16.0f/116.0f);
    }
  };
  
  float f_Xr = f(X_r);
  float f_Yr = f(Y_r);
  float f_Zr = f(Z_r);

  // 5. Calculate L*, a*, b*
  LabColor lab;
  lab.l = (116.0f * f_Yr) - 16.0f;
  lab.a = 500.0f * (f_Xr - f_Yr);
  lab.b = 200.0f * (f_Yr - f_Zr);
  
  return lab;
}

// Example usage in setup()
void setup() {
  Serial.begin(115200);
  LabColor lab_red = rgbToCielab(255, 0, 0);
  Serial.print("Red (255,0,0) in LAB: L*=");
  Serial.print(lab_red.l);
  Serial.print(", a*=");
  Serial.print(lab_red.a);
  Serial.print(", b*=");
  Serial.println(lab_red.b);
}

void loop() {
  // put your main code here, to run repeatedly:
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
another llm opines

///////////////////////////////////////////////////////???
#include <Arduino.h>
#include <math.h>

// Convert 8-bit RGB to CIELAB (D65 white point)
void rgbToCielab(int r, int g, int b, float &l, float &a, float &b_out) {
  // Step 1: sRGB to XYZ conversion
  // Normalize RGB values to the range [0, 1]
  float R = (float)r / 255.0;
  float G = (float)g / 255.0;
  float B = (float)b / 255.0;

  // Apply gamma correction (transfer function for sRGB)
  if (R > 0.04045) R = pow((R + 0.055) / 1.055, 2.4);
  else R = R / 12.92;
  if (G > 0.04045) G = pow((G + 0.055) / 1.055, 2.4);
  else G = G / 12.92;
  if (B > 0.04045) B = pow((B + 0.055) / 1.055, 2.4);
  else B = B / 12.92;

  // Multiply by 100 for percentage
  R *= 100.0;
  G *= 100.0;
  B *= 100.0;

  // Convert linear sRGB to XYZ (using D65 white point matrix)
  float X = R * 0.4124 + G * 0.3576 + B * 0.1805;
  float Y = R * 0.2126 + G * 0.7152 + B * 0.0722;
  float Z = R * 0.0193 + G * 0.1192 + B * 0.9505;

  // Step 2: Normalize XYZ with the D65 white point
  float ref_X = 95.047;
  float ref_Y = 100.000;
  float ref_Z = 108.883;

  float x_n = X / ref_X;
  float y_n = Y / ref_Y;
  float z_n = Z / ref_Z;

  // Apply a transfer function to the normalized XYZ values
  auto f = [](float t) {
    if (t > 0.008856) return pow(t, 1.0/3.0);
    else return 7.787 * t + 16.0 / 116.0;
  };

  x_n = f(x_n);
  y_n = f(y_n);
  z_n = f(z_n);

  // Step 3: XYZ to CIELAB conversion
  l = 116.0 * y_n - 16.0;
  a = 500.0 * (x_n - y_n);
  b_out = 200.0 * (y_n - z_n);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

and back again...

#include <vector>

// Assuming you have a CIELAB to XYZ conversion function
// and an XYZ to sRGB conversion function.
struct RGB {
    double r, g, b;
};

struct L_A_B {
    double l, a, b;
};

struct XYZ {
    double x, y, z;
};

XYZ convert_lab_to_xyz(L_A_B lab) {
    // Implement complex mathematical formulas here
    // ...
    return XYZ{0, 0, 0}; // Placeholder
}

RGB convert_xyz_to_srgb(XYZ xyz) {
    // Implement complex mathematical formulas here
    // ...
    return RGB{0, 0, 0}; // Placeholder
}

// Main conversion function
RGB convert_lab_to_rgb(L_A_B lab) {
    XYZ xyz = convert_lab_to_xyz(lab);
    RGB rgb = convert_xyz_to_srgb(xyz);
    return rgb;
}


////////////////////////////////////////////////////////////////////////
from :https://discourse.mcneel.com/t/cielab-to-rgb-conversion-not-plausible/125635/6

// using http://www.easyrgb.com/index.php?X=MATH&H=01#text1
void rgb2lab( float R, float G, float B, float & l_s, float &a_s, float &b_s )
{
float var_R = R/255.0;
float var_G = G/255.0;
float var_B = B/255.0;

if ( var_R > 0.04045 ) var_R = pow( (( var_R + 0.055 ) / 1.055 ), 2.4 );
else                   var_R = var_R / 12.92;
if ( var_G > 0.04045 ) var_G = pow( ( ( var_G + 0.055 ) / 1.055 ), 2.4);
else                   var_G = var_G / 12.92;
if ( var_B > 0.04045 ) var_B = pow( ( ( var_B + 0.055 ) / 1.055 ), 2.4);
else                   var_B = var_B / 12.92;

var_R = var_R * 100.;
var_G = var_G * 100.;
var_B = var_B * 100.;

//Observer. = 2°, Illuminant = D65
float X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
float Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
float Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;


float var_X = X / 95.047 ;         //ref_X =  95.047   Observer= 2°, Illuminant= D65
float var_Y = Y / 100.000;          //ref_Y = 100.000
float var_Z = Z / 108.883;          //ref_Z = 108.883

if ( var_X > 0.008856 ) var_X = pow(var_X , ( 1./3. ) );
else                    var_X = ( 7.787 * var_X ) + ( 16. / 116. );
if ( var_Y > 0.008856 ) var_Y = pow(var_Y , ( 1./3. ));
else                    var_Y = ( 7.787 * var_Y ) + ( 16. / 116. );
if ( var_Z > 0.008856 ) var_Z = pow(var_Z , ( 1./3. ));
else                    var_Z = ( 7.787 * var_Z ) + ( 16. / 116. );

l_s = ( 116. * var_Y ) - 16.;
a_s = 500. * ( var_X - var_Y );
b_s = 200. * ( var_Y - var_Z );

}



//http://www.easyrgb.com/index.php?X=MATH&H=01#text1
void lab2rgb( float l_s, float a_s, float b_s, float& R, float& G, float& B )
{
float var_Y = ( l_s + 16. ) / 116.;
float var_X = a_s / 500. + var_Y;
float var_Z = var_Y - b_s / 200.;

if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
else                      var_Y = ( var_Y - 16. / 116. ) / 7.787;
if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
else                      var_X = ( var_X - 16. / 116. ) / 7.787;
if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
else                      var_Z = ( var_Z - 16. / 116. ) / 7.787;

float X = 95.047 * var_X ;    //ref_X =  95.047     Observer= 2°, Illuminant= D65
float Y = 100.000 * var_Y  ;   //ref_Y = 100.000
float Z = 108.883 * var_Z ;    //ref_Z = 108.883


var_X = X / 100. ;       //X from 0 to  95.047      (Observer = 2°, Illuminant = D65)
var_Y = Y / 100. ;       //Y from 0 to 100.000
var_Z = Z / 100. ;      //Z from 0 to 108.883

float var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
float var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
float var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;

if ( var_R > 0.0031308 ) var_R = 1.055 * pow(var_R , ( 1 / 2.4 ))  - 0.055;
else                     var_R = 12.92 * var_R;
if ( var_G > 0.0031308 ) var_G = 1.055 * pow(var_G , ( 1 / 2.4 ) )  - 0.055;
else                     var_G = 12.92 * var_G;
if ( var_B > 0.0031308 ) var_B = 1.055 * pow( var_B , ( 1 / 2.4 ) ) - 0.055;
else                     var_B = 12.92 * var_B;

R = var_R * 255.;
G = var_G * 255.;
B = var_B * 255.;

}


//// much more @ https://www.easyrgb.com/en/math.php