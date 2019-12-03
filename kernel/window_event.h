#define W_SYN 0
#define W_CUR_MOVE_ABS 1
#define W_LEFT_CLICK_PRESS 2
#define W_LEFT_CLICK_RELEASE 3
#define W_RIGHT_CLICK_PRESS 4
#define W_RIGHT_CLICK_RELEASE 5


struct window_event {
  int type;
  int xval;
  int yval;
};
