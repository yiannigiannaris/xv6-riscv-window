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
#include "window_event.h"
#include "fs.h"
#include "file.h"
#include "events.h"
#include "keyconv.h"


struct spinlock windows_lock;
struct window *head;
struct window windows[MAX_WINDOWS+1];
struct window *applauncher;
int applauncher_vis;
int shift_held;

struct {
  int held;
  struct window *win;
} win_held;

struct {
  int held;
  int xanchor;
  int yanchor;
  struct window* win;
  int steps;
} header_held;


void
init_windows()
{
  initlock(&windows_lock, "windows lock");
  char *window_frames = ((char*)kdisplaymem()) + FRAME_DATA_SIZE + CURSOR_DATA_SIZE;
  for(int i = 0; i < MAX_WINDOWS + 1; i++){
    initlock(&windows[i].lock, "window lock"); 
    windows[i].free = 1;
    windows[i].frame_buf = (uint32*)window_frames;
    window_frames += FRAME_DATA_SIZE;
  }
  applauncher = &windows[MAX_WINDOWS];
  applauncher_vis = 0;
  shift_held = 0;
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
  } else if(oypos + oheight + yrel >= FRAME_HEIGHT - TASKBAR_SIZE){
    win->ypos = FRAME_HEIGHT - TASKBAR_SIZE - W_BORDER_SIZE - win->height;
  } else {
    win->ypos += yrel;
  }
}

void
display_taskbar()
{
  draw_rect(0, FRAME_HEIGHT-TASKBAR_SIZE, FRAME_WIDTH, TASKBAR_SIZE, C_LAUNCH_GRAY, 255);
  draw_rect(0, FRAME_HEIGHT-TASKBAR_SIZE, TASKBAR_SIZE, TASKBAR_SIZE, C_LAUNCH_BLUE, 255);
  draw_rect(8, FRAME_HEIGHT-TASKBAR_SIZE+8, TASKBAR_SIZE-16, TASKBAR_SIZE-16, C_WHITE, 255);
  draw_rect(12, FRAME_HEIGHT-TASKBAR_SIZE+12, TASKBAR_SIZE-24, TASKBAR_SIZE-24, C_LAUNCH_BLUE, 255);
}

void
display_applauncher()
{
  if(!applauncher_vis)
    return;
  uint32 pix;
  for(int y = 0; y < applauncher->height; y++){
    for(int x = 0; x < applauncher->width; x++){
      pix = *(applauncher->frame_buf + y * applauncher->width + x);
      set_pixel_hex(applauncher->xpos + x, applauncher->ypos + y, pix);
    }
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
  if(win){
    win = win->prev;
    do {
      display_window(win);
      win = win->prev;
    } while(win != head->prev);
  }
  display_applauncher();
  display_taskbar();
  send_frame_update();
}

struct window*
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
  win->ypos = 100;
  if(!head){
    head = win;
    win->next = win;
    win->prev = win;
  } else{
    win->next = head;
    win->prev = head->prev;
    head->prev->next = win;
    head->prev = win;
    head = win;
  }
  release(&windows_lock);
  return win;
}

struct window*
make_applauncher(struct file *rf, struct file *wf)
{
  acquire(&windows_lock);
  if(applauncher->free == 0)
    panic("applauncher already made");
  applauncher->free = 0;
  applauncher->rf = rf;
  applauncher->wf = wf;
  applauncher->xpos = 0;
  applauncher->ypos = 500;
  release(&windows_lock);
  return applauncher;
}


void
close_window(struct window* win)
{
  struct proc *p;
  int fd, has_window;
  for(p = proc; p < &proc[NPROC]; p++) {
    has_window = 0;
    for(fd = 0; fd < NOFILE; fd++){
      if(p->ofile[fd] == win->rf){
        p->ofile[fd] = 0;
        has_window = 1;
        break;
      }
    }

    if(has_window){
      uvmunmap(p->pagetable, win->fbva, FRAME_DATA_SIZE, 0);
      p->nwindows--;
      if(p->nwindows < 1){
        kill(p->pid);
      }
    }
  }

  if(win->next == win){
    head = 0;
  } else {
    win->next->prev = win->prev;
    win->prev->next = win->next;
    if(win == head)
      head = win->next;
  }
  fileclose(win->rf);
  fileclose(win->wf);
  win->free = 1;
  display_windows();
}

