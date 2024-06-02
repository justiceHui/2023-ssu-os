#include "types.h"
#include "stat.h"
#include "user.h"

#define PNUM 3

int priority[PNUM] = {1, 22, 34};
int end_tick[PNUM] = {300, 600, 600};

void scheduler_func(void){
  int pid[PNUM];

  for(int i=0; i<PNUM; i++){
    pid[i] = fork();
    if(pid[i] == 0){ 
      set_sche_info(priority[i], end_tick[i]);
      while(1);
      exit();
    }
    else{
    }
  }
  for(int i=0; i<PNUM; i++) wait();
}

int main(void){
  printf(1, "start scheduler_test\n");
  scheduler_func();
  printf(1, "end of scheduler_test\n");
  exit();
}
