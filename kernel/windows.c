#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "cursor.h"
#include "proc.h"
#include "display.h"
#include "colors.h"
#include "windows.h"

struct spinlock windows_lock;
struct window *head;
struct window *tail;
struct window windows[MAX_WINDOWS];

int
make_window()
{
  struct proc *p = myproc();
}

