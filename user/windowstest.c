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

  int pid = fork();
  if(pid < 0){
    fprintf(2, "windowstest: fork() failed\n");
    exit(-1);
  }

  if(pid == 0){
    struct window_event *we = 0;
    int bytes;
    while(1){
      if((bytes = read(fd, (void*)we, sizeof(struct window_event))) > 0){
        if(we->kind == W_KIND_MOUSE){
          switch(we->m_event.type){
            case W_CUR_MOVE_ABS:
              printf("YELLOW      W_CUR_MOVE_ABS: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);
              break;
            case W_LEFT_CLICK_PRESS:
              printf("YELLOW      W_LEFT_CLICK_PRESSS: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);
              break;
            case W_LEFT_CLICK_RELEASE:
              printf("YELLOW      W_LEFT_CLICK_RELEASE: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);
              break;
            default:
              printf("YELLOW      EVENT NOT RECOGNIZED: timestamp=%d, type=%d, xval=%d, yval=%d\n",we->timestamp, we->m_event.type, we->m_event.xval, we->m_event.yval);
          }
        } else {
          printf("YELLOW      KEYBOARD: timestamp=%d, character=%c\n",we->timestamp, we->k_event.ascii_val);
        }
      }
    }
  } else {
    struct window_event *we = 0;
    int bytes;
    while(1){
      if((bytes = read(fd2, (void*)we, sizeof(struct window_event))) > 0){
        if(we->kind == W_KIND_MOUSE){
          switch(we->m_event.type){
            case W_CUR_MOVE_ABS:
              printf("PURPLE      W_CUR_MOVE_ABS: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);
              break;
            case W_LEFT_CLICK_PRESS:
              printf("PURPLE      W_LEFT_CLICK_PRESSS: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);
              break;
            case W_LEFT_CLICK_RELEASE:
              printf("PURPLE      W_LEFT_CLICK_RELEASE: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);
              break;
            default:
              printf("PURPLE      EVENT NOT RECOGNIZED: timestamp=%d, type=%d, xval=%d, yval=%d\n",we->timestamp, we->m_event.type, we->m_event.xval, we->m_event.yval);
          }
        } else{
          printf("PURPLE      KEYBOARD: timestamp=%d, character=%c\n",we->timestamp, we->k_event.ascii_val);
        }
      }
    }
  }
  exit(0);
}

