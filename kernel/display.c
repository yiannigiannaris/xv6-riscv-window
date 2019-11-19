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

struct cursor dcursor;
struct frame dframe;

void
init_cursor()
{
  dcursor.xpos = FRAME_WIDTH / 2;
  dcursor.ypos = FRAME_HEIGHT / 2;
  dcursor.height = CURSOR_HEIGHT;
  dcursor.width = CURSOR_WIDTH;
  dcursor.frame_buf = kdisplaymem();
  memmove(dcursor.frame_buf, cursor_frame, CURSOR_WIDTH*CURSOR_HEIGHT*4);
  printf("cursor initialized\n");
}

void
update_cursor_rel(int yrel, int xrel)
{
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
}

void
update_cursor_abs(int yabs, int xabs)
{
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
}

void init_frame()
{
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
}
