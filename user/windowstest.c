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
  uint32 *frame_buf = (uint32*)mkwindow(&fd);
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

  int fd2;
  uint32 *frame_buf2 = (uint32*)mkwindow(&fd2);
  int width2 = 500;
  int height2 = 200;
  for(int y = 0; y < height2; y++){
    for(int x = 0; x < width2; x++){
      *(frame_buf2 + y * width2 + x) = 0xAA08FFFF;
    }
  }

  updatewindow(fd2, width2, height2);

  while(1){};
  exit(0);
}

