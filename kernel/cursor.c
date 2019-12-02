#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "cursor.h"
#include "proc.h"
#include "stat.h"
#include "cursor_frame.h"

struct cursor dcursor;

void
init_cursor()
{
  initlock(&dcursor.lock, "cursor");
  dcursor.xpos = CURSOR_START_X;
  dcursor.ypos = CURSOR_START_Y;
  dcursor.height = CURSOR_HEIGHT;
  dcursor.width = CURSOR_WIDTH;
  dcursor.frame_buf = kdisplaymem();
  dcursor.left_pressed = 0;
  dcursor.right_pressed = 0;
  memmove(dcursor.frame_buf, cursor_frame, CURSOR_WIDTH*CURSOR_HEIGHT*4);
  initialize_cursor();
  update_cursor(CURSOR_START_X, CURSOR_START_Y, (uint64)dcursor.frame_buf, 0);
  printf("cursor initialized\n");
}

void
get_cursor_pos(int *xpos, int *ypos)
{
  acquire(&dcursor.lock);
  *xpos = dcursor.xpos;
  *ypos = dcursor.ypos;
  release(&dcursor.lock);
}

void
update_cursor_rel(int xrel, int yrel)
{
  acquire(&dcursor.lock);
  if(dcursor.xpos + xrel < 0){
    dcursor.xpos = 0;
  } else if(dcursor.xpos + xrel >= FRAME_WIDTH){
    dcursor.xpos = FRAME_WIDTH - 1;
  } else {
    dcursor.xpos += xrel;
  }

  if(dcursor.ypos + yrel < 0){
    dcursor.ypos = 0;
  } else if(dcursor.ypos + yrel >= FRAME_HEIGHT){
    dcursor.ypos = FRAME_HEIGHT - 1;
  } else {
    dcursor.ypos += yrel;
  }
  release(&dcursor.lock);
}

void
update_cursor_abs(int xabs, int yabs)
{
  acquire(&dcursor.lock);
  if(xabs < 0){
    dcursor.xpos = 0;
  } else if(xabs >= FRAME_WIDTH){
    dcursor.xpos = FRAME_WIDTH - 1;
  } else {
    dcursor.xpos = xabs;
  }

  if(yabs < 0){
    dcursor.ypos = 0;
  } else if(yabs >= FRAME_HEIGHT){
    dcursor.ypos = FRAME_HEIGHT - 1;
  } else {
    dcursor.ypos = yabs;
  }
  release(&dcursor.lock);
}

void
send_cursor_update()
{
  move_cursor(dcursor.xpos, dcursor.ypos, 0);
}

void
left_click_press()
{
  acquire(&dcursor.lock);
  dcursor.left_pressed = 1;
  release(&dcursor.lock);
}

void
left_click_release()
{
  acquire(&dcursor.lock);
  dcursor.left_pressed = 0;
  release(&dcursor.lock);
}


