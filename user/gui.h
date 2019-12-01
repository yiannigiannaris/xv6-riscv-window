#define MAX_TEXT 512
#define MAX_ELMTS 100
#define MAX_STATES 10

//Event handlers
typedef int (*mouse_left_click_handle)(void);
typedef int (*mouse_right_click_handle)(void);

struct state{
  struct elmt *head_elmt;
  uint elmt_count;
  struct state *next;
  struct state *prev;
};

struct window{
  struct state *state;
  uint64 frame_buffer;
  struct window* next;
  struct window* prev;
};

struct gui{
  struct window* window;
  uint64 ***fonts;
}

struct gui_window* new_window();

int add_elmt(struct gui_window*, int state, struct elmt*);
int modify_elmt(struct gui_window*, int state, struct elmt*);
int remove_elmt(struct gui_window*, int state, struct elmt*);
int add_state(struct gui_window*, struct state* state);

int start_event_loop(struct gui_window*, int state);
