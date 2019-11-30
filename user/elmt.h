enum elmt_type {
  BUTTON,
  TEXTBOX,
  TEXTBLOCK
};

struct elmt{
  enum elmt_type type;
  typedef int (*mouse_left_click_handle)(void);
  typedef int (*mouse_right_click_handle)(void);
  uint uid;
  uint width;
  uint height;
  uint x;
  uint y;
  uint32 fill;
  int border;
  uint32 border_width;
  char text[MAX_TEXT];
  struct elmt *next;
  struct elmt *prev;
};
