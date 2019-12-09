struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct w_pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void            binit(void);
struct buf*     bread(uint, uint);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c
void            consoleinit(void);
void            consoleintr(int);
void            consputc(int);

// exec.c
int             exec(char*, char**);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64 addr);
int             filewrite(struct file*, uint64, int n);

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, int, uint64, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64, uint, uint);

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskintr(void);
void            ramdiskrw(struct buf*);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit();
void*           kdisplaymem(void);

// log.c
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op(int);
void            end_op(int);
void            crash_op(int,int);

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, uint64, int);
int             pipewrite(struct pipe*, uint64, int);

//w_pipe.c
int             w_pipealloc(struct file**, struct file**);
void            w_pipeclose(struct w_pipe*, int);
int             w_pipewrite(struct w_pipe*, uint64);
int             w_piperead(struct w_pipe*, uint64);

// printf.c
void            printf(char*, ...);
void            panic(char*) __attribute__((noreturn));
void            printfinit(void);

// proc.c
int             cpuid(void);
void            exit(int);
int             fork(void);
int             growproc(int);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, uint64);
int             kill(int);
struct cpu*     mycpu(void);
struct cpu*     getmycpu(void);
struct proc*    myproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
void            userinit(void);
int             wait(uint64);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);
void            procdump(void);

// swtch.S
void            swtch(struct context*, struct context*);

// spinlock.c
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);
void            push_off(void);
void            pop_off(void);
uint64          sys_ntas(void);

// sleeplock.c
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);

// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// syscall.c
int             argint(int, int*);
int             argstr(int, char*, int);
int             argaddr(int, uint64 *);
int             fetchstr(uint64, char*, int);
int             fetchaddr(uint64, uint64*);
void            syscall();

// trap.c
extern uint     ticks;
void            trapinit(void);
void            trapinithart(void);
extern struct spinlock tickslock;
void            usertrapret(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
uint64          kvmpa(uint64);
void            kvmmap(uint64, uint64, uint64, int);
int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uchar *, uint);
uint64          uvmalloc(pagetable_t, uint64, uint64);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
void            uvmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
uint64          walkaddr(pagetable_t, uint64);
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);

// plic.c
void            plicinit(void);
void            plicinithart(void);
int             plic_claim(void);
void            plic_complete(int);

// virtio_disk.c
void            virtio_disk_init(int);
void            virtio_disk_rw(int, struct buf *, int);
void            virtio_disk_intr(int);

// virtio_mouse.c
void            virtio_mouse_init(int);
void            virtio_mouse_recv_events(int);

// virtio_keyboard.c
void            virtio_keyboard_init(int);
void            virtio_keyboard_recv_events(int);

// input_handler.c
void            init_input_handler(void);
void            start_input_handler(void);
void            add_mouse_handler_event(uint, int, int, int); 
void            add_keyboard_handler_event(uint, int, int); 

// cursor.c
void            init_cursor(void);
void            get_cursor_pos(int*, int*);
void            update_cursor_rel(int, int);
void            update_cursor_abs(int, int);
void            send_cursor_update(void);
void            left_click_press(void);
void            left_click_release(void);

// display.c
void            init_frame(void);
void            send_frame_update(void);
void            set_pixel_hex(int, int, uint32);
void            set_pixel(int, int, uint32, uint32);
void            draw_rect(int, int, int, int, uint32, uint8);
void            draw_line(int, int, int, int, uint32, uint8);
void            draw_circle(int, int, int, uint32, uint8);
void            draw_wallpaper(void);
void            draw_diagram(uint32*);


// windows.c
void            init_windows(void);
struct window*  new_window(struct file*, struct file*);
struct window*  new_diagram(struct file*, struct file*);
int             update_window(struct file*, int, int);
void            user_close_window(struct file*);
struct window*  make_applauncher(struct file*, struct file*);
void            handle_left_click_press(uint, int, int);
void            handle_left_click_release(uint, int, int);
void            handle_right_click_press(uint, int, int);
void            handle_right_click_release(uint, int, int);
void            handle_cursor_move(uint, int, int);
void            handle_keyboard(uint, int, int);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

// Extra files for allocator lab


// buddy.c
void           bd_init(void*,void*);
void           bd_free(void*);
void           *bd_malloc(uint64);

struct list {
  struct list *next;
  struct list *prev;
};

// list.c
void lst_init(struct list*);
void lst_remove(struct list*);
void lst_push(struct list*, void *);
void *lst_pop(struct list*);
void lst_print(struct list*);
int lst_empty(struct list*);

//virtio_gpu.c
void create_send_rectangle();
void create_send_mouse();
void virtio_gpu_intr();
void virtio_gpu_init(int);
void update_frame_buffer(uint64, int);
void update_cursor(int, int, uint64, int);
void move_cursor(int, int, int);
void initialize_display(uint64);
void initialize_cursor();
void update_cursor2(int, int, uint64, int);
