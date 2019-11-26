#define MAX_TEXT 512
#define MAX_ELMTS 100
//Event handlers
typedef int (*mouse_left_click_handle)(void);
typedef int (*mouse_right_click_handle)(void);

struct elmt{
  enum {BUTTON, TEXTBOX} type;
  mouse_left_click_handle mouse_left;
  mouse_right_click_handle mouse_right;
  uint uid;
  uint width;
  uint height;
  uint x;
  uint y;
  char text[MAX_TEXT];
};

struct gui_window{
  elmt elmts[MAX_ELMTS];
  uint elmt_count;
  uint64 frame_buffer;
};

int add_elmt(struct gui_window*, struct elmt*);
int modify_elmt(struct gui_window*, struct elmt*);
int remove_elmt(struct gui_window*, struct elmt*);

int start_event_loop(struct gui_window*);
