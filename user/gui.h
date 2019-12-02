#define MAX_TEXT 512
#define MAX_ELMTS 100
#define MAX_STATES 10

typedef int (*event_handler)(void);

enum elmt_type {
  BUTTON,
  TEXTBOX,
  TEXTBLOCK
};

struct elmt{
  enum elmt_type type;
  event_handler mlc;
  event_handler mrc; 
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

struct state{
  struct elmt *head_elmt;
  uint elmt_count;
  struct state *next;
  struct state *prev;
};

struct window{
  struct state *state;
  uint32* frame_buffer;
  int fd;
  struct window* next;
  struct window* prev;
};

struct gui{
  struct window* window;
  uint64 ***fonts;
};

struct window* new_window();

int add_elmt(struct state*, struct elmt*);
int remove_elmt(struct elmt*);
int add_state(struct window*, struct state*);
int switch_state(struct window*, struct state*);

int start_event_loop(struct window*, int state);
struct gui* init_gui(void);
struct window* new_window(struct gui*);
