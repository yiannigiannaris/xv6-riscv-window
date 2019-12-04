#define W_KIND_MOUSE 0
#define W_KIND_KEYBOARD 1
#define W_SYN 0
#define W_CUR_MOVE_ABS 1
#define W_LEFT_CLICK_PRESS 2
#define W_LEFT_CLICK_RELEASE 3
#define W_RIGHT_CLICK_PRESS 4
#define W_RIGHT_CLICK_RELEASE 5


struct window_mouse_event {
  int type;
  int xval;
  int yval;
};

struct window_keyboard_event {
  uint8 ascii_val;
};

struct window_event {
  uint timestamp;
  uint8 kind;
  union {
    struct window_mouse_event m_event;
    struct window_keyboard_event k_event;
  };
};
