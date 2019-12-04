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
#include "input_handler.h"

#define R(n, r) ((volatile uint32 *)(VIRTION(n) + (r)))

struct virtqueue_avail {
  uint16 flags;
  uint16 idx;
  uint16 ring[NUM_KEYBOARD];
};

struct virtqueue {
    struct VRingDesc *desc;
    struct virtqueue_avail *avail;
    struct UsedArea *used;
    char free[NUM_KEYBOARD];
    uint16 used_idx;
};

struct virtio_input_event {
	uint16 type;
	uint16 code;
	uint32 value;
};

struct virtio_input_event keyboard_event_bufs[NUM_KEYBOARD];

struct {
    char pages_event[2*PGSIZE];
    char pages_status[2*PGSIZE];
    struct virtqueue eventq;
    struct virtqueue statusq;
    struct virtio_input_event* events[NUM_KEYBOARD];
    struct spinlock vkeyboard_lock;
    int init;
} __attribute__ ((aligned (PGSIZE))) keyboard;

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
virtio_keyboard_supports_key(int n)
{
  volatile struct virtio_input_config *config = (volatile struct virtio_input_config *)(VIRTION(n) +  (VIRTIO_MMIO_CONFIG));
  config->select = VIRTIO_INPUT_CFG_EV_BITS;
  config->subsel = EV_KEY;
  if(config->size == 0)
    panic("virtio keyboard EV_KEY not supported");
}

void virtio_keyboard_fill_event_queue(int);

