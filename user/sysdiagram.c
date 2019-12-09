#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"
#include "kernel/window_event.h"

int
main(void)
{
  int fd;
  uint32 *frame_buf = (uint32*)mkdiagram(&fd);
  int width = 800;
  int height = 600;
  updatewindow(fd, width, height);
  printf("%p\n", frame_buf);
  struct window_event event;
  while(read(fd, &event, sizeof(struct window_event) > 0)){

  }

  exit(0);
}
