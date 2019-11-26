#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"

int
main(void)
{
  int fd;
  printf("&fd=%p\n", &fd);
  uint32 *frame_buf = (uint32*)mkwindow(&fd);
  printf("frame_buf=%p\n", frame_buf);
  printf("user: fd=%d\n", fd);
  int width = 400;
  int height = 300;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      *(frame_buf + y * width + x) = 0xFFFF00FF;
    }
  }
  /*printf("n=%d\n", sizeof(uint32) * width * height);*/
  /*memset(frame_buf, sizeof(uint32) * width * height, 0xff);*/
  updatewindow(fd, width, height);
  printf("windowstest complete\n");
  exit(0);
}

