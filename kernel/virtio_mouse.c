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
#include "events.h"

#define NUM_MOUSE_MOUSE_EVENTS 10
#define R(n, r) ((volatile uint32 *)(VIRTION(n) + (r)))

#define NUM_MOUSE_KEY_SUPPORTED 2
#define NUM_MOUSE_REL_SUPPORTED 2
uint mouse_key_codes[] = {BTN_LEFT, BTN_RIGHT};
uint mouse_rel_codes[] = {REL_X, REL_Y};

struct virtqueue_avail {
  uint16 flags;
  uint16 idx;
  uint16 ring[NUM_MOUSE];
};

struct virtqueue {
    struct VRingDesc *desc;
    struct virtqueue_avail *avail;
    struct UsedArea *used;
    char free[NUM_MOUSE];
    uint16 used_idx;
};

struct virtio_input_event {
	uint16 type;
	uint16 code;
	uint32 value;
};

struct virtio_input_event event_bufs[NUM_MOUSE];

struct {
    char pages_event[2*PGSIZE];
    char pages_status[2*PGSIZE];
    struct virtqueue eventq;
    struct virtqueue statusq;
    struct virtio_input_event* events[NUM_MOUSE];
    struct spinlock vmouse_lock;
    int init;
} __attribute__ ((aligned (PGSIZE))) mouse;

enum virtio_input_config_select {
	VIRTIO_INPUT_CFG_UNSET      = 0x00,
	VIRTIO_INPUT_CFG_ID_NAME    = 0x01,
	VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02,
	VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03,
	VIRTIO_INPUT_CFG_PROP_BITS  = 0x10,
	VIRTIO_INPUT_CFG_EV_BITS    = 0x11,
	VIRTIO_INPUT_CFG_ABS_INFO   = 0x12,
};

struct virtio_input_absinfo {
	uint32 min;
	uint32 max;
	uint32 fuzz;
	uint32 flat;
	uint32 res;
};

struct virtio_input_devids {
	uint16 bustype;
	uint16 vendor;
	uint16 product;
	uint16 version;
};

struct virtio_input_config {
	uint8    select;
	uint8    subsel;
	uint8    size;
	uint8    reserved[5];
	union {
		char string[128];
		uint8 bitmap[128];
		struct virtio_input_absinfo abs;
		struct virtio_input_devids ids;
	} u;
};

void
virtio_mouse_supports_key(int n)
{
  volatile struct virtio_input_config *config = (volatile struct virtio_input_config *)(VIRTION(n) +  (VIRTIO_MMIO_CONFIG));
  config->select = VIRTIO_INPUT_CFG_EV_BITS;
  config->subsel = EV_KEY;
  if(config->size == 0)
    panic("virtio mouse EV_KEY not supported");
  uint key_code;
  for(int i = 0; i < NUM_MOUSE_KEY_SUPPORTED; i++){
    key_code = mouse_key_codes[i];
    if(!(config->u.bitmap[key_code / 8] & (1 << key_code % 8)))
      panic("virtio mouse key code not supported");
  }
}

void
virtio_mouse_supports_rel(int n)
{
  volatile struct virtio_input_config *config = (volatile struct virtio_input_config *)(VIRTION(n) +  (VIRTIO_MMIO_CONFIG));
  config->select = VIRTIO_INPUT_CFG_EV_BITS;
  config->subsel = EV_REL;
  if(config->size == 0)
    panic("virtio mouse EV_REL not supported");
  uint rel_code;
  for(int i = 0; i < NUM_MOUSE_REL_SUPPORTED; i++){
    rel_code = mouse_rel_codes[i];
    if(!(config->u.bitmap[rel_code / 8] & (1 << rel_code % 8)))
      panic("virtio mouse rel code not supported");
  }
}

void virtio_mouse_fill_event_queue(int);

