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
#define RECTTEST 20
#define NUMQS 2

void initialize_display(int n);
void create_send_rectangle(int);

struct virtioqueue {
  char pages[2*PGSIZE];
  
  struct VRingDesc *desc;
  struct VRingAvail *avail;
  struct UsedArea *used;

  // our own book-keeping.
  char free[NUM];  // is a descriptor free?
  int running[NUM];
  int id;
  uint16 used_idx; // we've looked this far in used[2..NUM].
};

struct {
 struct virtioqueue queues[NUMQS];
  //struct virtioqueue controlq;
  //struct virtioqueue cursorq;

  // initialized?
  int init;

  struct spinlock lock;
  char framebuffer[2*PGSIZE];
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
  struct virtioqueue *queue;
  for(int i = 0; i < NUMQS; i++){
    queue = &gpu.queues[i];
    queue->id = i;
    printf("queue id given: %d\n", queue->id);
    *R(n, VIRTIO_MMIO_QUEUE_SEL) = i;
    uint32 max = *R(n, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if(max == 0)
      panic("virtio disk has no queue 0");
    if(max < NUM)
      panic("virtio disk max queue too short");
    *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM;
    memset(queue->pages, 0, sizeof(queue->pages));
    *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)queue->pages) >> PGSHIFT;
    //
    // desc = pages -- num * VRingDesc
    // avail = pages + 0x40 -- 2 * uint16, then num * uint16
    // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem

    queue->desc = (struct VRingDesc *) queue->pages;
    queue->avail = (struct VRingAvail*)(((char*)queue->desc) + NUM*sizeof(struct VRingDesc));
    queue->used = (struct UsedArea *) (queue->pages + PGSIZE);

    for(int j = 0; j < NUM; j++){
      queue->free[j] = 1;
    }
  }

  gpu.init = 1;
  // plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
  
  virtio_gpu_get_config(n);
  initialize_display(n);
  uint32 rgba_value = 0;
  //rgba_value = rgba_value | ((uint64)255 << 24);
  //printf("rgba_value: %p\n", rgba_value);
  //rgba_value = rgba_value | ((uint32)255 << 24) | ((uint32)255 << 16) | ((uint32)255 << 8) | ((uint32)255);
  rgba_value = rgba_value | ((uint32)255 << 24) | ((uint32)0 << 16) | ((uint32)0 << 8) | (uint32)255;
  printf("rgba_value: %d\n", rgba_value);
  uint32 *fb = (uint32*)gpu.framebuffer;
  for(;fb < (uint32*)gpu.framebuffer + (RECTTEST*RECTTEST*4);fb++){
    *fb = rgba_value;
  }
  create_send_rectangle(n);
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
free_desc(int queue, int descidx, int n)
{
  struct virtioqueue *q = &gpu.queues[queue];
  if(n >= NUM)
    panic("virtio_gpu free index");
  for(int i = descidx; i < n; i++){
    if(q->free[i])
      panic("virtio_gpu free");
    q->desc[i].addr = 0;
    q->free[i] = 1;
  }
  wakeup(&q->free[0]);
}

static int
alloc_desc(int queue, int n)
{
  if(n > NUM){
    panic("virtio gpu alloc too much");
  }
  struct virtioqueue *q = &gpu.queues[queue];
  for(int i = 0; i < n; i++){
    if(q->free[i]){
      q->free[i] = 0;
    } else {
      panic("virtio gpu alloc");
    }
  }
  return 0;
}

uint64
read_response(int q, int respidx){
  struct VRingUsedElem used_elem; 
  struct virtioqueue *queue = &gpu.queues[q];
  used_elem = queue->used->elems[queue->used_idx % NUM];
  queue->used_idx += 1;
  uint32 id = used_elem.id + respidx;
  return queue->desc[id].addr;   
}

void
send_on_queue(int n, int q, int descidx){
  struct virtioqueue *queue = &gpu.queues[q];
  queue->running[descidx] = 1;
  queue->avail->ring[queue->avail->idx % NUM] = descidx;  
  __sync_synchronize();
  queue->avail->idx += 1;

  *R(n, VIRTIO_MMIO_QUEUE_NOTIFY) = q; // value is queue->number
  /*
  printf("used_idx: %d\n",queue->used_idx);
  printf("used->id: %d\n",queue->used->id);
  while((queue->used_idx) == (queue->used->id)){
  }
  */
  printf("before sleep\n");
  sleep(queue, &gpu.lock);
  printf("after sleep\n");
}
void
initialize_display(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(0, 2)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  if (descidx < 0)
    panic("virt gpu initialize display");
  struct virtioqueue *queue = &gpu.queues[0];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resp_display_info resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &ctrl_hdr);
  queue->desc[descidx].len = sizeof(struct virtio_gpu_ctrl_hdr);
  queue->desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(n, 0, descidx);
  resp = *(struct virtio_gpu_resp_display_info *)read_response(0,1);     
  if(resp.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO){
    panic("virt gpu initialize display"); 
  }
  free_desc(0, descidx, 2);
  release(&gpu.lock);
}

