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
#include "display.h"

#define R(n, r) ((volatile uint32 *)(VIRTION(n) + (r)))
#define NUMQS 2
#define FRAME_BUFFER_RESOURCE_ID 1
#define CURSOR_RESOURCE_ID 2

void initialize_display();
void initialize_cursor();
int DEVICE_NO;

struct virtioqueue {
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
  char control_pages[2*PGSIZE];
  char cursor_pages[2*PGSIZE];
  //struct virtioqueue *control_queue;
  //struct virtioqueue *cursor_queue;
  struct virtioqueue queues[NUMQS];
  // initialized?
  int init;

  struct spinlock lock;
  uint32 *framebuffer;
} __attribute__ ((aligned (PGSIZE))) gpu;

struct virtio_gpu_config gpu_config; 

void
virtio_gpu_get_config();

void
virtio_gpu_init(int n)
{
  uint32 status = 0;
  DEVICE_NO = 1;
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
    printf("max queue size: %d\n", max);
    if(max == 0)
      panic("virtio disk has no queue 0");
    if(max < NUM)
      panic("virtio disk max queue too short");
    *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM;

    if(i == 0){
      *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)gpu.control_pages) >> PGSHIFT;
      memset(gpu.control_pages, 0, sizeof(gpu.control_pages));
      queue->desc = (struct VRingDesc *) gpu.control_pages;
      queue->avail = (struct VRingAvail*)(((char*)queue->desc) + NUM*sizeof(struct VRingDesc));
      queue->used = (struct UsedArea *) (gpu.control_pages+ PGSIZE);
    } else{
      *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)gpu.cursor_pages) >> PGSHIFT;
      memset(gpu.cursor_pages, 0, sizeof(gpu.cursor_pages));
      queue->desc = (struct VRingDesc *) gpu.cursor_pages;
      queue->avail = (struct VRingAvail*)(((char*)queue->desc) + NUM*sizeof(struct VRingDesc));
      queue->used = (struct UsedArea *) (gpu.cursor_pages+ PGSIZE);
    }

    for(int j = 0; j < NUM; j++){
      queue->free[j] = 1;
    }
    *R(n, VIRTIO_MMIO_QUEUE_READY) = 1;

  }

  gpu.init = 1;
  virtio_gpu_get_config();
}

void
virtio_gpu_get_config(){
  gpu_config = *(struct virtio_gpu_config*)R(DEVICE_NO, VIRTIO_MMIO_CONFIG); 
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

static int
alloc_desc_wait(int q, int n)
{
  int descidx = -1;
  while(1){
    if((descidx = alloc_desc(q, n)) >= 0) {
      break;
    }
    sleep(&gpu.queues[q], &gpu.lock);
  } 
  return descidx; 
}

uint64
read_response(int q, int respidx){
  struct VRingUsedElem used_elem; 
  struct virtioqueue *queue = &gpu.queues[q];
  used_elem = queue->used->elems[queue->used_idx % NUM];
  queue->used_idx += 1;
  uint32 id = used_elem.id + respidx;
  *R(DEVICE_NO, VIRTIO_MMIO_INTERRUPT_ACK) = 1;
  return queue->desc[id].addr;   
}

void
send_on_queue(int q, int descidx, int should_sleep){
  struct virtioqueue *queue = &gpu.queues[q];
  queue->running[descidx] = 1;
  queue->avail->ring[queue->avail->idx % NUM] = descidx;  
  __sync_synchronize();
  queue->avail->idx += 1;

  *R(DEVICE_NO, VIRTIO_MMIO_QUEUE_NOTIFY) = q; // value is queue->number
  while((queue->used_idx) == (queue->used->id)){
    if(should_sleep)
      sleep(&gpu.queues[q], &gpu.lock);
  }
}

void
get_display_info(int q, int flags, int should_sleep)
{
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu get display info");
  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
      .flags = 0 | flags,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resp_display_info resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &ctrl_hdr);
  queue->desc[descidx].len = sizeof(struct virtio_gpu_ctrl_hdr);
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_resp_display_info *)read_response(q,1);     
  if(resp.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO){
    panic("virt gpu get display info"); 
  }
  free_desc(q, descidx, 2);
  release(&gpu.lock);
}

void
create_resource(int q, int frame_width, int frame_height, int resource_id, uint32 flags, int should_sleep){
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu create resource");
  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
      .flags = 0 | flags,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resource_create_2d request =
  {
    .hdr = ctrl_hdr,
    .resource_id = resource_id,
    .format = VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM,
    .width = frame_width,
    .height = frame_height,
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags |= VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;
  

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu create_resource"); 
  }
  free_desc(q, descidx, 2);
   
  release(&gpu.lock);

}

