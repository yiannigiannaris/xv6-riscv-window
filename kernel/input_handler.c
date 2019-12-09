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
#include "input_handler.h"

#define MAX_EVENTS 2000

struct spinlock startlock, endlock;
struct input_event events[MAX_EVENTS];
int startidx, endidx;
int running;

void
init_input_handler()
{
  initlock(&startlock, "input handler start");
  initlock(&endlock, "input handler end");
  startidx = 0;
  endidx = 0;
  running = 0;
  printf("input handler initialized\n");
}

void
add_mouse_handler_event(uint timestamp, int type, int xval, int yval)
{
  acquire(&endlock);
  if(endidx - startidx >= MAX_EVENTS)
    printf("WARNING: INPUT HANDLER QUEUE FULL\n");
  events[endidx % MAX_EVENTS].dev = DEV_MOUSE;
  events[endidx % MAX_EVENTS].timestamp = timestamp;
  events[endidx % MAX_EVENTS].m_event.type = type;
  events[endidx % MAX_EVENTS].m_event.xval = xval;
  events[endidx % MAX_EVENTS].m_event.yval = yval;
  endidx++;
  if(running)
    wakeup(&endidx);
  release(&endlock);
}

void
add_keyboard_handler_event(uint timestamp, int code, int val)
{
  acquire(&endlock);
  if(endidx - startidx >= MAX_EVENTS)
    printf("WARNING: INPUT HANDLER QUEUE FULL\n");
  events[endidx % MAX_EVENTS].dev = DEV_KEYBOARD;
  events[endidx % MAX_EVENTS].timestamp = timestamp;
  events[endidx % MAX_EVENTS].k_event.code = code;
  events[endidx % MAX_EVENTS].k_event.val = val;;
  endidx++;
  if(running)
    wakeup(&endidx);
  release(&endlock);
}

int
process_event()
{
  if((startidx % MAX_EVENTS) == (endidx % MAX_EVENTS) && (endidx - startidx == 0))
    return -1; 
  int ret = (startidx % MAX_EVENTS); 
  startidx++;
  return ret;
}

void
handle_mouse_rel(uint timestamp, int xrel, int yrel)
{

}

void
handle_mouse_syn(uint timestamp, int xpos, int ypos)
{
  handle_cursor_move(timestamp, xpos, ypos);
}

void
handle_mouse_key(uint timestamp, int type, int xpos, int ypos)
{
  switch(type){
    case MOUSE_LEFT_CLICK_PRESS:
      handle_left_click_press(timestamp, xpos, ypos);
      break;
    case MOUSE_LEFT_CLICK_RELEASE:
      handle_left_click_release(timestamp, xpos, ypos);
      break;
    case MOUSE_RIGHT_CLICK_PRESS:
      handle_right_click_press(timestamp, xpos, ypos);
      break;
    case MOUSE_RIGHT_CLICK_RELEASE:
      handle_right_click_release(timestamp, xpos, ypos);
      break;
    default:
      printf("mouse_handler: handle_key event type not recognized\n");
  }
}

void
handle_mouse_event(uint timestamp, int type, int xval, int yval)
{
  switch(type){
    case MOUSE_SYN:
      handle_mouse_syn(timestamp, xval, yval);
      break;
    case MOUSE_REL:
      handle_mouse_rel(timestamp, xval, yval);
      break;
    default:
      handle_mouse_key(timestamp, type, xval, yval);
  }
}

void
handle_keyboard_event(uint timestamp, int code, int val)
{
  handle_keyboard(timestamp, code, val);
  /*printf("KEYBOARD EVENT timestamp=%d code=%d val=%d\n",timestamp, code, val);*/
}

void
start_input_handler()
{
  printf("starting input handler\n");
  if(running)
    panic("input handler already running\n");
  running = 1;
  int idx;
  int type, xval, yval, code, val;
  uint timestamp;
  acquire(&startlock);
  while(1){
    while((idx = process_event()) >= 0){
      timestamp = events[idx].timestamp;
      if(events[idx].dev == DEV_MOUSE){
        type = events[idx].m_event.type;
        xval = events[idx].m_event.xval;
        yval = events[idx].m_event.yval;
        handle_mouse_event(timestamp, type, xval, yval);
      } else if(events[idx].dev == DEV_KEYBOARD){
        code = events[idx].k_event.code;
        val = events[idx].k_event.val;
        handle_keyboard_event(timestamp, code, val);
      } else{
        printf("input_handler error: device type not recognized\n");
      }
    }
    sleep(&endidx, &startlock);
  }
}

