#include "kernel/types.h"
#include "user/user.h"
#include "user/genfonts.h"
#include "user/gui.h"

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

int add_state(struct window* window, struct state* state)
{
  struct state* head = window->state;
  head->prev->next = state;
  state->prev = head->prev;
  state->next = head;
  head->prev = state;
  return 0;
}

int switch_state(struct window* window, struct state* state)
{
  window->state = state;
  notify_change();
  return 0;
}

int add_elmt(struct state* state, struct elmt* elmt)
{
  struct elmt* head_elmt = state->head_elmt;
  head_elmt->prev->next = elmt;
  elmt->prev = head_elmt->prev;
  elmt->next = head_elmt;
  head_elmt->prev = elmt; 
  notify_change();
  return 0;
}


int remove_elmt(struct elmt* elmt)
{
 elmt->prev->next = elmt->next;
 elmt->next->prev = elmt->prev; 
 notify_change();
 return 0;
}

void loop(struct elmt* elmt){
  if(check_change())
    ack_change();
  /*(
  int fd = 0;
  struct event e;
  for(read(fd,&e, sizeof(struct event))){
     //read mouse event
     //process mouse event
     //check for     
    for (;;elmt = elmt->next){
       
    } 
  }
  */
}

struct window*
new_window(struct gui* gui)
{
  int fd;
  uint32* frame_buffer = (uint32*)mkwindow(&fd); 
  struct window* win = (struct window*)malloc(sizeof(struct window*));
  win->fd = fd;
  win->frame_buffer = frame_buffer;
  gui->window = win;
  return 0;
}

struct gui*
init_gui(void)
{
  struct gui *gui = (struct gui*)malloc(sizeof(gui));
  if(!(gui->fonts = loadfonts())){
    return 0;
  }
  return gui; 
}
