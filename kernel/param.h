#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       0  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define NDISK        2
#define NWINDOW      5

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 1024
#define CURSOR_WIDTH 64
#define CURSOR_HEIGHT 64
#define CURSOR_START_X (FRAME_WIDTH / 2)
#define CURSOR_START_Y (FRAME_HEIGHT / 2)
#define W_BORDER_SIZE 4
#define W_HEADER_SIZE 30
#define MAX_WINDOWS 10

