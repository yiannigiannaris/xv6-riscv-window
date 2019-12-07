#include "kernel/types.h"
#include "user/user.h"
#include "user/genfonts.h"
#include "user/gui.h"
#include "user/draw.h"
#include "kernel/window_event.h"
#include "kernel/events.h"
#include "kernel/input_handler.h"

int fontsizes[5] = FONTSIZES;

int break_loop_flag = 0;

void notify_change()
{
  break_loop_flag = 1;
  return;
}

void ack_change()
{
  break_loop_flag = 0;
  return;
}

int check_change()
{
  return break_loop_flag;
}

void
draw_elmt(struct gui* gui, struct window* window, struct elmt* elmt)
{
  if(elmt->border)
    draw_rect(window->frame_buffer, window->width, window->height, elmt->x, elmt->y, elmt->width + (2*elmt->border), elmt->height + (2*elmt->border), elmt->border_fill, elmt->border_alpha);    
  draw_rect(window->frame_buffer, window->width, window->height, elmt->x + elmt->border, elmt->y + elmt->border, elmt->width, elmt->height, elmt->main_fill, elmt->main_alpha); 
  if(elmt->textlength > 0){
    int dwidth = gui->fonts[elmt->fontsize][41][2]; 
    int totallength = dwidth * elmt->textlength;
    int text_x = 0;
    int text_y = 0;
    switch(elmt->textalignment){
      case 0:
        text_x = elmt->x + elmt->border + 1;
        break;
      case 1:
        text_x = (elmt->x + (elmt->x + elmt->width + (2*elmt->border)))/2 - totallength/2;
        break;
      case 2:
        text_x = elmt->x + elmt->width +elmt->border - totallength;
        break;
    }
    text_y = (elmt->y + (elmt->y + elmt->height + (2*elmt->border)))/2 - fontsizes[elmt->fontsize]/2;
    draw_font(window->frame_buffer, window->width, window->height, text_x, text_y, gui->fonts, elmt->text, elmt->fontsize, elmt->textlength, elmt->text_fill, elmt->text_alpha);
  } 
}

void
draw_button(struct gui* gui, struct window* window, struct elmt* elmt)
{
  draw_elmt(gui, window, elmt);
}

void
draw_textbox(struct gui* gui, struct window* window, struct elmt* elmt)
{
  draw_elmt(gui, window, elmt);
}

void
draw_textblock(struct gui* gui, struct window* window, struct elmt* elmt)
{
  draw_elmt(gui, window, elmt);
}

void
draw_rectangle(struct window* window, uint32 color, uint8 alpha)
{
  draw_rect(window->frame_buffer, window->width, window->height, 0, 0, window->width, window->height, color, alpha); 
}

void 
draw(struct gui* gui, struct window* window, struct elmt* elmt)
{
  switch (elmt->type){
    case BUTTON:
      draw_button(gui, window, elmt);
      break;
    case TEXTBOX:
      draw_textbox(gui, window, elmt);
      break;
    case TEXTBLOCK:
      draw_textblock(gui, window, elmt);
      break;
  }
  return;    
}


void
switch_state(struct window* window, struct state* state)
{
  window->state = state;
  notify_change();
  return;
}

void
add_elmt(struct state* state, struct elmt* elmt)
{
  if(!state->elmt){
    state->elmt = elmt;
    elmt->next = 0;
    elmt->prev = elmt;
    return;
  }
  struct elmt* head_elmt = state->elmt;
  head_elmt->prev->next = elmt;
  elmt->prev = head_elmt->prev;
  elmt->next = 0;
  head_elmt->prev = elmt; 
  notify_change();
  return;
}

void
remove_elmt(struct state* state, struct elmt* elmt)
{
 elmt->prev->next = elmt->next;
 if(elmt->next)
   elmt->next->prev = elmt->prev; 
 if(elmt->prev == elmt)
   state->elmt = 0;
 notify_change();
 return;
}

void
modify_elmt(void)
{
  notify_change();
  return;
}

int
intersect(struct window_event *event, struct elmt *elmt)
{
  return event->m_event.xval >= elmt->x && 
         event->m_event.xval <= elmt->x + elmt->width && 
         event->m_event.yval >= elmt->y && 
         event->m_event.yval <= elmt->y + elmt->height;
}