void
attach_frame_buffer(int q, uint64 fb, int frame_width, int frame_height, int resource_id, uint32 flags, int should_sleep){
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,3)) < 0)
    panic("virt gpu attach frame buffer");
  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
      .flags = 0 | flags,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_resource_attach_backing request1 =
  {
    .hdr = ctrl_hdr,
    .resource_id = resource_id,
    .nr_entries = 1,
  };
  struct virtio_gpu_mem_entry request2 =
  {
    .addr = (uint64)kvmpa((uint64)fb),
    .length = frame_width*frame_height*4,
    .padding = 0,
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request1);
  queue->desc[descidx].len = sizeof(request1);
  queue->desc[descidx].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &request2);
  queue->desc[descidx + 1].len = sizeof(request2);
  queue->desc[descidx + 1].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx + 1].next = descidx + 2;

  queue->desc[descidx + 2].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 2].len = sizeof(resp);
  queue->desc[descidx + 2].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 2].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,2);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    printf("resp=%p\n", resp.type);
    panic("virt gpu attach frame buffer"); 
  }
  free_desc(q, descidx, 3);
   
  release(&gpu.lock);
}

void
set_scanout(int q, int frame_width, int frame_height, int resource_id, int scanout_id, uint32 flags, int should_sleep){
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu set scanout");

  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_SET_SCANOUT,
      .flags = 0 | flags,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_rect rect =
  {
    .x = 0,
    .y = 0,
    .width = frame_width,
    .height = frame_height
  };

  struct virtio_gpu_set_scanout request =
  {
    .hdr = ctrl_hdr,
    .r = rect,
    .scanout_id = scanout_id,
    .resource_id = resource_id
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    printf("scanout error: %p\n", resp.type);
    panic("virt gpu set scanout"); 
  }
  free_desc(q, descidx, 2);
   
  release(&gpu.lock);
}
void
transfer_to_host(int q, int x, int y, int frame_width, int frame_height, int resource_id, uint32 flags, int should_sleep){
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu transfer to host");

  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
      .flags = 0 | flags,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_rect rect =
  {
    .x = x,
    .y = y,
    .width = frame_width,
    .height = frame_height,
  };

  struct virtio_gpu_transfer_to_host_2d request =
  {
    .hdr = ctrl_hdr,
    .r = rect,
    .offset = 0,
    .resource_id = resource_id,
    .padding = 0
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu transfer to host"); 
  }
  free_desc(q, descidx, 2);
   
  release(&gpu.lock);
}

void
flush(int q, int frame_width, int frame_height, int resource_id, uint32 flags, int should_sleep){
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu flush");

  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_RESOURCE_FLUSH,
      .flags = 0 | flags,
      .fence_id = 0,
      .ctx_id = 0,
      .padding = 0
    };
  struct virtio_gpu_rect rect =
  {
    .x = 0,
    .y = 0,
    .width = frame_width,
    .height = frame_height,
  };

  struct virtio_gpu_resource_flush request =
  {
    .hdr = ctrl_hdr,
    .r = rect,
    .resource_id = resource_id,
    .padding = 0
  };
  struct virtio_gpu_ctrl_hdr resp; 

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,1);     
  if(resp.type != VIRTIO_GPU_RESP_OK_NODATA){
    panic("virt gpu flush"); 
  }
  free_desc(q, descidx, 2);
   
  release(&gpu.lock);
}

void
update_cursor_cmd(int q, int x, int y, int resource_id, int scanout_id, uint32 flags, int should_sleep)
{
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu update cursor");

  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_UPDATE_CURSOR,
      .flags = 0 | flags,
      .fence_id = 1,
      .ctx_id = 0,
      .padding = 0
    };

  struct virtio_gpu_cursor_pos pos =
  {
    .scanout_id = scanout_id,
    .x = x,
    .y = y,
    .padding = 0
  };
  
  struct virtio_gpu_update_cursor request =
  {
    .hdr = ctrl_hdr,
    .pos = pos,
    .resource_id = resource_id,
    .hot_x = 0,
    .hot_y = 0,
    .padding = 0
  }; 
  struct virtio_gpu_ctrl_hdr resp;
  
  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,1);     
  free_desc(q, descidx, 2);
   
  release(&gpu.lock);
}

