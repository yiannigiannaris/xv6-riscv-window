#define MAX_TEXT 512
#define MAX_ELMTS 100
#define MAX_STATES 10

//Event handlers
typedef int (*mouse_left_click_handle)(void);
typedef int (*mouse_right_click_handle)(void);

struct state{
  struct elmt *elmts[MAX_ELMTS];

};

struct gui_window{
  struct state *states[MAX_STATES];
  uint elmt_count;
  uint64 frame_buffer;
};

int add_elmt(struct gui_window*, int state, struct elmt*);
int modify_elmt(struct gui_window*, int state, struct elmt*);
int remove_elmt(struct gui_window*, int state, struct elmt*);

int start_event_loop(struct gui_window*, int state);
