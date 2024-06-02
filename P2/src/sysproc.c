#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_date(void){
  struct rtcdate *d;
  // 첫 번째 인자를 가져온 다음, cmostime을 이용해 현재 시간을 구함
  argptr(0, (void*)&d, sizeof d);
  cmostime(d);
  return 0;
}

int
sys_alarm(void){
  // 현재 시간 계산(초 단위)
  struct rtcdate d;
  cmostime(&d);
  const static int month_date[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int cur_time = 0;
  for(int i=2020; i<d.year; i++){
    cur_time += i % 400 == 0 || (i % 4 == 0 && i % 100 != 0) ? 366 : 365;
  }
  for(int i=1; i<d.month; i++){
    if(i != 2) cur_time += month_date[i];
    else if(d.year % 400 == 0 || (d.year % 4 == 0 && d.year % 100 != 0)) cur_time += 29;
    else cur_time += 28;
  }
  cur_time += d.day;
  cur_time = cur_time * 86400 + d.hour * 3600 + d.minute * 60 + d.second;

  // 현재 시간 + seconds가 되면 종료
  int count;
  argint(0, &count);
  myproc()->alarmticks = cur_time + count;
  myproc()->alarm_timer = 1; // Activate: 1

  return 0;
}
