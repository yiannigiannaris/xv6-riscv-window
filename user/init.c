// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "graphicstest", 0 };

int
main(void)
{
  //int pid, wpid;
  int pid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  //for(;;){
  printf("init: starting sh\n");
  pid = fork();
  if(pid < 0){
    printf("init: fork failed\n");
    exit(1);
  }
  if(pid == 0){
    printf("init: exec sh failed\n");
    while (1){}
    exit(1);
  }
  //while((wpid=wait(0)) >= 0 && wpid != pid){
    //printf("zombie!\n");
  //}
  while (1){}
  //}
}
