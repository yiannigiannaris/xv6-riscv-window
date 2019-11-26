struct window {
  int width;
  int height;
  int xpos;
  int ypos;
  uint32 *frame_buf;
  struct spinlock lock;
  struct file *rf;
  struct file *wf;
  struct window *next;
  struct window *prev;
  int free;
};
