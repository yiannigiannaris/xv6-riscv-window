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

typedef signed long int64;

struct c_float{
  int64 val;
  int64 decimal;
};

char nums[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// Reverses a string 'str' of length 'len' 
void reverse(char* str, int len) 
{ 
    int i = 0, j = len - 1, temp; 
    while (i < j) { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 

int pow(int x, int y)  {  
    int temp;  
    if(y == 0)  
        return 1;  
    temp = pow(x, y / 2);  
    if (y % 2 == 0)  
        return temp * temp;  
    else
    {  
        if(y > 0)  
            return x * temp * temp;  
        else
            return (temp * temp) / x;  
    }  
} 



struct c_float
new_float(int i)
{
  struct c_float f = {.val = i, .decimal = 0};
  return f;
}

struct c_float
add(struct c_float a, struct c_float b)
{
 if(a.decimal < b.decimal){
   a.val = a.val * pow(10, b.decimal - a.decimal);
   a.decimal = b.decimal;
 } else {
   b.val = b.val * pow(10, a.decimal - b.decimal);
   b.decimal = a.decimal;
 } 
 struct c_float f = {.val = a.val + b.val, .decimal = a.decimal};
 return f; 
}

struct c_float
subtract(struct c_float a, struct c_float b)
{
 if(a.decimal < b.decimal){
   a.val = a.val * pow(10, b.decimal - a.decimal);
   a.decimal = b.decimal;
 } else {
   b.val = b.val * pow(10, a.decimal - b.decimal);
   b.decimal = a.decimal;
 } 
 struct c_float f = {.val = a.val - b.val, .decimal = a.decimal};
 return f; 
}

struct c_float
multi(struct c_float a, struct c_float b)
{
 struct c_float f = {.val = a.val * b.val, .decimal = a.decimal + b.decimal};
 return f; 
}

struct c_float
div(struct c_float a, struct c_float b)
{
 if(a.decimal < b.decimal){
   a.val *= pow(10, b.decimal - a.decimal);
 } else {
   b.val *= pow(10, a.decimal - b.decimal);
 } 
 int64 even_divide = a.val / b.val;
 int64 remainder = a.val % b.val; 
 int64 places = 0;
 int64 temp = remainder;
 while (temp != 0){
  places += 1;
  temp /= 10; 
 }
 places = 12 - places;
 int64 accuracy = pow(10, places);
 int64 decimal = (remainder*accuracy)/b.val;
 temp = decimal;
 int64 decimal_places = 0;
 places = 1;
 while ( temp != 0){
  places *= 10;
  decimal_places += 1;
  temp /= 10; 
 }
 even_divide *= places;
 even_divide += decimal;
 struct c_float f = {.val = even_divide, .decimal = decimal_places};
 return f; 
}

void
print_float(struct c_float f, char* num)
{
  char *c = num;
  int i; 
  int neg = f.val < 0;
  if(neg)
    f.val = -1 * f.val;
  int putdec = f.decimal;
  if(f.val == 0){
    *c = '0';
  } else{
    while(f.val != 0 || f.decimal >= 0){
      if(f.decimal != 0 || !putdec){
        i = f.val % 10;
        *c = nums[i];
        f.val = f.val / 10;
      } else if(putdec){
        if(f.val % 10 == 0){
          *c = *(c - 1);
          *(c - 1) = '.';
        } else{
        *c = '.';  
        }
      }
      c++;
      f.decimal--;
    } 
    if(neg)
      *c = '-';
  }
  c++;
  *c = '\0'; 
  reverse(num, strlen(num));
  return;
}

void
pop_button(struct elmt* button)
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

char textbox_text[100];
void
pop_textbox(struct elmt* textbox, int x, int y, int width, int height)
{
  textbox->width = width;
  textbox->height = height;
  textbox->x = x;
  textbox->y = y;
  textbox->main_fill = C_WHITE;
  textbox->main_alpha = 255;
  textbox->border = 2;  
  textbox->border_fill = C_BLUE;  
  textbox->border_alpha = 255;
  textbox->text = textbox_text;
  textbox->textlength = 0;
  textbox->fontsize = 2;
  textbox->textalignment = 2;
  textbox->text_fill = C_BLACK;
  textbox->text_alpha = 255;
  return;
}

struct elmt *textbox;

enum ops {NONE, EQ, PERC, ADD, SUB, MULT, DIV};

struct c_float value = {.val = 0, .decimal = 0};
struct c_float scratch = {.val = 0, .decimal = 0};
int decimal = 0;
enum ops op = NONE;

void
write_scratch()
{
  print_float(scratch, textbox_text);
  textbox->text = textbox_text;
  textbox->textlength = strlen(textbox_text);
  modify_elmt();
}

void
process_value(int i)
{
  scratch.val = scratch.val * 10 + i;
  if(decimal){  
    scratch.decimal += 1;
  }
  write_scratch();
}

void
process_op(enum ops operation)
{
  if(op == NONE){
    op = operation;
    value = scratch;
  }
}

void handle_0(void)
{
 process_value(0);
}

void handle_1(void)
{
  process_value(1);
}

void handle_2(void)
{
  process_value(2);
}

void handle_3(void)
{
  process_value(3);
}

void handle_4(void)
{
  process_value(4);
}

void handle_5(void)
{
  process_value(5);
}

void handle_6(void)
{
  process_value(6);
}

void handle_7(void)
{
  process_value(7);
}

void handle_8(void)
{
  process_value(8);
}

void handle_9(void)
{
  process_value(9);
}

void handle_decimal(void)
{
  decimal = 1;
  scratch.decimal = decimal;
  write_scratch(); 
}

void handle_clear(void)
{
  decimal = 0;
  scratch.decimal = 0;
  scratch.val = 0;
  write_scratch();
  op = NONE;
}

void handle_neg_tog(void)
{
}

void handle_mod(void)
{
}


void handle_multi(void)
{
  process_op(MULT);
}
void handle_sub(void)
{
  process_op(SUB);
}

void handle_add(void)
{
  process_op(ADD);
}

void handle_div(void)
{
  process_op(DIV);
  return;
}

void handle_eq(void)
{
  process_op(EQ);
}



void
pop_calc_button(struct elmt* button, int width, int height, char * text, int textlen)
{
  button->width = width;
  button->height = height;
  button->x = 50;
  button->y = 50;
  button->main_fill = C_GRAY;
  button->main_alpha = 255;
  button->border = 2;  
  button->border_fill = C_BLUE;  
  button->border_alpha = 255;
  button->text = text;
  button->textlength = textlen;
  button->fontsize = 1;
  button->textalignment = 1;
  button->text_fill = C_BLACK;
  button->text_alpha = 255;

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

struct elmt* numbers_buttons[11];
event_handler number_handlers[11] = {&handle_0, &handle_1, &handle_2, &handle_3, &handle_4, &handle_5, &handle_6, &handle_7, &handle_8, &handle_9, &handle_decimal};
char *numbers[11] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "."};
struct elmt* tops_buttons[3];
char *tops[3] = {"AC", "+/-", "%"};
event_handler tops_handlers[3] = {&handle_clear, &handle_neg_tog, &handle_mod};
struct elmt* side_buttons[5];
char *sides[5] = {"/", "*", "-", "+", "="};
event_handler side_button_handlers[5] = {&handle_div, &handle_multi, &handle_sub, &handle_add, &handle_eq};


void
make_buttons(struct state* state)
{
  struct elmt* button;
  int button_width = 50;
  int button_height = 50;
  for(int i = 0; i < 11; i++){
    button = new_elmt(BUTTON);
    pop_calc_button(button, button_width, button_height, numbers[i], strlen(numbers[i]));
    button->mlc = number_handlers[i];
    numbers_buttons[i] = button;
    add_elmt(state, button);
  }  
  for(int i = 0; i < 3; i++){
    button = new_elmt(BUTTON);
    pop_calc_button(button, button_width, button_height, tops[i], strlen(tops[i]));
    button->mlc = tops_handlers[i];
    tops_buttons[i] = button;
    add_elmt(state, button);
  } 
  for(int i = 0; i < 5; i++){
    button = new_elmt(BUTTON);
    pop_calc_button(button, button_width, button_height, sides[i], strlen(sides[i]));
    button->mlc = side_button_handlers[i];
    side_buttons[i] = button;
    add_elmt(state, button);

  }

  int margin = 10;
  int text_x = 100;
  int text_y = 100;
  int text_height = 35;
  int top_x = text_x;
  int top_y = text_y + text_height + margin;
  int start_numbers_x = top_x;
  int start_numbers_y = top_y; 
  for(int but_x = 0; but_x < 3; but_x++){
    button = tops_buttons[but_x];
    button->x = start_numbers_x;
    button->y = start_numbers_y;
    start_numbers_x += button_width + margin;
  }
  start_numbers_x = top_x + 3*(button_width + margin);
  for(int but = 0; but < 5; but++){
      button = side_buttons[but];     
      button->x = start_numbers_x;
      button->y = start_numbers_y;
      start_numbers_y += button_height + margin;
  }
  start_numbers_x = top_x;
  start_numbers_y = top_y + button_height + margin; 
  int numidx = 1;
  for(int but_y = 0; but_y < 3; but_y++){
    start_numbers_x = top_x;
     for(int but_x = 0; but_x < 3; but_x++){
      button = numbers_buttons[numidx++];     
      button->x = start_numbers_x;
      button->y = start_numbers_y;
      start_numbers_x += button_width + margin;
    }
    start_numbers_y += button_height + margin;
  }  
  start_numbers_x = top_x;
  button = numbers_buttons[0];
  button->width = 2 * button->width + margin;
  button->x = start_numbers_x;
  button->y = start_numbers_y;
  start_numbers_x += 2*(button_width + margin);
  button = numbers_buttons[10];
  button->x = start_numbers_x;
  button->y = start_numbers_y;

  textbox = new_elmt(TEXTBOX);
  pop_textbox(textbox, text_x, text_y, 4*button_width + 3*margin, text_height);
  add_elmt(state, textbox);
}

void
test_float()
{
  struct c_float a = {.val = 51408248, .decimal = 5};
  struct c_float b = {.val = 617, .decimal = 0};
  struct c_float c = div(a , b); 
  char num[100];
  print_float(c, num);
  printf("c: %s\n", num);
}



int
main(void)
{
  test_float();
  char *c = "hello";
  struct gui* gui = init_gui();
  struct window* window1 = new_window(gui, 500, 500);  
  struct state* state1 = new_state();
  add_state(window1, state1);
  add_window(gui, window1);
  reverse(c, strlen(c));
  make_buttons(state1);
  handle_div();
  loop(gui, window1);
  exit(0);
}
