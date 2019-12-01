struct cursor {
  int xpos;
  int ypos;
  int height;
  int width;
  int left_pressed;
  int right_pressed;
  struct spinlock lock;
  uint32 *frame_buf;
};
