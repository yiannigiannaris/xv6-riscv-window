struct frame {
  int height;
  int width;
  struct spinlock lock;
  uint32 *frame_buf;
};