void
virtio_mouse_init(int n)
{
  uint32 status = 0;

  __sync_synchronize();
  if(mouse.init)
      return;
  
  printf("virtio mouse init %d\n", n);

  initlock(&mouse.vmouse_lock, "virtio_mouse");

  if(*R(n, VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
     *R(n, VIRTIO_MMIO_VERSION) != 1 ||
     *R(n, VIRTIO_MMIO_DEVICE_ID) != 18 ||
     *R(n, VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
    panic("could not find virtio mouse");
  }

  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *R(n, VIRTIO_MMIO_STATUS) = status;

  *R(n, VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;


  *R(n, VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = *R(n, VIRTIO_MMIO_QUEUE_NUM_MAX);
  if(max == 0)
    panic("virtio mouse has no queue 0");
  if(max < NUM_MOUSE)
    panic("virtio mouse max queue too short");
  *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM_MOUSE;
  memset(mouse.pages_event, 0, sizeof(mouse.pages_event));
  *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)mouse.pages_event) >> PGSHIFT;


  *R(n, VIRTIO_MMIO_QUEUE_SEL) = 1;
  if(max == 0)
    panic("virtio mouse has no queue 1");
  if(max < NUM_MOUSE)
    panic("virtio mouse max queue too short");
  *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM_MOUSE;
  memset(mouse.pages_status, 0, sizeof(mouse.pages_status));
  *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)mouse.pages_status) >> PGSHIFT;

  mouse.eventq.desc = (struct VRingDesc *) mouse.pages_event;
  mouse.eventq.avail = (struct virtqueue_avail *)(((char*)mouse.eventq.desc) + NUM_MOUSE*sizeof(struct VRingDesc));
  mouse.eventq.used = (struct UsedArea *) (mouse.pages_event + PGSIZE);
  for(int i = 0; i < NUM_MOUSE; i++)
    mouse.eventq.free[i] = 1;

  mouse.statusq.desc = (struct VRingDesc *) mouse.pages_status;
  mouse.statusq.avail = (struct virtqueue_avail *)(((char*)mouse.statusq.desc) + NUM_MOUSE*sizeof(struct VRingDesc));
  mouse.statusq.used = (struct UsedArea *) (mouse.pages_status + PGSIZE);

  for(int i = 0; i < NUM_MOUSE; i++)
    mouse.statusq.free[i] = 1;

  mouse.init = 1;

  virtio_mouse_supports_key(n);
  virtio_mouse_supports_rel(n);
  virtio_mouse_fill_event_queue(n);
}

static int
alloc_event_desc()
{
  for(int i = 0; i < NUM_MOUSE; i++){
    if(mouse.eventq.free[i]){
      mouse.eventq.free[i] = 0;
      return i;
    }
  }
  return -1;
}

static void
free_event_desc(int i)
{
  if(i >= NUM_MOUSE)
    panic("virtio mouse free eventq desc 1");
  if(mouse.eventq.free[i])
    panic("virtio mouse free eventq desc 2");
  mouse.eventq.desc[i].addr = 0;
  mouse.eventq.free[i] = 1;
  /*wakeup(&mouse.eventq.free[0]);*/
}

void
virtio_mouse_queue_event_buf(struct virtio_input_event *b)
{
  acquire(&mouse.vmouse_lock);
  int idx = -1;
  while(1){
    if((idx = alloc_event_desc()) >= 0){
      break;
    }
    /*sleep(&mouse.eventq.free[0], &mouse.vmouse_lock);*/
  }

  mouse.eventq.desc[idx].addr = (uint64) kvmpa((uint64) b);
  mouse.eventq.desc[idx].len = sizeof(struct virtio_input_event);
  mouse.eventq.desc[idx].flags = VRING_DESC_F_WRITE;
  mouse.eventq.desc[idx].next = 0;
  mouse.events[idx] = b;
  mouse.eventq.avail->ring[mouse.eventq.avail->idx % NUM_MOUSE] = idx;
  __sync_synchronize();
  mouse.eventq.avail->idx = mouse.eventq.avail->idx + 1;
  release(&mouse.vmouse_lock);
}

void
virtio_mouse_notify_eventq(int n)
{
  __sync_synchronize();
  *R(n, VIRTIO_MMIO_QUEUE_NOTIFY) = 0;
}

void
virtio_mouse_fill_event_queue(int n)
{ 
  for(int i = 0; i < NUM_MOUSE; i++){
    virtio_mouse_queue_event_buf(&event_bufs[i]);
  }
  virtio_mouse_notify_eventq(n);
}

void
virtio_mouse_handle_syn_event(uint16 code, uint32 value)
{
  printf("EV_SYN: ");
  switch(code)
  {
    case SYN_REPORT:
      printf("report\n");
      send_cursor_update();
      break;
    case SYN_CONFIG:
      printf("config\n");
      break;
    case SYN_MT_REPORT:
      printf("mt report\n");
      break;
    case SYN_DROPPED:
      printf("dropped\n");
      break;
    default:
      printf(" code %p not recognized\n", code);
  }
}

