#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "window_event.h"

#define PIPESIZE 202

struct w_pipe {
  struct spinlock lock;
  struct window_event data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
w_pipealloc(struct file **f0, struct file **f1)
{
  struct w_pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct w_pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "w_pipe");
  (*f0)->type = FD_W_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->w_pipe = pi;
  (*f1)->type = FD_W_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->w_pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
w_pipeclose(struct w_pipe *pi, int writable)
{
  acquire(&pi->lock);
  if(writable){
    pi->writeopen = 0;
  } else {
    pi->readopen = 0;
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    kfree((char*)pi);
  } else
    release(&pi->lock);
}

int
w_pipewrite(struct w_pipe *pi, uint64 addr)
{
  struct window_event we = *(struct window_event*)addr;
  acquire(&pi->lock);
  if(pi->nwrite == pi->nread + PIPESIZE){  //DOC: pipewrite-full
    wakeup(&pi->nread);
    release(&pi->lock);
    return -1;
  }
  pi->data[pi->nwrite++ % PIPESIZE] = we;
  wakeup(&pi->nread);
  release(&pi->lock);
  return sizeof(struct window_event);
}

int
w_piperead(struct w_pipe *pi, uint64 addr)
{
  struct window_event we;
  struct proc *pr = myproc();

  acquire(&pi->lock);
  while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
    if(myproc()->killed){
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock); //DOC: piperead-sleep
  }
  if(pi->nread == pi->nwrite){
    release(&pi->lock);
    return 0;
  }
  we = pi->data[pi->nread++ % PIPESIZE];
  if(copyout(pr->pagetable, addr, (char*)&we, sizeof(struct window_event)) == -1){
    release(&pi->lock);
    return -1;
  }
  wakeup(&pi->nwrite);  //DOC: piperead-wakeup
  release(&pi->lock);
  return sizeof(struct window_event);
}

