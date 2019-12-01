#include "kernel/types.h"
#include "user/user.h"
#include "elmt.h"
#include "gui.h"
#include "genfonts.h"

int break_loop_flag = 0;

void notify_change()
{
  //acquire locks, call update
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
  gui->state = state;
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
  //acquire locks, call update
  notify_change();
  return 0;
}


int remove_elmt(struct elmt* elmt)
{
 elmt->prev->next = elmt->next;
 elmt->next->prev = elmt->prev; 
 //call update
 notify_change();
 return 0;
}

void loop(struct elmt* elmt){
  if(check_change())
    ack_change();
  int fd = 0
  struct event e;
  for(read(fd,&e, sizeof(struct event))){
     //read mouse event
     //process mouse event
     //check for     
    for (;;elmt = elmt->next){
       
    } 
  }
}

struct window*
new_window(struct gui* gui)
{
  struct window* win = (struct window*)malloc(sizeof(struct window*));
  gui->window = win;
  return 0;
}

struct gui*
init_gui()
{
  struct gui *gui = (struct gui*)malloc(sizeof(gui));
  if(!(gui->fonts = load_fonts())){
    return 0;
  }
  //call for framebuffer
  return gui; 
}
