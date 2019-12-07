#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"
#include "kernel/colors.h"
#include "user/gui.h"
#include "user/uthread.h"
#include "kernel/window_event.h"
#include "kernel/fs.h"
#include "kernel/param.h"

#define WIDTH 200

#define MAXAPPS 10

struct application {
  char appname[DIRSIZ];
  char path[MAXPATH];
  char execname[DIRSIZ];
};

struct application apps[MAXAPPS];
int numapps;

void
load_apps()
{
  char* appspath = "./applications";
  int fdapps, fdapp;
  struct dirent dedir;
  struct dirent defile;
  struct stat st;
  if((fdapps = open(appspath, 0)) < 0){
    fprintf(2, "applauncher: cannot open applications dir\n");
    return;
  }

  int appspathlen = strlen(appspath);
  char appdirpath[MAXPATH];
  int appidx = 0;
  while(read(fdapps, &dedir, sizeof(dedir)) == sizeof(dedir)){
    if(dedir.inum == 0)
      continue;
    strcpy(appdirpath, appspath);
    appdirpath[appspathlen] = '/';
    strcpy(appdirpath + appspathlen + 1, dedir.name);
    strcpy(apps[appidx].appname, dedir.name);
    if((fdapp = open(appdirpath, 0)) < 0){
      fprintf(2, "applauncher: cannot open application sub directory %s\n", appdirpath);
      close(fdapps);
    }
    if(fstat(fdapp, &st) < 0){
      fprintf(2, "applauncher: cannot stat application sub directory %s\n", appdirpath);
      close(fdapp);
      close(fdapps);
      return;
    }
    if(st.type != T_DIR)
      continue;
    int baselen = strlen(appdirpath);
    appdirpath[baselen] = '/';
    appdirpath[baselen+1] = 0;
    while(read(fdapp, &defile, sizeof(defile)) == sizeof(defile)){
      if(defile.inum == 0)
        continue;
      strcpy(apps[appidx].execname, defile.name);
      strcpy(apps[appidx].path, appdirpath);
      strcpy(apps[appidx].path + baselen + 1, defile.name);
    }
    close(fdapp);
    appidx++;
    if(appidx > MAXAPPS)
      break;
  }
  close(fdapps);

  numapps = appidx;
  /*for(int i = 0; i < appidx; i++){*/
    /*printf("APP: name=%s   exec=%s   path=%s\n", apps[i].appname, apps[i].execname, apps[i].path);*/
  /*}*/
}

void
click_handler(int id)
{
  char *argv[] = {apps[id].execname, 0};
  int pid = fork();
  if(pid < 0){
    printf("applauncher: fork failed\n");
  }
  if(pid == 0){
    exec(apps[id].path, argv);
  }
}

void
make_buttons(struct state *state)
{
  int ypos = 0;
  struct elmt* button;
  for(int appidx = 0; appidx < numapps; appidx++){
    button = new_elmt(BUTTON);
    button->id = appidx;
    button->mlc = &click_handler;
    button->width = WIDTH;
    button->height = 50;
    button->x = 0;
    button->y = ypos;
    button->main_fill = C_LAUNCH_GRAY;
    button->main_alpha = 255;
    button->border = 2;  

    button->border_fill = C_LAUNCH_GRAY;  
    button->border_alpha = 255;
    button->text = apps[appidx].appname;
    button->textlength = strlen(button->text);
    button->fontsize = 4;
    button->textalignment = 0;
    button->text_fill = C_WHITE;
    button->text_alpha = 255;

    add_elmt(state, button);

    ypos += 50;
  }
}

int
main(void)
{
  load_apps();
  int winheight = 50 * numapps;
  struct gui* gui = init_gui();
  struct window* win = new_applauncher(gui, WIDTH, winheight);  
  struct state* state = new_state();
  add_state(win, state);
  add_window(gui, win);
  make_buttons(state);
  printf("looping\n");
  loop(gui, win);
  exit(0);

  /*int fd;*/
  /*uint32 *frame_buf = (uint32*)mkapplauncher(&fd);*/
  /*int width = 300;*/
  /*int height = 500;*/
  /*for(int y = 0; y < height; y++){*/
    /*for(int x = 0; x < width; x++){*/
      /**(frame_buf + y * width + x) = 0xFFFF00FF;*/
    /*}*/
  /*}*/
  /*updatewindow(fd, width, height);*/


  /*struct window_event *we = 0;*/
  /*int bytes;*/
  /*while(1){*/
    /*if((bytes = read(fd, (void*)we, sizeof(struct window_event))) > 0){*/
      /*if(we->kind == W_KIND_MOUSE){*/
        /*switch(we->m_event.type){*/
          /*case W_CUR_MOVE_ABS:*/
            /*printf("APPLAUNCHER      W_CUR_MOVE_ABS: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);*/
            /*break;*/
          /*case W_LEFT_CLICK_PRESS:*/
            /*printf("APPLAUNCHER      W_LEFT_CLICK_PRESSS: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);*/
            /*break;*/
          /*case W_LEFT_CLICK_RELEASE:*/
            /*printf("APPLAUNCHER      W_LEFT_CLICK_RELEASE: timestamp=%d, x=%d, y=%d\n",we->timestamp, we->m_event.xval, we->m_event.yval);*/
            /*break;*/
          /*default:*/
            /*printf("APPLAUNCHER      EVENT NOT RECOGNIZED: timestamp=%d, type=%d, xval=%d, yval=%d\n",we->timestamp, we->m_event.type, we->m_event.xval, we->m_event.yval);*/
        /*}*/
      /*} else {*/
        /*printf("APPLAUNCHER      KEYBOARD: timestamp=%d, character=%c\n",we->timestamp, we->k_event.ascii_val);*/
      /*}*/
    /*}*/
  /*}*/
  /*exit(0);*/
}

