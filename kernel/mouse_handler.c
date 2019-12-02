#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "display.h"
#include "colors.h"
#include "windows.h"
#include "mouse_handler.h"

#define MAX_EVENTS 2000

struct spinlock startlock, endlock;
struct mouse_event events[MAX_EVENTS];
int startidx, endidx;
int running;

void
init_mouse_handler()
{
  initlock(&startlock, "mouse handler start");
  initlock(&endlock, "mouse handler end");
  startidx = 0;
  endidx = 0;
  running = 0;
  printf("mouse handler initialized\n");
}

void
add_mouse_handler_event(int type, int xval, int yval)
{
  acquire(&endlock);
  if(endidx - startidx > MAX_EVENTS)
    printf("WARNING: MOUSE HANDLER QUEUE FULL\n");
  events[endidx % MAX_EVENTS].type = type;
  events[endidx % MAX_EVENTS].xval = xval;
  events[endidx % MAX_EVENTS].yval = yval;
  endidx++;
  if(running)
    wakeup(&endidx);
  release(&endlock);
}

int
process_event()
{
  if((startidx % MAX_EVENTS) == (endidx % MAX_EVENTS))
    return -1; 
  int ret = (startidx % MAX_EVENTS); 
  startidx++;
  return ret;
}

void
handle_rel(int xrel, int yrel)
{

}

void
handle_syn(int xpos, int ypos)
{
  handle_cursor_move(xpos, ypos);
}

void
handle_key(int type, int xpos, int ypos)
{
  switch(type){
    case MOUSE_LEFT_CLICK_PRESS:
      handle_left_click_press(xpos, ypos);
      break;
    case MOUSE_LEFT_CLICK_RELEASE:
      handle_left_click_release(xpos, ypos);
      break;
    case MOUSE_RIGHT_CLICK_PRESS:
      break;
    case MOUSE_RIGHT_CLICK_RELEASE:
      break;
    default:
      printf("mouse_handler: handle_key event type not recognized\n");
  }
}

void
handle_event(int type, int xval, int yval)
{
  switch(type){
    case MOUSE_SYN:
      handle_syn(xval, yval);
      break;
    case MOUSE_REL:
      handle_rel(xval, yval);
      break;
    default:
      handle_key(type, xval, yval);
  }
}

void
start_mouse_handler()
{
  if(running)
    panic("mouse handler already running\n");
  running = 1;
  int idx;
  int type, xval, yval;
  acquire(&startlock);
  while(1){
    while((idx = process_event()) >= 0){
      type = events[idx].type;
      xval = events[idx].xval;
      yval = events[idx].yval;
      handle_event(type, xval, yval);
    }
    /*sleep(&endidx, &startlock);*/
  }
}

