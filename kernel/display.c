#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "fs.h"
#include "cursor.h"
#include "proc.h"
#include "stat.h"
#include "display.h"
#include "colors.h"
#include "wallpaper.h"

struct frame dframe;

void print_frame();
void draw_window(int xpos, int ypos, int width, int height);


void
draw_wallpaper()
{
  memmove(dframe.frame_buf, wallpaper_frame, FRAME_WIDTH*FRAME_HEIGHT*4);
}

void
send_frame_update()
{
  update_frame_buffer((uint64)dframe.frame_buf, 0);
}

void init_frame()
{
  initlock(&dframe.lock, "frame");
  dframe.frame_buf = kdisplaymem() + CURSOR_DATA_SIZE;
  dframe.height = FRAME_HEIGHT;
  dframe.width = FRAME_WIDTH;
  memset(dframe.frame_buf, 0, sizeof(char) * FRAME_DATA_SIZE);
  draw_wallpaper();
  initialize_display((uint64)dframe.frame_buf);
  update_frame_buffer((uint64)dframe.frame_buf, 0);
}

void
set_pixel_hex(int x, int y, uint32 rgba)
{
  if((y >= 0 && y < FRAME_HEIGHT) && (x >= 0 && x ))
    *(dframe.frame_buf + y * FRAME_WIDTH + x) = rgba;
}

void
set_pixel(int x, int y, uint32 color, uint32 alpha)
{
  uint32 rgba = (color << 8) | alpha;
  if((y >= 0 && y < FRAME_HEIGHT) && (x >= 0 && x ))
    *(dframe.frame_buf + y * FRAME_WIDTH + x) = rgba;
}

void
draw_rect(int xpos, int ypos, int width, int height, uint32 color, uint8 alpha)
{
  for(int y = ypos; y < ypos + height; y++){
    for(int x = xpos; x < xpos + width; x++){
      set_pixel(x, y, color, alpha);
    }
  }
}

int
abs(int x)
{
  if(x >= 0)
    return x;
  return -x;
}

void 
draw_line(int x0, int y0, int x1, int y1, uint32 color, uint8 alpha) 
{
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
 
  for(;;){
    set_pixel(y0, x0, color, alpha);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

/*void draw_line(int x1, int y1, int x2, int y2, uint32 color, uint8 alpha)*/
/*{*/
  /*int m_new = 2 * (y2 - y1);*/
  /*int slope_error_new = m_new - (x2 - x1);*/
  /*for (int x = x1, y = y1; x <= x2; x++)*/
  /*{*/
    /*set_pixel(x, y, color, alpha);*/
    /*slope_error_new += m_new;*/
    /*if (slope_error_new >= 0)*/
    /*{*/
      /*y++;*/
      /*slope_error_new  -= 2 * (x2 - x1);*/
    /*}*/
  /*}*/
/*}*/

void 
circle_helper(int xcenter, int ycenter, int x, int y, uint32 color, uint8 alpha) 
{
  draw_line(xcenter+x, ycenter+y, xcenter-x, ycenter+y, color, alpha);
  draw_line(xcenter+x, ycenter-y, xcenter-x, ycenter-y, color, alpha);
  draw_line(xcenter+y, ycenter+x, xcenter-y, ycenter+x, color, alpha);
  draw_line(xcenter+y, ycenter-x, xcenter-y, ycenter-x, color, alpha);
  /*set_pixel(xcenter+x, ycenter+y, color, alpha);*/
  /*set_pixel(xcenter-x, ycenter+y, color, alpha);*/
  /*set_pixel(xcenter+x, ycenter-y, color, alpha);*/
  /*set_pixel(xcenter-x, ycenter-y, color, alpha);*/
  /*set_pixel(xcenter+y, ycenter+x, color, alpha);*/
  /*set_pixel(xcenter-y, ycenter+x, color, alpha);*/
  /*set_pixel(xcenter+y, ycenter-x, color, alpha);*/
  /*set_pixel(xcenter-y, ycenter-x, color, alpha);*/
} 
  
void 
draw_circle(int xcenter, int ycenter, int r, uint32 color, uint8 alpha) 
{ 
  int x = 0, y = r; 
  int d = 3 - 2 * r; 
  circle_helper(xcenter, ycenter, x, y, color, alpha); 
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
    circle_helper(xcenter, ycenter, x, y, color, alpha); 
  } 
}

void
draw_window(int xpos, int ypos, int width, int height)
{
  if(width > FRAME_WIDTH || height > FRAME_HEIGHT){
    printf("DRAW_WINDOW: window too big\n");
    return;
  }
  if(xpos < 0)
    xpos = 0;
  if(xpos + width > FRAME_WIDTH)
    xpos = FRAME_WIDTH - width;
  if(ypos < 0)
    ypos = 0;
  if(ypos + width > FRAME_HEIGHT)
    ypos = FRAME_HEIGHT - height;
  
  draw_rect(xpos, ypos, width, height, C_WINDOW_GRAY, 255);
  draw_rect(xpos+4, ypos+4, width-8, height-8, C_AQUA, 255);
  draw_rect(xpos, ypos, width, 30, C_WINDOW_GRAY, 255);
  draw_rect(xpos+4, ypos+4, 22, 22, C_BLACK, 255);
  draw_rect(xpos+6, ypos+6, 18, 18, C_WINDOW_RED, 255);
}

void
print_frame()
{
  for(int y = 0; y < FRAME_HEIGHT; y++){
    for(int x = 0; x < FRAME_WIDTH; x++){
      printf("%d,", *(dframe.frame_buf + (y * FRAME_WIDTH) + x));
    }
  }
  printf("\n\n\n");
}

void
reset_frame()
{
  memset(dframe.frame_buf, 0, sizeof(char) * FRAME_DATA_SIZE);
}

/*void*/
/*display_test(int n)*/
/*{*/
  /*printf("display test\n");*/
  /*[>draw_rect(500, 500, 100, 100, C_MAROON, 255);<]*/
  /*[>draw_circle(500, 500, 100, C_AQUA, 255);<]*/
  /*[>draw_line(20, 20, 600, 600, C_YELLOW, 255);<]*/
  /*[>draw_line(20, 20, 600, 20, C_BLUE, 255);<]*/
  /*[>draw_line(600, 605, 20, 25, C_GREEN, 255);<]*/
  /*[>draw_line(605, 25, 25, 25, C_PURPLE, 255);<]*/
  /*[>print_frame();<]*/
  /*draw_window(200, 200, 400, 500);*/
  /*draw_window(100, 300, 400, 200);*/
  /*update_screen(n);*/
  /*int x = 0;*/
  /*while(1){*/
    /*reset_frame();*/
    /*draw_rect(x % FRAME_WIDTH, 480, 50, 50, C_AQUA, 255);*/
    /*update_screen(n);*/
    /*x+=15;*/
  /*}*/
/*}*/


