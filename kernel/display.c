#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "cursor.h"
#include "file.h"
#include "proc.h"
#include "stat.h"

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 1024
#define CURSOR_WIDTH 64
#define CURSOR_HEIGHT 64

uint8 *frame_array;
uint8 *cursor_array;

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];
  printf("after\n");
  if((dp = nameiparent(path, name)) == 0)
    return 0;
  printf("after\n");
  ilock(dp);
  printf("after\n");
  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }
  printf("after\n");
  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);
  printf("after\n");
  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }
  printf("after\n");
  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);
  printf("after\n");
  return ip;
}

void
init_cursor()
{
  cursor_array = kdisplaymem();
  frame_array = cursor_array + CURSOR_DATA_SIZE;
  printf("before\n");
  struct inode *ip = create("/cursorbytes", T_FILE, 0, 0);
  printf("after\n");
  readi(ip, 0, (uint64)cursor_array, 0, CURSOR_DATA_SIZE);
}