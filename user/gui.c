#include "kernel/types.h"
#include "user/user.h"
#include "elmt.h"
#include "gui.h"


int add_elmt(struct gui_window* gui, int state state, struct elmt* elmt)
{
  gui->states[state]->elmts[gui.elmt_count++] = elmt;
  return 0;
}

int modify_elmt(struct gui_window* gui, int state state, struct elmt* elmt)
{
  struct elmt *
}
int remove_elmt(struct gui_window* gui, int state state, struct elmt* elmt)
{
}
