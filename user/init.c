// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv_win[] = { "windowstest", 0 };
char *argv_hand[] = {"input_handler", 0};

int
main(void)
{
  //int pid, wpid;
  int pid, pid2, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
   printf("init: starting sh\n");
	 pid = fork();
	 if(pid < 0){
     printf("fork 1 failed\n");
	   exit(1);
	 }
	 if(pid == 0){
	   exec("windowstest", argv_win);
	   printf("init: exec sh failed\n");
	   exit(1);
	 }
   pid2 = fork();
   if(pid2 < 0){
     printf("fork 2 failed\n");
     exit(1);
   }
   if(pid2 == 0){
     exec("input_handler", argv_hand);
     printf("start input handler failed\n");
     exit(1);
   }
 	 while((wpid=wait(0)) >= 0 && wpid != pid){
	   printf("zombie!\n");
	 }    
   printf("end of main\n");
  }
}
