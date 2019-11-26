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

struct spinlock windows_lock;
struct window *head;
struct window windows[MAX_WINDOWS];

void
init_windows()
{
  initlock(&windows_lock, "windows lock");
  char *window_frames = ((char*)kdisplaymem()) + FRAME_DATA_SIZE + CURSOR_DATA_SIZE;
  for(int i = 0; i < MAX_WINDOWS; i++){
    initlock(&windows[i].lock, "window lock"); 
    windows[i].free = 1;
    windows[i].frame_buf = (uint32*)window_frames;
    window_frames += FRAME_DATA_SIZE;
  }
}

void
get_window_outer_dims(struct window *win, int *oxpos, int *oypos, int *owidth, int *oheight)
{
  *oxpos = win->xpos - W_BORDER_SIZE;
  *oypos = win->ypos - W_HEADER_SIZE;
  *owidth = win->width + 2*W_BORDER_SIZE;
  *oheight = win->height + W_BORDER_SIZE + W_HEADER_SIZE;
}

void  
display_window(struct window *win)
{
  int oxpos, oypos, owidth, oheight; 
  get_window_outer_dims(win, &oxpos, &oypos, &owidth, &oheight);
  draw_rect(oxpos, oypos, owidth, oheight, C_WINDOW_GRAY, 255);
  int button_size = W_HEADER_SIZE - 2 * W_BORDER_SIZE;
  draw_rect(oxpos+W_BORDER_SIZE, oypos+W_BORDER_SIZE, button_size, button_size, C_BLACK, 255);
  draw_rect(oxpos+W_BORDER_SIZE+2, oypos+W_BORDER_SIZE+2, button_size-4, button_size-4, C_WINDOW_RED, 255);
  uint32 pix;
  printf("height=%d, width=%d\n", win->height, win->width);
  for(int y = 0; y < win->height; y++){
    for(int x = 0; x < win->width; x++){
      pix = *(win->frame_buf + y * win->width + x);
      /*if((y * win->width + x) % 100 == 0){*/
        /*printf("x=%d, y=%d, pix=%p\n", x, y, ((uint64)pix) & 0x00000000FFFFFFFF);*/
      /*}*/
      set_pixel_hex(win->xpos + x, win->ypos + y, pix);
    }
  }
}

void
display_windows()
{
  struct window *win = head;
  if(!win)
    return;
  
  do {
    display_window(win);
    win = win->next;
  } while(win != head);

  send_frame_update();
}

uint64
new_window(struct file *rf, struct file *wf)
{
  acquire(&windows_lock);
  struct window *win = 0;
  for(int i = 0; i < MAX_WINDOWS; i++){
    if(windows[i].free){
      windows[i].free = 0;
      win = &windows[i];
      break;
    }
  }
  if(!win){
    release(&windows_lock);
    return 0;
  }
  win->rf = rf;
  win->wf = wf;
  win->xpos = 300;
  win->ypos = 300;
  if(!head){
    head = win;
    win->next = win;
    win->prev = win;
  } else{
    win->prev = head->prev;
    win->next = head;
    head->prev->next = win;
    head->prev = win;
  }
  release(&windows_lock);
  return (uint64)win->frame_buf;
}

// windows_lock should be held when calling find_window
struct window*
find_window(struct file *rf)
{
  struct window* win = head;
  if(!win)
    return win;
  
  do {
    if(win->rf == rf)
      return win;
    win = win->next;
  } while(win != head);

  return (struct window*)0;
}

// windows_lock should be held when calling focus_window
void
focus_window(struct window* win)
{
  if(win->next == win)
    return; 
  win->prev->next = win->next;
  win->next->prev = win->prev;
  win->prev = head->prev;
  win->next = head;
  head->prev->next = win;
  head->prev = win;
}

int
update_window(struct file *rf, int width, int height)
{
  acquire(&windows_lock); 
  struct window *win = find_window(rf);  
  if(!win){
    release(&windows_lock);
    return -1;
  }
  win->width = width;
  win->height = height;
  display_windows();
  release(&windows_lock);
  return 0;
}