void
create_resource(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(0, 2)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  printf("first while\n");
  if (descidx < 0)
    panic("virt gpu initialize display");
  struct virtioqueue *queue = &gpu.queues[0];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resource_create_2d request =
  {
    .hdr = ctrl_hdr,
    .resource_id = 1,
    .format = VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM, 
    .width = RECTTEST,
    .height = RECTTEST,
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  

  send_on_queue(n, 0, descidx);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(0,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu create_resource display"); 
  }
  free_desc(0, descidx, 2);
   
  release(&gpu.lock);

}

void
attach_frame_buffer(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(0, 3)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  printf("first while\n");
  if (descidx < 0)
    panic("virt gpu initialize display");
  struct virtioqueue *queue = &gpu.queues[0];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resource_attach_backing request1 =
  {
    .hdr = ctrl_hdr,
    .resource_id = 1,
    .nr_entries = 1,
  };
  struct virtio_gpu_mem_entry request2 =
  {
    .addr = (uint64)kvmpa((uint64)gpu.framebuffer),
    .length = RECTTEST*RECTTEST*4,
    .padding = 0,
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request1);
  queue->desc[descidx].len = sizeof(request1);
  queue->desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &request2);
  queue->desc[descidx + 1].len = sizeof(request2);
  queue->desc[descidx + 1].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx + 1].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx + 1].next = descidx + 2;

  queue->desc[descidx + 2].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 2].len = sizeof(resp);
  queue->desc[descidx + 2].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 2].next = 0;

  

  send_on_queue(n, 0, descidx);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(0,2);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu attach frame buffer"); 
  }
  free_desc(0, descidx, 3);
   
  release(&gpu.lock);
}

void
set_scanout(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(0, 2)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  printf("first while\n");
  if (descidx < 0)
    panic("virt gpu initialize display");

  struct virtioqueue *queue = &gpu.queues[0];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_SET_SCANOUT,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_rect rect =
  {
    .x = 0,
    .y = 0,
    .width = RECTTEST,
    .height = RECTTEST
  };

  struct virtio_gpu_set_scanout request =
  {
    .hdr = ctrl_hdr,
    .r = rect,
    .scanout_id = 0,
    .resource_id = 1
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  

  send_on_queue(n, 0, descidx);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(0,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu set scanout"); 
  }
  free_desc(0, descidx, 2);
   
  release(&gpu.lock);
}
void
transfer_to_host(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(0, 2)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  printf("first while\n");
  if (descidx < 0)
    panic("virt gpu initialize display");

  struct virtioqueue *queue = &gpu.queues[0];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_rect rect =
  {
    .x = 0,
    .y = 0,
    .width = RECTTEST,
    .height = RECTTEST
  };

  struct virtio_gpu_transfer_to_host_2d request =
  {
    .hdr = ctrl_hdr,
    .r = rect,
    .offset = 0,
    .resource_id = 1,
    .padding = 0
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  

  send_on_queue(n, 0, descidx);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(0,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu transfer to host"); 
  }
  free_desc(0, descidx, 2);
   
  release(&gpu.lock);
}

void
flush(int n){
  acquire(&gpu.lock);
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(0, 2)) >= 0) {
      break;
    }
    //sleep(&gpu.controlq.free[0], &gpu.lock);
  } 
  printf("first while\n");
  if (descidx < 0)
    panic("virt gpu initialize display");

  struct virtioqueue *queue = &gpu.queues[0];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_RESOURCE_FLUSH,
      .flags = 0,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_rect rect =
  {
    .x = 0,
    .y = 0,
    .width = RECTTEST,
    .height = RECTTEST
  };

  struct virtio_gpu_resource_flush request =
  {
    .hdr = ctrl_hdr,
    .r = rect,
    .resource_id = 1,
    .padding = 0
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VIRTIO_GPU_FLAG_FENCE;
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  

  send_on_queue(n, 0, descidx);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(0,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu flush"); 
  }
  free_desc(0, descidx, 2);
   
  release(&gpu.lock);
}
void create_send_rectangle(int n){
  printf("create resource\n");
  create_resource(n);
  printf("attach frame buffer\n");
  attach_frame_buffer(n);
  printf("set_scanout\n");
  set_scanout(n);
  printf("transfer to host\n");
  transfer_to_host(n);
  printf("flush\n");
  flush(n);
}

void
virtio_gpu_intr(){
  acquire(&gpu.lock);

  struct virtioqueue queue = gpu.queues[0];
  while((queue.used_idx % NUM) != (queue.used->id % NUM)){
    int id = queue.used->elems[queue.used_idx].id;

    queue.running[id] = 0;   // disk is done with buf
    wakeup(&queue.running[id]);

    queue.used_idx = (queue.used_idx + 1) % NUM;
  }

  queue = gpu.queues[1];
  while((queue.used_idx % NUM) != (queue.used->id % NUM)){
    int id = queue.used->elems[queue.used_idx].id;

    queue.running[id] = 0;   // disk is done with buf
    wakeup(&queue.running[id]);

    queue.used_idx = (queue.used_idx + 1) % NUM;
  }

  release(&gpu.lock);
}
