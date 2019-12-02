#define MOUSE_SYN 0
#define MOUSE_REL 1
#define MOUSE_LEFT_CLICK_PRESS 2
#define MOUSE_LEFT_CLICK_RELEASE 3
#define MOUSE_RIGHT_CLICK_PRESS 4
#define MOUSE_RIGHT_CLICK_RELEASE 5


struct mouse_event {
  int type;
  int xval;
  int yval;
};
