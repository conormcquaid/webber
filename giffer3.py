#! /usr/bin/python

import sys
from PIL import Image

fname = sys.argv[1]

try:
    img = Image.open(fname)

except FileNotFoundError as fnf:
    print(f"You asked to open {fname}, but that seems to be unavailable")

except Exception as e:
    print("A bad thing was :" , e)

(width, height) = img.size

print(f"/* Image: {fname}, width {width}, height {height} */")

print(f"/* Mode is {img.mode} */")

is_binary = img.mode == '1'

#print(f"Binary {is_binary}")

#print(f"uint8_t {fname.replace('.','_')}[{ int(width * height / 8)}] = ", '{')

print('{')

has_alpha = img.has_transparency_data

for y in range(0, height>>3):
    
    for x in range (0, width):
        val = 0
        for bit in range(0,8):
        
            if is_binary:
                if img.getpixel( (x,(y*8)+bit) ) > 0:
                    val = val | (1 << bit)
            
            else: # assume mode is 'RGB' for now
                if has_alpha:
                    (r,g,b,a) = img.getpixel( (x,(y*8)+bit) )
                else:
                    (r,g,b) = img.getpixel( (x,(y*8)+bit) )
                
                
                if( (r+g+b)/3 < 128):
                    val = val | (1 << bit)
        print(f' 0x{val:02x}', end=",")
        # if x != (width-1):
        #     print(',')
    print(f"    /* Page {y} */    ")
    #print("\n")
print('},')



# for w in range(0,width):
#     for h in range(0, height):
#         (r,g,b,a) = img.getpixel((w,h))
#         if( (r+b+g)/3 < 128 ):
#             img.putpixel((w,h),(0,0,0,255))
#         else:
#             img.putpixel((w,h), (255,255,255,255))
