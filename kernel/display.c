#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "cursor.h"
#include "file.h"
#include "proc.h"
#include "stat.h"
#include "display.h"
#include "colors.h"

struct cursor dcursor;
struct frame dframe;

void
init_cursor()
{
  initsleeplock(&dcursor.lock, "cursor");
  dcursor.xpos = FRAME_WIDTH / 2;
  dcursor.ypos = FRAME_HEIGHT / 2;
  dcursor.height = CURSOR_HEIGHT;
  dcursor.width = CURSOR_WIDTH;
  dcursor.frame_buf = kdisplaymem();
  memmove(dcursor.frame_buf, cursor_frame, CURSOR_WIDTH*CURSOR_HEIGHT*4);
  printf("cursor initialized\n");

  /*uint32 *pix;*/
  /*for(int y = 0; y < CURSOR_HEIGHT; y++){*/
    /*for(int x = 0; x < CURSOR_WIDTH; x++){*/
      /*pix = dcursor.frame_buf + (y * CURSOR_WIDTH) + x;*/
      /*int r = (*pix & 0xff000000) >> 24;*/
      /*int g = (*pix & 0x00ff0000) >> 16;*/
      /*int b = (*pix & 0x0000ff00) >> 8;*/
      /*int a = (*pix & 0x000000ff);*/
      /*printf("{%d,%d,%d,%d}", r, g, b, a);*/
    /*}*/
  /*}*/
}

void
update_cursor_rel(int xrel, int yrel)
{
  acquiresleep(&dcursor.lock);
  if(dcursor.xpos + xrel < 0){
    dcursor.xpos = 0;
  } else if(dcursor.xpos + xrel >= dframe.width){
    dcursor.xpos = dframe.width - 1;
  } else {
    dcursor.xpos += xrel;
  }

  if(dcursor.ypos + yrel < 0){
    dcursor.ypos = 0;
  } else if(dcursor.ypos + yrel >= dframe.height){
    dcursor.ypos = dframe.height - 1;
  } else {
    dcursor.ypos += yrel;
  }
  releasesleep(&dcursor.lock);
}

void
update_cursor_abs(int xabs, int yabs)
{
  acquiresleep(&dcursor.lock);
  if(xabs < 0){
    dcursor.xpos = 0;
  } else if(xabs >= dframe.width){
    dcursor.xpos = dframe.width - 1;
  } else {
    dcursor.xpos = xabs;
  }

  if(yabs < 0){
    dcursor.ypos = 0;
  } else if(yabs >= dframe.height){
    dcursor.ypos = dframe.height - 1;
  } else {
    dcursor.ypos = yabs;
  }
  releasesleep(&dcursor.lock);
}

void init_frame()
{
  initsleeplock(&dframe.lock, "frame");
  dframe.frame_buf = kdisplaymem() + CURSOR_DATA_SIZE;
  dframe.height = FRAME_HEIGHT;
  dframe.width = FRAME_WIDTH;
  memset(dframe.frame_buf, 0, sizeof(uint8) * FRAME_DATA_SIZE);
}

void*
get_frame_buf()
{
  return (void*) dframe.frame_buf;
}

void*
get_cursor_frame_buf()
{
  return (void*) dcursor.frame_buf;
void
set_pixel(int x, int y, uint32 color, uint8 alpha)
{
  uint32 rgba = (color << 8) & alpha;
  if((y >= 0 && y < FRAME_HEIGHT) && (x >= 0 && x ))
    *(dframe.frame_buf + y * FRAME_WIDTH + x) = rgba;
}

void
draw_rect(int xpos, int ypos, int width, int height, uint32 color, uint8 alpha)
{
  for(int y = ypos; y < ypos + height; y++){
    for(int x = xpos; x < xpos + height; x++){
      set_pixel(x, y, color, alpha);
    }
  }
}
