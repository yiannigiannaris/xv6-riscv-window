// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv_win[] = { "windowstest", 0 };
char *argv_hand[] = {"input_handler", 0};
char *argv_calc[] = {"calc", 0};

int
main(void)
{
  int pid1, pid2, pid3;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  pid1 = fork();
  if(pid1 < 0){
    printf("init: fork 1 failed\n");
    exit(1);
  }
  if(pid1 == 0){
    printf("init: starting input handler\n");
    exec("input_handler", argv_hand);
    printf("init: exec input handler failed\n");
    exit(1);
  }  
  
  pid2 = fork();
  if(pid2 < 0){
    printf("init: fork 2 failed\n");
    exit(1);
  }
  if(pid2 == 0){
    sleep(5);
    printf("init: starting windows test\n");
    exec("windowstest", argv_win);
    exit(0);
  }

  pid3 = fork();
  if(pid3 < 0){
    printf("init: fork 3 failed\n");
    exit(1);
  }
  if(pid3 == 0){
    sleep(10);
    printf("init: starting calc\n");
    exec("calc", argv_calc);
    exit(0);
  }

  while(1){};
  exit(0);

  /*pid = fork();*/
  /*if(pid < 0){*/
    /*printf("fork 1 failed\n");*/
    /*exit(1);*/
  /*}*/
  /*if(pid == 0){*/
    /*printf("user start input handler\n");*/
    /*exec("input_handler", argv_hand);*/
    /*printf("start input handler failed\n");*/
    /*exit(1);*/
  /*}*/
  /*pid2 = fork();*/
  /*if(pid2 < 0){*/
    /*printf("fork 2 failed\n");*/
    /*exit(1);*/
  /*}*/
  /*if(pid2 == 0){*/
    /*exec("windowstest", argv_win);*/
    /*while(1){};*/
    /*exit(1);*/
  /*}*/
  /*while((wpid=wait(0)) >= 0 && wpid != pid){*/
    /*printf("zombie!\n");*/
  /*}    */
  /*printf("end of main\n");*/
  /*exit(-1);*/
}
