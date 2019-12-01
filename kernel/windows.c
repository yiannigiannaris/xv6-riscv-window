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

struct {
  int held;
  int xanchor;
  int yanchor;
  struct window* win;
} header_held;

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
print_windows()
{
  printf("WINDOWS PRINT:\n");
  struct window *win = head;
  if(!win){
    printf("  no windows\n");
    return;
  }
  do {
    printf("%p: xpos=%d, ypos=%d, width=%d, height=%d, prev=%p, next=%p\n", win, win->xpos, win->ypos, win->width, win->height, win->prev, win->next);
    win = win->next;
  } while(win != head);
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
move_window_rel(struct window *win, int xrel, int yrel)
{
  int oxpos, oypos, owidth, oheight;
  get_window_outer_dims(win, &oxpos, &oypos, &owidth, &oheight);
  if(oxpos + xrel < 0){
    win->xpos = 0 + W_BORDER_SIZE;
  } else if(oxpos + owidth + xrel >= FRAME_WIDTH){
    win->xpos = FRAME_WIDTH - W_BORDER_SIZE - win->width;
  } else {
    win->xpos += xrel;
  }

  if(oypos + yrel < 0){
    win->ypos = 0 + W_HEADER_SIZE;
  } else if(oypos + oheight + yrel >= FRAME_HEIGHT){
    win->ypos = FRAME_HEIGHT - W_BORDER_SIZE - win->width;
  } else {
    win->ypos += yrel;
  }
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
  for(int y = 0; y < win->height; y++){
    for(int x = 0; x < win->width; x++){
      pix = *(win->frame_buf + y * win->width + x);
      set_pixel_hex(win->xpos + x, win->ypos + y, pix);
    }
  }
}

void
display_windows()
{
  draw_wallpaper();
  struct window *win = head;
  if(!win)
    return;

  win = win->prev;
  do {
    display_window(win);
    win = win->prev;
  } while(win != head->prev);

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
  head = win;
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

int
is_pos_in_window(struct window* win, int xpos, int ypos)
{
  int oxpos, oypos, owidth, oheight;
  get_window_outer_dims(win, &oxpos, &oypos, &owidth, &oheight);
  if(xpos >= oxpos && xpos < oxpos + owidth && ypos >= oypos && ypos < oypos + oheight)
    return 1;
  return 0;
}

int
is_pos_in_header(struct window* win, int xpos, int ypos)
{
  if(is_pos_in_window(win, xpos, ypos) && ypos < win->ypos)
    return 1;
  return 0;
}

void
handle_left_click_press_focused(struct window* win, int xpos, int ypos)
{
  if(!is_pos_in_header(win, xpos, ypos))
    return;
  header_held.held = 1;
  header_held.win = win;
  header_held.xanchor = xpos;
  header_held.yanchor = ypos;
}


void
handle_left_click_press(int xpos, int ypos)
{
  struct window* win = head;
  if(!win)
    return;
  if(is_pos_in_window(win, xpos, ypos)){
    handle_left_click_press_focused(win, xpos, ypos);
    return;
  }

  win = win->next;
  while(win != head) {
    if(is_pos_in_window(win, xpos, ypos)){
      focus_window(win);
      display_windows();
      return;
    }
    win = win->next;
  } 
}

void
handle_left_click_release()
{
  header_held.held = 0;
}

void
handle_cursor_move(int xpos, int ypos)
{
  if(!header_held.held)
    return;
  int xrel = xpos - header_held.xanchor;
  int yrel = ypos - header_held.yanchor;
  /*printf("window: xrel=%d, yrel=%d\n", xrel, yrel);*/
  header_held.xanchor = xpos;
  header_held.yanchor = ypos;
  move_window_rel(header_held.win, xrel, yrel);
  display_windows();
}

