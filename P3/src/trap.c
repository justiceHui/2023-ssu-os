#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "date.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  static int sche_tick = 0; // timer for recalculate priority

  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }

    // CPU use!
    if(myproc() != 0 && myproc()->pid >= 3 && myproc()->state == RUNNING){
      myproc()->proc_tick += 1;
      myproc()->priority_tick += 1;
      myproc()->cpu_used += 1;
      if(myproc()->parent->pid != 2) sche_tick += 1; // for convenience of output, we don't count ticks from scheduler_test
    }

    // if alarm is activated
    if(myproc() != 0 && myproc()->alarm_timer == 1){
      struct rtcdate d;
      cmostime(&d);

      // 현재 시간 계산(초 단위)
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

      // 종료 시간 이후면 알람 종료 후 출력
      if(myproc()->alarmticks <= cur_time){
        myproc()->alarmticks = 0;
        myproc()->alarm_timer = 2; // deactivate
        cprintf("SSU_Alarm!\n");
        cprintf("Current time : %d-%d-%d %d:%d:%d\n", d.year, d.month, d.day, d.hour, d.minute, d.second);
      }
    }

    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // if timer is ended
  if(myproc() && myproc()->alarm_timer == 2){
    exit();
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  // if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER) yield();
  
  // terminate process
  if(myproc() != 0 && myproc()->pid >= 3 && myproc()->cpu_used >= myproc()->end_tick){
    cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_usage : %d ticks (3)\n", myproc()->pid, myproc()->priority, myproc()->proc_tick, myproc()->cpu_used);
    cprintf("PID :%d terminated\n", myproc()->pid);
    exit();
  }

  // recalculate priority
  if(myproc() != 0 && myproc()->state == RUNNING && sche_tick >= 60){
    cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_usage : %d ticks (2)\n", myproc()->pid, myproc()->priority, myproc()->proc_tick, myproc()->cpu_used);
    sche_tick = 0;
    recalculate();
  }
  
  // yield and select process to run
  if(myproc() != 0 && myproc()->state == RUNNING && (myproc()->pid <= 2 || myproc()->proc_tick >= 30)){
    if(myproc()->pid >= 3 && myproc()->parent->pid != 2)
      cprintf("PID : %d, priority : %d, proc_tick : %d ticks, total_cpu_usage : %d ticks (1)\n", myproc()->pid, myproc()->priority, myproc()->proc_tick, myproc()->cpu_used);
    yield();
  }

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