void
virtio_mouse_handle_key_event(uint16 code, uint32 value)
{
  printf("EV_KEY: ");
  switch(code)
  {
    case BTN_LEFT:
      printf("left click");
      if(value == 1){
        printf("[pressed]\n");
        cursor_left_click_press();
      } else if(value == 0) {
        printf("[released]\n");
      } else {
        printf("[value not recognized]\n");
      }
      break;
    case BTN_RIGHT:
      printf("left click");
      if(value == 1){
        printf("[pressed]\n");
      } else if(value == 0) {
        printf("[released]\n");
      } else {
        printf("[value not recognized]\n");
      }
      break;
    case BTN_GEAR_DOWN:
      printf("gear down %d\n", value);
      break;
    case BTN_GEAR_UP:
      printf("gear up %d\n", value);
      break;
    default:
      printf("code %p not recognized\n", code);
  }
}

void
virtio_mouse_handle_rel_event(uint16 code, uint32 value)
{
  printf("EV_REL: ");
  switch(code)
  {
    case REL_X:
      update_cursor_rel((int)value, 0);
      printf("x ");
      break;
    case REL_Y:
      update_cursor_rel(0, (int)value);
      printf("y ");
      break;
    default:
      printf("code %p not recognized\n", code);
      return;
  }
  int delta = (int) value;
  if(delta > 0){
    printf("+%d\n", delta);
  } else if(delta == 0){
    printf("0\n");
  } else {
    printf("%d\n", delta);
  }
}

void
virtio_mouse_handle_event(struct virtio_input_event *b)
{
  switch(b->type)
  {
    case EV_SYN:
      virtio_mouse_handle_syn_event(b->code, b->value);
      break;
    case EV_KEY:
      virtio_mouse_handle_key_event(b->code, b->value);
      break;
    case EV_REL:
      virtio_mouse_handle_rel_event(b->code, b->value);
      break;
    default:
      printf("EV TYPE %p NOT RECOGNIZED\n", b->type);
  }
}

int
virtio_mouse_get_event_buf()
{
  if((mouse.eventq.used_idx % NUM_MOUSE) == (mouse.eventq.used->id % NUM_MOUSE)){
    return -1;
  }
  int id = mouse.eventq.used->elems[mouse.eventq.used_idx].id;
  mouse.eventq.used_idx = (mouse.eventq.used_idx + 1) % NUM_MOUSE;
  return id;
}

void 
print_bufs()
{
  printf("EVENT BUFS:\n");
  for(int i = 0; i < NUM_MOUSE; i++){
    printf("  %d:, type=%p, code=%p, value=%p\n", i, event_bufs[i].type, event_bufs[i].code, event_bufs[i].value);
  }
}

void
print_eventq()
{
  printf("EVENTQ:\n");
  printf("  DESC:\n");
  for(int id = 0; id < NUM_MOUSE; id++){
    printf("    %d: addr=%p, len=%d, flags=%p, next=%d\n", id, mouse.eventq.desc[id].addr, mouse.eventq.desc[id].len, mouse.eventq.desc[id].flags, mouse.eventq.desc[id].next);
  }
  printf("  AVAIL: flags=%p, idx=%d, ring:\n", mouse.eventq.avail->flags, mouse.eventq.avail->idx);
  for(int id = 0; id < NUM_MOUSE; id++){
    printf("    %d: %d\n", id, mouse.eventq.avail->ring[id]);
  }
  printf("  USED: flags=%p, id=%d elems:\n", mouse.eventq.used->flags, mouse.eventq.used->id);
  for(int id = 0; id < NUM_MOUSE; id++){
    printf("    %d: id=%d, len=%d\n", id, mouse.eventq.used->elems[id].id, mouse.eventq.used->elems[id].len);
  }
  printf("  FREE:\n");
  for(int id = 0; id < NUM_MOUSE; id++){
    printf("    %d: %d\n", id, mouse.eventq.free[id]);
  }
  printf("  USED_IDX=%d\n", mouse.eventq.used_idx);
}

void
print_state()
{
  printf("EVENTS: ");
  for(int i = 0; i < NUM_MOUSE; i++){
    printf("%p, ", mouse.events[i]);
  }
  printf("\n");
  print_bufs();
  print_eventq();
}

void
virtio_mouse_recv_events(int n){
  int id;
  struct virtio_input_event *b;
  while (1)
  {
    while((id = virtio_mouse_get_event_buf()) >= 0){
      b = mouse.events[id];
      virtio_mouse_handle_event(b);
      free_event_desc(id);
      virtio_mouse_queue_event_buf(b);
    }
    virtio_mouse_notify_eventq(n);
  }
}












