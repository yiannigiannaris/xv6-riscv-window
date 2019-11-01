#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "virtio.h"

#define R(n, r) ((volatile uint32 *)(VIRTION(n) + (r)))

struct virtioqueue {
  char pages[2*PGSIZE];
  
  struct VRingDesc *desc;
  uint16 *avail;
  struct UsedArea *used;

  // our own book-keeping.
  char free[NUM];  // is a descriptor free?
  uint16 used_idx; // we've looked this far in used[2..NUM].
};

struct {
  struct virtioqueue controlq;
  struct virtioqueue cursorq;


  // track info about in-flight operations,
  // for use when completion interrupt arrives.
  // indexed by first descriptor index of chain.
  struct {
    struct buf *b;
    char status;
  } info[NUM];

  // initialized?
  int init;

  struct spinlock lock;
} __attribute__ ((aligned (PGSIZE))) gpu;



void
virtio_gpu_init(int n)
{
  uint32 status = 0;

  __sync_synchronize();
  if(gpu.init)
    return;

  printf("virtio gpu init %d\n", n);
  
  initlock(&gpu.lock, "virtio_gpu");

  if(*R(n, VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
     *R(n, VIRTIO_MMIO_VERSION) != 1 ||
     *R(n, VIRTIO_MMIO_DEVICE_ID) != 16){
    panic("could not find virtio disk");
  }

  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  // negotiate features
  uint64 features = *R(n, VIRTIO_MMIO_DEVICE_FEATURES);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  *R(n, VIRTIO_MMIO_DRIVER_FEATURES) = features;

  // tell device that feature negotiation is complete.
  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  // tell device we're completely ready.
  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  *R(n, VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;

  // initialize control queue.
  *R(n, VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = *R(n, VIRTIO_MMIO_QUEUE_NUM_MAX);
  if(max == 0)
    panic("virtio disk has no queue 0");
  if(max < NUM)
    panic("virtio disk max queue too short");
  *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM;
  memset(gpu.controlq.pages, 0, sizeof(gpu.controlq.pages));
  *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)gpu.controlq.pages) >> PGSHIFT;
  //
  // desc = pages -- num * VRingDesc
  // avail = pages + 0x40 -- 2 * uint16, then num * uint16
  // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem

  gpu.controlq.desc = (struct VRingDesc *) gpu.controlq.pages;
  gpu.controlq.avail = (uint16*)(((char*)gpu.controlq.desc) + NUM*sizeof(struct VRingDesc));
  gpu.controlq.used = (struct UsedArea *) (gpu.controlq.pages + PGSIZE);
  
  // initialize cursor queue.
  *R(n, VIRTIO_MMIO_QUEUE_SEL) = 1;
  max = *R(n, VIRTIO_MMIO_QUEUE_NUM_MAX);
  if(max == 0)
    panic("virtio disk has no queue 0");
  if(max < NUM)
    panic("virtio disk max queue too short");
  *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM;
  memset(gpu.cursorq.pages, 0, sizeof(gpu.cursorq.pages));
  *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)gpu.cursorq.pages) >> PGSHIFT;

  gpu.cursorq.desc = (struct VRingDesc *) gpu.cursorq.pages;
  gpu.cursorq.avail = (uint16*)(((char*)gpu.cursorq.desc) + NUM*sizeof(struct VRingDesc));
  gpu.cursorq.used = (struct UsedArea *) (gpu.cursorq.pages + PGSIZE);


  for(int i = 0; i < NUM; i++){
    gpu.controlq.free[i] = 1;
    gpu.cursorq.free[i] = 1;
  }

  gpu.init = 1;
  // plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
}





