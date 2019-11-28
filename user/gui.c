#include "kernel/types.h"
#include "user/user.h"
#include "elmt.h"
#include "gui.h"

int break_loop_flag = 0;

int add_elmt(struct gui_window* gui, int state state, struct elmt* elmt)
{
  struct elmt* head_elmt = gui->states[state]->head_elmt;
  head_elmt->prev->next = elmt;
  elmt->prev = head_elmt->prev;
  elmt->next = head_elmt;
  head_elmt->prev = elmt; 
  //acquire locks, call update
  break_loop_flag = 1;
  return 0;
}

int notify_elmt_modify(struct gui_window* gui, int state state, struct elmt* elmt)
{
  //acquire locks, call update
  break_loop_flag = 1;
}

int remove_elmt(struct gui_window* gui, int state state, struct elmt* elmt)
{
 elmt->prev->next = elmt->next;
 elmt->next->prev = elmt->prev; 
 //call update
 break_loop_flag = 1;
}

void loop(struct elmt* elmt){
  int break_loop_flag = 0;
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
