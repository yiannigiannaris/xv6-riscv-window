#define MAX_WINDOWS 10

struct window {
  int width;
  int height;
  int xpos;
  int ypos;
  uint32 *frame_buf;
  struct spinlock lock;
  int rfd;
  struct file *wf;
  struct window *next;
  struct window *prev;
}