void
move_cursor_cmd(int q, int x, int y, int resource_id, int scanout_id, uint32 flags, int should_sleep)
{
  acquire(&gpu.lock);
  int descidx;
  if ((descidx = alloc_desc_wait(q,2)) < 0)
    panic("virt gpu move cursor");

  struct virtioqueue *queue = &gpu.queues[q];
  struct virtio_gpu_ctrl_hdr ctrl_hdr = 
    {
      .type = VIRTIO_GPU_CMD_MOVE_CURSOR,
      .flags = 0 | flags,
      .fence_id = 1,
      .ctx_id = 0,
      .padding = 0
    };

  struct virtio_gpu_cursor_pos pos =
  {
    .scanout_id = scanout_id,
    .x = x,
    .y = y,
    .padding = 0
  };
  
  struct virtio_gpu_update_cursor request =
  {
    .hdr = ctrl_hdr,
    .pos = pos,
    .resource_id = resource_id,
    .hot_x = 0,
    .hot_y = 0,
    .padding = 0
  }; 
  struct virtio_gpu_ctrl_hdr resp;

  queue->desc[descidx].addr = (uint64) kvmpa((uint64) &request);
  queue->desc[descidx].len = sizeof(request);
  queue->desc[descidx].flags = VRING_DESC_F_NEXT;
  queue->desc[descidx].next = descidx + 1;

  queue->desc[descidx + 1].addr = (uint64) kvmpa((uint64) &resp);
  queue->desc[descidx + 1].len = sizeof(resp);
  queue->desc[descidx + 1].flags = VRING_DESC_F_WRITE;
  queue->desc[descidx + 1].next = 0;

  send_on_queue(q, descidx, should_sleep);
  resp = *(struct virtio_gpu_ctrl_hdr *)read_response(q,1);     
  free_desc(q, descidx, 2);
   
  release(&gpu.lock);
}


void
update_cursor2(int x, int y, uint64 framebuffer, int should_sleep)
{
  int q = 0;
  int scanout_id = 0;
  q = 1;
  update_cursor_cmd(q, x, y, CURSOR_RESOURCE_ID, scanout_id, VIRTIO_GPU_FLAG_FENCE, should_sleep);
}


void
update_cursor(int x, int y, uint64 framebuffer, int should_sleep)
{
  int q = 0;
  int scanout_id = 0;
  attach_frame_buffer(q, framebuffer, CURSOR_WIDTH, CURSOR_HEIGHT, CURSOR_RESOURCE_ID, VIRTIO_GPU_FLAG_FENCE, should_sleep);
  transfer_to_host(q, 0, 0, CURSOR_WIDTH, CURSOR_HEIGHT, CURSOR_RESOURCE_ID, VIRTIO_GPU_FLAG_FENCE, should_sleep);
  q = 1;
  update_cursor_cmd(q, x, y, CURSOR_RESOURCE_ID, scanout_id, VIRTIO_GPU_FLAG_FENCE, should_sleep);
}

void
move_cursor(int x, int y, int should_sleep)
{
  move_cursor_cmd(1, x, y, CURSOR_RESOURCE_ID, 0, VIRTIO_GPU_FLAG_FENCE, should_sleep);
}

void
update_frame_buffer(uint64 framebuffer, int should_sleep)
{
  int q = 0;
  int scanout_id = 0;
  transfer_to_host(q, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, FRAME_BUFFER_RESOURCE_ID, scanout_id, should_sleep);
  flush(q, FRAME_WIDTH, FRAME_HEIGHT, FRAME_BUFFER_RESOURCE_ID, scanout_id, should_sleep);
}

void
initialize_cursor()
{
  create_resource(0, CURSOR_WIDTH, CURSOR_HEIGHT, CURSOR_RESOURCE_ID, VIRTIO_GPU_FLAG_FENCE, 0);
}

void
initialize_display(uint64 framebuffer){
  get_display_info(0, VIRTIO_GPU_FLAG_FENCE, 0);
  create_resource(0, FRAME_WIDTH, FRAME_HEIGHT, FRAME_BUFFER_RESOURCE_ID, 0, 0);
  attach_frame_buffer(0, framebuffer, FRAME_WIDTH, FRAME_HEIGHT, FRAME_BUFFER_RESOURCE_ID, VIRTIO_GPU_FLAG_FENCE, 0);
  set_scanout(0, FRAME_WIDTH, FRAME_HEIGHT, FRAME_BUFFER_RESOURCE_ID, 0, VIRTIO_GPU_FLAG_FENCE, 0);
}

void
virtio_gpu_intr(){
  acquire(&gpu.lock);
    uint32 interrupt_status = *R(1, VIRTIO_MMIO_INTERRUPT_STATUS);
    if (interrupt_status == 1){
      wakeup(&gpu.queues[0]);
      wakeup(&gpu.queues[1]);
    }

  release(&gpu.lock);
}

void create_send_rectangle(){
  update_frame_buffer((uint64)gpu.framebuffer, 1);
}

void create_send_mouse()
{
  update_cursor(20, 20, (uint64)get_cursor_frame_buf(), 1);
}
