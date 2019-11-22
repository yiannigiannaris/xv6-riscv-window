#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 1024
#define CURSOR_WIDTH 64
#define CURSOR_HEIGHT 64

struct cursor {
  int xpos;
  int ypos;
  int height;
  int width;
  struct sleeplock lock;
  uint32 *frame_buf;
};

struct frame {
  int height;
  int width;
  struct sleeplock lock;
  uint32 *frame_buf;
};

