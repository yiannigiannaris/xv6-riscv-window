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
#include "virtio_gpu_cmds.h"

#define R(n, r) ((volatile uint32 *)(VIRTION(n) + (r)))

void initialize_display(int n);

struct virtioqueue {
  char pages[2*PGSIZE];
  
  struct VRingDesc *desc;
  struct VRingAvail *avail;
  struct UsedArea *used;

  // our own book-keeping.
  char free[NUM];  // is a descriptor free?
  int running[NUM];
  uint16 used_idx; // we've looked this far in used[2..NUM].
};

struct {
  struct virtioqueue controlq;
  struct virtioqueue cursorq;

  // initialized?
  int init;

  struct spinlock lock;
} __attribute__ ((aligned (PGSIZE))) gpu;

struct virtio_gpu_config gpu_config; 

void
virtio_gpu_get_config(int);

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
  gpu.controlq.avail = (struct VRingAvail*)(((char*)gpu.controlq.desc) + NUM*sizeof(struct VRingDesc));
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
  gpu.cursorq.avail = (struct VRingAvail*)(((char*)gpu.cursorq.desc) + NUM*sizeof(struct VRingDesc));
  gpu.cursorq.used = (struct UsedArea *) (gpu.cursorq.pages + PGSIZE);


  for(int i = 0; i < NUM; i++){
    gpu.controlq.free[i] = 1;
    gpu.cursorq.free[i] = 1;
  }

  gpu.init = 1;
  // plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
  virtio_gpu_get_config(n);
  initialize_display(n);
}

void
virtio_gpu_get_config(int n){
  gpu_config = *(struct virtio_gpu_config*)R(n, VIRTIO_MMIO_CONFIG); 
  /*
  printf("--gpu config--\n");
  printf("gpu events read: %d\n", gpu_config.events_read);
  printf("gpu events clear: %d\n", gpu_config.events_clear);
  printf("gpu num scanouts: %d\n", gpu_config.num_scanouts);
  printf("gpu reserved: %d\n", gpu_config.reserved);
  */
}
//
// mark a descriptor as free.
static void
free_desc(int is_controlq, int n)
{
  struct virtioqueue *q;
  if(is_controlq){
    q = &gpu.controlq;
  }else{
    q = &gpu.cursorq;
  }
  if(n >= NUM)
    panic("virtio_gpu free index");
  for(int i = 0; i < n; i++){
    if(q->free[i])
      panic("virtio_gpu free");
    q->desc[i].addr = 0;
    q->free[i] = 1;
  }
  wakeup(&q->free[0]);
}

static int
alloc_desc(int is_controlq, int n)
{
  if(n > NUM){
    panic("virtio gpu alloc too much");
  }
  struct virtioqueue *q;
  if(is_controlq){
    q = &gpu.controlq;
  }else{
    q = &gpu.cursorq;
  }
  for(int i = 0; i < n; i++){
    if(q->free[i]){
      q->free[i] = 0;
    } else {
      panic("virtio gpu alloc");
    }
  }
  return 0;
}

void
initialize_display(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(1, 2)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  printf("first while\n");
  if (descidx < 0)
    panic("virt gpu initialize display");

  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resp_display_info resp; 

  gpu.controlq.desc[descidx].addr = (uint64) kvmpa((uint64) &ctrl_hdr);
  gpu.controlq.desc[descidx].len = sizeof(struct virtio_gpu_ctrl_hdr);
  gpu.controlq.desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  gpu.controlq.desc[descidx].next = descidx + 1;

  gpu.controlq.desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  gpu.controlq.desc[descidx + 1].len = sizeof(resp);
  gpu.controlq.desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  gpu.controlq.desc[descidx + 1].next = 0;

  

  gpu.controlq.running[descidx] = 1;
  gpu.controlq.avail->ring[gpu.controlq.avail->idx % NUM] = descidx;  
  __sync_synchronize();
  gpu.controlq.avail->idx += 1;

  *R(n, VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number
  
  printf("used_idx: %d\n",gpu.controlq.used_idx);
  printf("used->id: %d\n",gpu.controlq.used->id);
  while((gpu.controlq.used_idx) == (gpu.controlq.used->id)){
  }

  printf("second while\n");
  struct VRingUsedElem used_elem = gpu.controlq.used->elems[gpu.controlq.used_idx % NUM];
  gpu.controlq.used_idx += 1;
  uint32 id = used_elem.id + 1;
  struct VRingDesc buf = gpu.controlq.desc[id];  
  printf("buf addr: %p\n", buf.addr); 
  printf("buf len: %d\n", buf.len); 
  resp = *(struct virtio_gpu_resp_display_info *)gpu.controlq.desc[id].addr;     
  int bytes = used_elem.len;
  printf("bytes: %d\n", bytes);
  if(resp.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO){
    printf("resp: %p\n",resp.hdr.type);
    panic("virt gpu initialize display"); 
  }
  free_desc(1,2);
  release(&gpu.lock);
}

void
virtio_gpu_intr(){
  acquire(&gpu.lock);

  while((gpu.controlq.used_idx % NUM) != (gpu.controlq.used->id % NUM)){
    int id = gpu.controlq.used->elems[gpu.controlq.used_idx].id;

    gpu.controlq.running[id] = 0;   // disk is done with buf
    wakeup(&gpu.controlq.running[id]);

    gpu.controlq.used_idx = (gpu.controlq.used_idx + 1) % NUM;
  }

  while((gpu.cursorq.used_idx % NUM) != (gpu.cursorq.used->id % NUM)){
    int id = gpu.cursorq.used->elems[gpu.cursorq.used_idx].id;

    gpu.cursorq.running[id] = 0;   // disk is done with buf
    wakeup(&gpu.cursorq.running[id]);

    gpu.cursorq.used_idx = (gpu.cursorq.used_idx + 1) % NUM;
  }

  release(&gpu.lock);
}
