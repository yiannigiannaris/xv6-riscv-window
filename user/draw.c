#include "kernel/types.h"
#include "user/user.h"
#include "user/draw.h"

int
abs(int x)
{
  if(x >= 0)
    return x;
  return -x;
}

void
set_pixel(uint32 *fb, int frame_width, int frame_height, int x, int y, uint32 color, uint32 alpha)
{
  uint32 rgba = (color << 8) | alpha;
  if((y >= 0 && y < frame_height) && (x >= 0 && x ))
    *(fb + y * frame_width + x) = rgba;
}

void
draw_rect(uint32 *fb, int frame_width, int frame_height, int xpos, int ypos, int width, int height, uint32 color, uint8 alpha)
{
  for(int y = ypos; y < ypos + height; y++){
    for(int x = xpos; x < xpos + width; x++){
      set_pixel(fb, frame_width, frame_height, x, y, color, alpha);
    }
  }
}

void 
draw_line(uint32 *fb, int frame_width, int frame_height, int x0, int y0, int x1, int y1, uint32 color, uint8 alpha) 
{
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
 
  for(;;){
    set_pixel(fb, frame_width, frame_height, y0, x0, color, alpha);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

void 
circle_helper(uint32 *fb, int frame_width, int frame_height, int xcenter, int ycenter, int x, int y, uint32 color, uint8 alpha) 
{
  draw_line(fb, frame_width, frame_height, xcenter+x, ycenter+y, xcenter-x, ycenter+y, color, alpha);
  draw_line(fb, frame_width, frame_height, xcenter+x, ycenter-y, xcenter-x, ycenter-y, color, alpha);
  draw_line(fb, frame_width, frame_height, xcenter+y, ycenter+x, xcenter-y, ycenter+x, color, alpha);
  draw_line(fb, frame_width, frame_height, xcenter+y, ycenter-x, xcenter-y, ycenter-x, color, alpha);
} 
  
void 
draw_circle(uint32 *fb, int frame_width, int frame_height, int xcenter, int ycenter, int r, uint32 color, uint8 alpha) 
{ 
  int x = 0, y = r; 
  int d = 3 - 2 * r; 
  circle_helper(fb, frame_width, frame_height, xcenter, ycenter, x, y, color, alpha); 
  while (y >= x) 
  { 
    x++; 
    if (d > 0) 
    { 
      y--;  
      d = d + 4 * (x - y) + 10; 
    } 
    else
      d = d + 4 * x + 6; 
    circle_helper(fb, frame_width, frame_height, xcenter, ycenter, x, y, color, alpha); 
  } 
}

void draw_font_helper(uint32 *fb, int frame_width, int frame_height, int places, int cx, int cy, int width, int height, uint64* bitmap, uint32 color, uint8 alpha)
{
  uint64 w = 1 << (4*places); 
  int x = cx;
  int y = cy;
  uint64 *b = bitmap;
  for(; bitmap < b + height; bitmap++){
    for(int i = 0; i < width; i++){
      if(*bitmap & w){
        set_pixel(fb, frame_width, frame_height, x, y, color, alpha); 
      }
      w >>= 1;
      x++;
    }
    w = 1 << (4*places);
    x = cx;
    y++;
  }
}

void draw_font(uint32 *fb, int frame_width, int frame_height, int x, int y, uint64 ***fonts, char * c, int fontsize, int n, uint32 color, uint8 alpha)
{
  int cx, cy, dwidth, bbxw, bbxh, bbox, bboy, places;
  uint64 *dimensions;
  uint64 *bitmap;
  char *cstart = c;
  for(; c < cstart + n; c++){
    dimensions = fonts[fontsize][(uint8)*c];
    dwidth = (int)dimensions[2];
    bbxw = (int)dimensions[4];
    bbxh = (int)dimensions[5];
    bbox = (int)dimensions[6];
    bboy = (int)dimensions[7];
    places = (int)dimensions[8];
    bitmap = (uint64*)dimensions[9];
    cx = x + bbox;
    cy = y + bboy;
    draw_font_helper(fb, frame_width, frame_height, places, cx, cy, bbxw, bbxh, bitmap, color, alpha);
    x += dwidth;
  }
}


