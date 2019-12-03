#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"
#include "kernel/colors.h"
#include "user/gui.h"


void
pop_button(struct *elmt button)
{
  button->width = 100;
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
pop_textbox(struct *elmt textbox)
{
  textbox->width = 200;
  textbox->height = 20;
  textbox->x = 100;
  textbox->y = 50;
  textbox->main_fill = C_WHITE;
  textbox->main_alpha = 255;
  textbox->border = 2;  
  textbox->border_fill = C_GRAY;  
  textbox->border_alpha = 255;
  textbox->text = "";
  textbox->textlength = strlen(textbox->text);
  textbox->fontsize = 0;
  textbox->textalignment = 2;
  textbox->text_fill = C_BLACK;
  textbox->text_alpha = 255;
  return;
}

int
main(void)
{
  /*
  int fd;
  printf("&fd=%p\n", &fd);
  uint32 *frame_buf = (uint32*)mkwindow(&fd);
  printf("frame_buf=%p\n", frame_buf);
  printf("user: fd=%d\n", fd);
  int width = 400;
  int height = 300;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      *(frame_buf + y * width + x) = 0xFFFF00FF;
    }
  }
  struct gui* gui = init_gui();
  printf("finished init gui\n");
  char string[] = "Nutter butter &#$%^&*(";
  draw_font(frame_buf, width, height, 1, 1, gui->fonts, string, 4, strlen(string), C_BLACK, 255);  
  printf("finished draw_font\n");
  *memset(frame_buf, sizeof(uint32) * width * height, 0xff);*
  updatewindow(fd, width, height);
  */
  struct gui* gui = init_gui();
  struct window* window1 = new_window(gui, 500, 300);  
  struct state* state1 = new_state();
  add_state(window1, state1);
  add_window(gui, window1);
  struct elmt* button1 = new_elmt(BUTTON);
  pop_button(button1);
  struct elmt* textbox = new_elmt(TEXTBOX);
  pop_textbox(textbox);
  add_elmt(state1, button1);
  loop(gui, window1);
  exit(0);
}

