#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"
#include "kernel/colors.h"
#include "user/gui.h"
#include "user/uthread.h"


void
pop_button(struct elmt* button)
{
  printf("button: %p\n", button);
  printf("test\n");
  button->width = 100;
  printf("test\n");
  button->height = 50;
  button->x = 50;
  button->y = 50;
  button->main_fill = C_GRAY;
  button->main_alpha = 255;
  button->border = 2;  
  button->border_fill = C_BLUE;  
  button->border_alpha = 255;
  button->text = "Button 1";
  button->textlength = strlen(button->text);
  button->fontsize = 0;
  button->textalignment = 1;
  button->text_fill = C_BLACK;
  button->text_alpha = 255;
  return;
}

void
pop_textbox(struct elmt* textbox)
{
  printf("textbox: %p\n", textbox);
  printf("test\n");
  textbox->width = 150;
  printf("test\n");
  textbox->height = 20;
  textbox->x = 250;
  textbox->y = 50;
  textbox->main_fill = C_WHITE;
  textbox->main_alpha = 255;
  textbox->border = 2;  
  textbox->border_fill = C_BLUE;  
  textbox->border_alpha = 255;
  textbox->text = "Textbox 1";
  textbox->textlength = strlen(textbox->text);
  textbox->fontsize = 0;
  textbox->textalignment = 2;
  textbox->text_fill = C_BLACK;
  textbox->text_alpha = 255;
  return;
}

void funcl(void){
  printf("left click handler\n");
};

void funcr(void){
  printf("right click handler\n");
};

void tfuncl(void){
  printf("textbox left click handler\n");
};

void tfuncr(void){
  printf("textbox right click handler\n");
};

int
main(void)
{
  struct gui* gui = init_gui();
  struct window* window1 = new_window(gui, 500, 300);  
  struct state* state1 = new_state();
  add_state(window1, state1);
  add_window(gui, window1);
  struct elmt* button1 = new_elmt(BUTTON);
  pop_button(button1);
  button1->mlc = &funcl;
  button1->mrc = &funcr;
  struct elmt* textbox = new_elmt(TEXTBOX);
  pop_textbox(textbox);
  float testf = 1.9;
  int testff = testf * 10 -10;
  printf("test: %d\n", testff);
  textbox->mlc = &tfuncl;
  textbox->mrc = &tfuncr;
  add_elmt(state1, button1);
  add_elmt(state1, textbox);
  loop(gui, window1);
  exit(0);
}