// windows_lock should be held when calling find_window
struct window*
find_window(struct file *rf)
{
  if(!applauncher->free && applauncher->rf == rf)
    return applauncher;
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

void
user_close_window(struct file *rf)
{
  struct window *win = find_window(rf);
  close_window(win);
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
  if(win == applauncher){
    win->ypos = FRAME_HEIGHT - TASKBAR_SIZE - height;
  }
  display_windows();
  release(&windows_lock);
  return 0;
}

void
abs_pos_to_rel(struct window *win, int xabs, int yabs, int *xrel, int *yrel)
{
  *xrel = xabs - win->xpos;
  *yrel = yabs - win->ypos;
}

int
is_pos_in_taskbar(int xpos, int ypos)
{
  if(ypos >= FRAME_HEIGHT - TASKBAR_SIZE)
    return 1;
  return 0;
}

int
is_pos_in_launchbutton(int xpos, int ypos)
{
  if(!is_pos_in_taskbar(xpos, ypos))
    return 0;
  if(xpos < TASKBAR_SIZE)
    return 1;
  return 0;
}

int
is_pos_in_applauncher(int xpos, int ypos)
{
  if(xpos >= applauncher->xpos && xpos < applauncher->xpos + applauncher->width && ypos >= applauncher->ypos && ypos < applauncher->ypos + applauncher->height)
    return 1;
  return 0;
}

int
is_pos_in_window_frame(struct window *win, int xpos, int ypos)
{
  if(xpos >= win->xpos && xpos < win->xpos + win->width && ypos >= win->ypos && ypos < win->ypos + win->height)
    return 1;
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

int
is_pos_in_close_box(struct window *win, int xpos, int ypos)
{
  if(!is_pos_in_header(win, xpos, ypos))
    return 0;
  int oxpos, oypos, owidth, oheight;
  get_window_outer_dims(win, &oxpos, &oypos, &owidth, &oheight);
  int bxpos = oxpos + W_BORDER_SIZE+2;
  int bypos = oypos + W_BORDER_SIZE+2;
  int button_size =  W_HEADER_SIZE - 2 * W_BORDER_SIZE;
  if(xpos >= bxpos && xpos < bxpos + button_size && ypos >= bypos && ypos < bypos + button_size)
    return 1;
  return 0;
}

void
handle_left_click_press_header(struct window *win, int xpos, int ypos)
{
  header_held.held = 1;
  header_held.win = win;
  header_held.xanchor = xpos;
  header_held.yanchor = ypos;
  header_held.steps = 0;
}

void
handle_left_click_press_focused(uint timestamp, struct window* win, int xpos, int ypos)
{
  if(is_pos_in_close_box(win, xpos, ypos)){
    close_window(win);
  } else if(is_pos_in_header(win, xpos, ypos)){
    handle_left_click_press_header(win, xpos, ypos);
  } else if(is_pos_in_window_frame(win, xpos, ypos)){
    int xrel = xpos - win->xpos;
    int yrel = ypos - win->ypos;
    struct window_event event = {.timestamp = timestamp, .kind = W_KIND_MOUSE, .m_event.type = W_LEFT_CLICK_PRESS, .m_event.xval = xrel, .m_event.yval = yrel};
    w_pipewrite(win->wf->w_pipe, (uint64)&event);
    win_held.held = 1;
    win_held.win = win;
  }
}


void
handle_left_click_press(uint timestamp, int xpos, int ypos)
{
  acquire(&windows_lock);
  if(!applauncher->free){
    if(applauncher_vis){
      if(is_pos_in_applauncher(xpos, ypos)){
        handle_left_click_press_focused(timestamp, applauncher, xpos, ypos);
        applauncher_vis = 0;
        release(&windows_lock);
        return;
      } else {
        applauncher_vis = 0;
        display_windows();
      }
    } else {
      if(is_pos_in_launchbutton(xpos, ypos)){
        applauncher_vis = 1;
        display_windows();
        release(&windows_lock);
        return;
      }
    }
  }
  struct window* win = head;
  if(!win){
    release(&windows_lock);
    return;
  }
  if(is_pos_in_window(win, xpos, ypos)){
    handle_left_click_press_focused(timestamp, win, xpos, ypos);
    release(&windows_lock);
    return;
  }

  win = win->next;
  while(win != head) {
    if(is_pos_in_window(win, xpos, ypos)){
      focus_window(win);
      display_windows();
      if(is_pos_in_header(win, xpos, ypos))
        handle_left_click_press_header(win, xpos, ypos);
      release(&windows_lock);
      return;
    }
    win = win->next;
  } 
  release(&windows_lock);
}

void
handle_left_click_release(uint timestamp, int xpos, int ypos)
{
  acquire(&windows_lock);
  if(header_held.held){
    int xrelmove = xpos - header_held.xanchor;
    int yrelmove = ypos - header_held.yanchor;
    header_held.xanchor = xpos;
    header_held.yanchor = ypos;
    move_window_rel(header_held.win, xrelmove, yrelmove);
    display_windows();
    header_held.held = 0;
  } else if(win_held.held){
    int xrelpos = xpos - win_held.win->xpos;
    int yrelpos = ypos - win_held.win->ypos;
    struct window_event event = {.timestamp = timestamp, .kind = W_KIND_MOUSE, .m_event.type = W_LEFT_CLICK_RELEASE, .m_event.xval = xrelpos, .m_event.yval = yrelpos};
    w_pipewrite(win_held.win->wf->w_pipe, (uint64)&event);
    win_held.held = 0;
  }
  release(&windows_lock);
}

void
handle_right_click_press_focused(uint timestamp, struct window* win, int xpos, int ypos)
{
  if(is_pos_in_window_frame(win, xpos, ypos)){
    int xrel = xpos - win->xpos;
    int yrel = ypos - win->ypos;
    struct window_event event = {.timestamp = timestamp, .kind = W_KIND_MOUSE, .m_event.type = W_RIGHT_CLICK_PRESS, .m_event.xval = xrel, .m_event.yval = yrel};
    w_pipewrite(win->wf->w_pipe, (uint64)&event);
    win_held.held = 1;
    win_held.win = win;
  }
}


void
handle_right_click_press(uint timestamp, int xpos, int ypos)
{
  acquire(&windows_lock);
  struct window* win = head;
  if(!win){
    release(&windows_lock);
    return;
  }
  if(is_pos_in_window(win, xpos, ypos)){
    handle_right_click_press_focused(timestamp, win, xpos, ypos);
    release(&windows_lock);
    return;
  }
  release(&windows_lock);
}

void
handle_right_click_release(uint timestamp, int xpos, int ypos)
{
  acquire(&windows_lock);
  if(win_held.held){
    int xrelpos = xpos - win_held.win->xpos;
    int yrelpos = ypos - win_held.win->ypos;
    struct window_event event = {.timestamp = timestamp, .kind = W_KIND_MOUSE, .m_event.type = W_RIGHT_CLICK_RELEASE, .m_event.xval = xrelpos, .m_event.yval = yrelpos};
    w_pipewrite(win_held.win->wf->w_pipe, (uint64)&event);
    win_held.held = 0;
  }
  release(&windows_lock);
}


void
handle_cursor_move(uint timestamp, int xpos, int ypos)
{
  acquire(&windows_lock);
  if(header_held.held){
    header_held.steps++;
    if(!(header_held.steps % 4 == 0)){
      release(&windows_lock);
      return;
    }
    int xrelmove = xpos - header_held.xanchor;
    int yrelmove = ypos - header_held.yanchor;
    header_held.xanchor = xpos;
    header_held.yanchor = ypos;
    move_window_rel(header_held.win, xrelmove, yrelmove);
    display_windows();
  } else {
    if(!head){
      release(&windows_lock);
      return;
    }
    struct window *win = 0;
    if(!applauncher->free && applauncher_vis && is_pos_in_applauncher(xpos, ypos))
      win = applauncher;
    else if(is_pos_in_window_frame(head, xpos, ypos))
      win = head;

    if(win){
      int xrelpos = xpos - win->xpos;
      int yrelpos = ypos - win->ypos;
      struct window_event event = {.timestamp = timestamp, .kind = W_KIND_MOUSE, .m_event.type = W_CUR_MOVE_ABS, .m_event.xval = xrelpos, .m_event.yval = yrelpos};
      w_pipewrite(win->wf->w_pipe, (uint64)&event);
    }
  }
  release(&windows_lock);
}

void
handle_keyboard(uint timestamp, int code, int val)
{
  acquire(&windows_lock);
  struct window *win;
  if(!applauncher->free && applauncher_vis)
    win = applauncher;
  else
    win = head;
  if(!win){
    release(&windows_lock);
    return;
  }
  if(code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT){
    if(val)
      shift_held = 1;
    else
      shift_held = 0;
    release(&windows_lock);
    return;
  }  

  if(!val){
    release(&windows_lock);
    return;
  }

  uint8 c;
  if(shift_held)
    c = upper_keys[code]; 
  else
    c = lower_keys[code];
  struct window_event event = {.timestamp = timestamp, .kind = W_KIND_KEYBOARD, .k_event.ascii_val = c};
  w_pipewrite(win->wf->w_pipe, (uint64)&event);
  release(&windows_lock);
}


