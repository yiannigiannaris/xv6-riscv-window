#include "kernel/types.h"
#include "user/user.h"
#include "user/genfonts.h"
#include "user/gui.h"
#include "user/draw.h"

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
    text_y = (elmt->y + (elmt->y + elmt->height + (2*elmt->border)))/2 - elmt->fontsize/2;
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
  state->elmt_count += 1;
  if(!state->elmt){
    state->elmt = elmt;
    return;
  }
  struct elmt* head_elmt = state->elmt;
  head_elmt->prev->next = elmt;
  elmt->prev = head_elmt->prev;
  elmt->next = head_elmt;
  head_elmt->prev = elmt; 
  notify_change();
  return;
}

void
remove_elmt(struct state* state, struct elmt* elmt)
{
 elmt->prev->next = elmt->next;
 elmt->next->prev = elmt->prev; 
 state->elmt_count -= 1;
 if(!state->elmt_count)
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

void
loop(struct gui* gui, struct window* window){
  /*(
  int fd = 0;
  struct event e;
  for(read(fd,&e, sizeof(struct event))){
     //read mouse event
     //process mouse event
     //check for     
  */
  struct state* state;
  while(1){
    state = window->state;
    struct elmt* e = state->elmt;
    for(int i = 0; i < state->elmt_count; i++, e = e->next){
      draw(gui, window, e);
    }  
    updatewindow(window->fd, window->width, window->height);
    ack_change();
    for (struct elmt* elmt = state->elmt;;elmt = elmt->next){
      if(check_change())
        break; 
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
  state->elmt_count = 0;
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
  gui->window = win;
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