void
virtio_keyboard_init(int n)
{
  uint32 status = 0;

  __sync_synchronize();
  if(keyboard.init)
      return;
  
  printf("virtio keyboard init %d\n", n);

  initlock(&keyboard.vkeyboard_lock, "virtio_keyboard");

  if(*R(n, VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
     *R(n, VIRTIO_MMIO_VERSION) != 1 ||
     *R(n, VIRTIO_MMIO_DEVICE_ID) != 18 ||
     *R(n, VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
    panic("could not find virtio keyboard");
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
    panic("virtio keyboard has no queue 0");
  if(max < NUM_KEYBOARD)
    panic("virtio keyboard max queue too short");
  *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM_KEYBOARD;
  memset(keyboard.pages_event, 0, sizeof(keyboard.pages_event));
  *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)keyboard.pages_event) >> PGSHIFT;


  *R(n, VIRTIO_MMIO_QUEUE_SEL) = 1;
  if(max == 0)
    panic("virtio keyboard has no queue 1");
  if(max < NUM_KEYBOARD)
    panic("virtio keyboard max queue too short");
  *R(n, VIRTIO_MMIO_QUEUE_NUM) = NUM_KEYBOARD;
  memset(keyboard.pages_status, 0, sizeof(keyboard.pages_status));
  *R(n, VIRTIO_MMIO_QUEUE_PFN) = ((uint64)keyboard.pages_status) >> PGSHIFT;

  keyboard.eventq.desc = (struct VRingDesc *) keyboard.pages_event;
  keyboard.eventq.avail = (struct virtqueue_avail *)(((char*)keyboard.eventq.desc) + NUM_KEYBOARD*sizeof(struct VRingDesc));
  keyboard.eventq.used = (struct UsedArea *) (keyboard.pages_event + PGSIZE);
  for(int i = 0; i < NUM_KEYBOARD; i++)
    keyboard.eventq.free[i] = 1;

  keyboard.statusq.desc = (struct VRingDesc *) keyboard.pages_status;
  keyboard.statusq.avail = (struct virtqueue_avail *)(((char*)keyboard.statusq.desc) + NUM_KEYBOARD*sizeof(struct VRingDesc));
  keyboard.statusq.used = (struct UsedArea *) (keyboard.pages_status + PGSIZE);

  for(int i = 0; i < NUM_KEYBOARD; i++)
    keyboard.statusq.free[i] = 1;

  keyboard.init = 1;

  virtio_keyboard_supports_key(n);
  virtio_keyboard_fill_event_queue(n);
}

static int
alloc_event_desc()
{
  for(int i = 0; i < NUM_KEYBOARD; i++){
    if(keyboard.eventq.free[i]){
      keyboard.eventq.free[i] = 0;
      return i;
    }
  }
  return -1;
}

static void
free_event_desc(int i)
{
  if(i >= NUM_KEYBOARD)
    panic("virtio keyboard free eventq desc 1");
  if(keyboard.eventq.free[i])
    panic("virtio keyboard free eventq desc 2");
  keyboard.eventq.desc[i].addr = 0;
  keyboard.eventq.free[i] = 1;
  /*wakeup(&keyboard.eventq.free[0]);*/
}

void
virtio_keyboard_queue_event_buf(struct virtio_input_event *b)
{
  int idx = -1;
  while(1){
    if((idx = alloc_event_desc()) >= 0){
      break;
    }
    /*sleep(&keyboard.eventq.free[0], &keyboard.vkeyboard_lock);*/
  }

  keyboard.eventq.desc[idx].addr = (uint64) kvmpa((uint64) b);
  keyboard.eventq.desc[idx].len = sizeof(struct virtio_input_event);
  keyboard.eventq.desc[idx].flags = VRING_DESC_F_WRITE;
  keyboard.eventq.desc[idx].next = 0;
  keyboard.events[idx] = b;
  keyboard.eventq.avail->ring[keyboard.eventq.avail->idx % NUM_KEYBOARD] = idx;
  __sync_synchronize();
  keyboard.eventq.avail->idx = keyboard.eventq.avail->idx + 1;
}

void
virtio_keyboard_notify_eventq(int n)
{
  __sync_synchronize();
  *R(n, VIRTIO_MMIO_QUEUE_NOTIFY) = 0;
}

void
virtio_keyboard_fill_event_queue(int n)
{ 
  for(int i = 0; i < NUM_KEYBOARD; i++){
    virtio_keyboard_queue_event_buf(&keyboard_event_bufs[i]);
  }
  virtio_keyboard_notify_eventq(n);
}

void
virtio_keyboard_handle_syn_event(uint16 code, uint32 value)
{
  /*printf("EV_SYN: ");*/
  switch(code)
  {
    case SYN_REPORT:
      /*printf("report\n");*/
      break;
    case SYN_CONFIG:
      /*printf("config\n");*/
      break;
    case SYN_MT_REPORT:
      /*printf("mt report\n");*/
      break;
    case SYN_DROPPED:
      /*printf("dropped\n");*/
      break;
    default:
      printf(" code %p not recognized\n", code);
  }
}

void
virtio_keyboard_handle_key_event(uint16 code, uint32 value)
{
  add_keyboard_handler_event(ticks, code, value);
  printf("EV_KEY: ");
  printf("code=%d, value=%d\n", code, value);
}


void
virtio_keyboard_handle_event(struct virtio_input_event *b)
{
  switch(b->type)
  {
    case EV_SYN:
      virtio_keyboard_handle_syn_event(b->code, b->value);
      break;
    case EV_KEY:
      virtio_keyboard_handle_key_event(b->code, b->value);
      break;
    default:
      printf("EV TYPE %p NOT RECOGNIZED\n", b->type);
  }
}

int
virtio_keyboard_get_event_buf()
{
  if((keyboard.eventq.used_idx % NUM_KEYBOARD) == (keyboard.eventq.used->id % NUM_KEYBOARD)){
    return -1;
  }
  int id = keyboard.eventq.used->elems[keyboard.eventq.used_idx].id;
  keyboard.eventq.used_idx = (keyboard.eventq.used_idx + 1) % NUM_KEYBOARD;
  return id;
}

void 
print_keyboard_bufs()
{
  printf("EVENT BUFS:\n");
  for(int i = 0; i < NUM_KEYBOARD; i++){
    printf("  %d:, type=%p, code=%p, value=%p\n", i, keyboard_event_bufs[i].type, keyboard_event_bufs[i].code, keyboard_event_bufs[i].value);
  }
}

void
print_keyboard_eventq()
{
  printf("EVENTQ:\n");
  printf("  DESC:\n");
  for(int id = 0; id < NUM_KEYBOARD; id++){
    printf("    %d: addr=%p, len=%d, flags=%p, next=%d\n", id, keyboard.eventq.desc[id].addr, keyboard.eventq.desc[id].len, keyboard.eventq.desc[id].flags, keyboard.eventq.desc[id].next);
  }
  printf("  AVAIL: flags=%p, idx=%d, ring:\n", keyboard.eventq.avail->flags, keyboard.eventq.avail->idx);
  for(int id = 0; id < NUM_KEYBOARD; id++){
    printf("    %d: %d\n", id, keyboard.eventq.avail->ring[id]);
  }
  printf("  USED: flags=%p, id=%d elems:\n", keyboard.eventq.used->flags, keyboard.eventq.used->id);
  for(int id = 0; id < NUM_KEYBOARD; id++){
    printf("    %d: id=%d, len=%d\n", id, keyboard.eventq.used->elems[id].id, keyboard.eventq.used->elems[id].len);
  }
  printf("  FREE:\n");
  for(int id = 0; id < NUM_KEYBOARD; id++){
    printf("    %d: %d\n", id, keyboard.eventq.free[id]);
  }
  printf("  USED_IDX=%d\n", keyboard.eventq.used_idx);
}

void
print_keyboard_state()
{
  printf("EVENTS: ");
  for(int i = 0; i < NUM_KEYBOARD; i++){
    printf("%p, ", keyboard.events[i]);
  }
  printf("\n");
  print_keyboard_bufs();
  print_keyboard_eventq();
}

void
virtio_keyboard_recv_events(int n){
  int id;
  struct virtio_input_event *b;
  acquire(&keyboard.vkeyboard_lock);
  while((id = virtio_keyboard_get_event_buf()) >= 0){
    b = keyboard.events[id];
    /*release(&keyboard.vkeyboard_lock);*/
    virtio_keyboard_handle_event(b);
    /*acquire(&keyboard.vkeyboard_lock);*/
    free_event_desc(id);
    virtio_keyboard_queue_event_buf(b);
  }
  virtio_keyboard_notify_eventq(n);
  release(&keyboard.vkeyboard_lock);
}

