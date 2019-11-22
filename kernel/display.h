#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 1024
#define CURSOR_WIDTH 64
#define CURSOR_HEIGHT 64
#define CURSOR_START_X (FRAME_WIDTH / 2)
#define CURSOR_START_Y (FRAME_HEIGHT / 2)

struct cursor {
  int xpos;
  int ypos;
  int height;
  int width;
  struct spinlock lock;
  uint32 *frame_buf;
};

struct frame {
  int height;
  int width;
  struct spinlock lock;
  uint32 *frame_buf;
};