void
handle_mouse_event(struct window* window, struct window_event *event, struct elmt *elmt)
{
  switch (event->m_event.type){
    case W_SYN:
      break;
    case W_CUR_MOVE_ABS:
      break;
    case W_LEFT_CLICK_PRESS:
      if(intersect(event, elmt)){
        elmt->l_depressed = 1;
      }
      break;
    case W_LEFT_CLICK_RELEASE:
      if(intersect(event, elmt) && elmt->l_depressed){
        elmt->l_depressed = 0;
        (*(elmt->mlc))(elmt->id);
      }
      break;
    case W_RIGHT_CLICK_PRESS:
      for(struct elmt* e = window->state->elmt; e != 0;e = e->next){
        e->r_depressed = 0;
      }
      if(intersect(event, elmt)){
        elmt->r_depressed = 1;
      }
      break;
    case W_RIGHT_CLICK_RELEASE:
      if(intersect(event, elmt) && elmt->r_depressed){
        elmt->r_depressed = 0;
        elmt->mrc(elmt->id);
      }
      break;
  }    
} 


void
handle_keyboard_event(struct window* window, struct window_event *event, struct elmt *elmt)
{
  if(elmt->type != TEXTBOX)
   return; 
  elmt->keyboard_input(elmt->id, event->k_event.ascii_val);
}

void
handle_event(struct window* window, struct window_event *event, struct elmt *elmt){
  if(event->kind == W_KIND_MOUSE){
    handle_mouse_event(window, event, elmt);
  } else if(event->kind == W_KIND_KEYBOARD){
    handle_keyboard_event(window, event, elmt);
  } else{
    printf("gui: window event kind not recognized\n");
  }
}

void
loop(struct gui* gui, struct window* window){
  struct window_event event;
  while(1){
    for(struct elmt* e = window->state->elmt; e != 0;e = e->next){
      draw(gui, window, e);
    }  
    updatewindow(window->fd, window->width, window->height);
    ack_change();
    while(read(window->fd, &event, sizeof(struct window_event) > 0)){
      for(struct elmt* e = window->state->elmt; e != 0;e = e->next){
        handle_event(window, &event, e);
        if(check_change())
          break; 
      } 
      if(check_change()){
        break;
      }
    }
  }
}

struct elmt*
new_elmt(enum elmt_type type)
{
  struct elmt* elmt = (struct elmt*)malloc(sizeof(struct elmt));
  elmt->type = type;
  return elmt;
}

void
add_state(struct window* window, struct state* state)
{
  if(!window->state){
    window->state = state;
    return;
  }
  struct state* head = window->state;
  head->prev->next = state;
  state->prev = head->prev;
  state->next = head;
  head->prev = state;
  return;
}

struct state*
new_state()
{
  struct state* state = (struct state*)malloc(sizeof(struct state));
  state->elmt = 0;
  return state;
}

void
add_window(struct gui* gui, struct window* win)
{
  if(!gui->window){
    gui->window = win;
    return;
  }
  gui->window->prev->next = win;
  win->prev = gui->window->prev;
  win->next = gui->window;
  gui->window->prev = win;
}



struct window*
new_window(struct gui* gui, int width, int height)
{
  int fd;
  uint32* frame_buffer = (uint32*)mkwindow(&fd); 
  struct window* win = (struct window*)malloc(sizeof(struct window));
  win->fd = fd;
  win->frame_buffer = frame_buffer;
  win->width = width;
  win->height = height;
  win->state = 0;
  return win;
}

struct window*
new_applauncher(struct gui* gui, int width, int height)
{
  int fd;
  uint32* frame_buffer = (uint32*)mkapplauncher(&fd); 
  struct window* win = (struct window*)malloc(sizeof(struct window));
  win->fd = fd;
  win->frame_buffer = frame_buffer;
  win->width = width;
  win->height = height;
  win->state = 0;
  return win;
}

struct gui*
init_gui(void)
{
  struct gui *gui = (struct gui*)malloc(sizeof(gui));
  if(!(gui->fonts = loadfonts())){
    return 0;
  }
  gui->window = 0;
  return gui; 
}
