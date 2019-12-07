// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv_win[] = { "windowstest", 0 };
char *argv_hand[] = {"input_handler", 0};
char *argv_calc[] = {"calc", 0};
char *argv_applauncher[] = {"applauncher", 0};
char *argv_ls[] = {"ls", 0};

int
main(void)
{
  int pid1, pid2; //, pid3, pid4;

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
    printf("init: starting applauncher\n");
    exec("applauncher", argv_applauncher);
    exit(0);
  }
  while(1){};
  exit(0);
}
