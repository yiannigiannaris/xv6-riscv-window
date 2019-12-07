#define MAX_TEXT 512
#define MAX_ELMTS 100
#define MAX_STATES 10

typedef void (*event_handler)(int);

enum elmt_type {
  BUTTON,
  TEXTBOX,
  TEXTBLOCK
};

struct elmt{
  enum elmt_type type;
  event_handler mlc;
  event_handler mrc; 
  uint id;
  uint width;
  uint height;
  uint x;
  uint y;
  uint32 main_fill;
  uint8 main_alpha;
  int border;
  uint32 border_fill;
  uint8 border_alpha; 
  char *text;
  int textlength;
  int fontsize;
  int textalignment;
  uint32 text_fill;
  uint8 text_alpha;
  struct elmt *next;
  struct elmt *prev;
  int l_depressed;
  int r_depressed;
};

struct state{
  struct elmt *elmt;
  struct state *next;
  struct state *prev;
};

struct window{
  struct state *state;
  uint32* frame_buffer;
  int width;
  int height;
  int fd;
  struct window* next;
  struct window* prev;
};

struct gui{
  struct window* window;
  uint64 ***fonts;
};

struct gui* init_gui(void);
struct window* new_window(struct gui*, int, int);
struct window* new_applauncher(struct gui*, int, int);
void add_window(struct gui*, struct window*);
struct state* new_state();
void add_state(struct window*, struct state*);
void loop(struct gui*, struct window*);
void remove_elmt(struct state*, struct elmt*);
void add_elmt(struct state*, struct elmt*);
void switch_state(struct window*, struct state*);
struct elmt* new_elmt(enum elmt_type);
void modify_elmt(void);
