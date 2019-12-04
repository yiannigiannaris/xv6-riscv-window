#define MOUSE_SYN 0
#define MOUSE_REL 1
#define MOUSE_LEFT_CLICK_PRESS 2
#define MOUSE_LEFT_CLICK_RELEASE 3
#define MOUSE_RIGHT_CLICK_PRESS 4
#define MOUSE_RIGHT_CLICK_RELEASE 5


#define DEV_MOUSE 0
#define DEV_KEYBOARD 1

struct mouse_event {
  int type;
  int xval;
  int yval;
};

struct keyboard_event {
  int code;
  int val;
};

struct input_event {
  uint timestamp;
  uint8 dev;
  union {
    struct mouse_event m_event;
    struct keyboard_event k_event;
  };
};
