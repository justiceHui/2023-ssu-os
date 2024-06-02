# 1. 개요

본 과제는 (1) xv6에서 실행할 프로세스를 선택하고 실행하는 과정 및 xv6에서 기본적으로 제공하는 스케줄링 알고리즘을 분석하고, (2) xv6의 scheduler() 함수를 우선순위 스케줄링 방식의 SSU Scheduler로 수정하고 (3) 프로세스의 우선순위를 조작하는 시스템콜 set_sche_info()를 만들어서 (4) 스케줄링 과정을 확인한 뒤, (5) xv6에서 기본적으로 제공하는 라운드 로빈 방식의 스케줄링과 우선순위를 사용하는 SSU_Scheduler의 성능을 비교하는 실습 과제다.

# 2. 기존 xv6 프로세스 분석 (과제 3-1)

### 2-1. 프로세스의 실행 과정

각각의 가상 CPU는 실행할 프로세스를 결정할 scheduler와 현재 실행하고 있는 프로세스를 나타내는 proc을 멤버로 갖고 있다. scheduler() 함수에서 어떤 가상 CPU c에게 프로세스 p를 실행하도록 하는 것은 다음과 같은 과정으로 진행된다.    

1. CPU의 proc 값을 실행하고자 하는 프로세스로 바꿈 (c->proc = p)
2. switchuvm(p)를 이용해 유저 모드에서 p를 실행할 때 사용할 페이지 테이블로 전환
3. swtch(&(c->scheduler), p->context)를 이용해 현재 CPU의 컨텍스트를 스케줄러에서 p로 전환해서 실행

이렇게 해서 실행된 프로세스 p가 yield(), sleep(), exit() 등을 호출해서 제어권을 운영체제에게 넘겨주면, 아래 과정을 통해 다시 scheduler() 함수로 돌아와서 다음에 실행할 프로세스를 결정한다.

1. yield(), sleep(), exit()에서 sched() 함수 호출
2. sched() 함수에서 swtch(&p->convext, mycpu()->scheduler)를 이용해 현재 CPU의 컨텍스트를 p에서 스케줄러로 전환
3. switchkvm() 함수를 이용해 커널 모드에서 사용할 페이지 테이블로 전환
4. CPU가 실행하는 프로세스를 비움 (c->proc = 0)
5. scheduler() 함수에서 다음에 실행할 프로세스를 선택해서 프로세스를 실행하는 과정 반복

이 과정을 그림으로 나타내면 다음과 같다.

![](./img/xv6-process.png)

### 2-2. scheduler() 함수의 작동 과정

scheduler 함수의 전문은 다음과 같다.

```c
void scheduler(void){
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE) continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}
```

scheduler() 함수는 각각의 가상 CPU마다 백그라운드에서 무한루프로 돌아가는 함수로, 프로세스가 yield(), sleep(), exit()을 호출해서 제어권을 포기하면 sched() 함수를 통해 호출된다.

함수 최상단에 선언되어 있는 변수 p와 c는 실행하고 있는 프로세스와 가상 CPU를 의미한다. p는 무한루프 안에서 매번 스케줄링 알고리즘에 의해 값이 결정된다.

무한루프 안에서는 프로세스 테이블의 가장 앞에 있는 프로세스부터 차례대로 보면서, 실행 가능한(state가 RUNNABLE)인 프로세스 p를 발견하면, CPU가 실행하는 프로세스를 p로 바꾼 뒤(c->proc = p), switchuvm() 함수를 이용해 유저 모드의 페이지 테이블로 교체하고, p의 상태를 RUNNING으로 바꾼 뒤, p의 context로 context switching을 한다.

이때, 반복문에서 탈출하지 않은 채로 context switching 한다는 것에 주목해야 한다. 이를 통해 매번 프로세스 테이블의 맨 앞에 있는 프로세스부터 보는 것이 아닌, 프로세스 테이블을 한 번 다 훑을 때까지 다시 앞으로 이동하지 않게 된다. 따라서 xv6의 기본 스케줄러는 모든 프로세스에게 공평한 기회를 주는 라운드 로빈 방식으로 작동한다는 것을 알 수 있다.

다시 프로세스가 yield(), sleep(), exit()을 통해 제어권을 반납하면 switchkvm()을 통해 커널 모드의 페이지 테이블을 가져오고, CPU가 실행하는 프로세스를 비운다(c->proc = 0).

# 3. 상세 설계

### 3-1. 연결 리스트와 큐 설계

kernel node에서 malloc과 free를 사용하지 않기 위해서 미리 연결 리스트의 node pool을 정적으로 할당받은 뒤, page table에서 사용하는 방식과 비슷하게 노드 할당 요청이 들어오면 pool에서 아직 사용하지 않은 공간을 넘겨주는 방식으로 구현했다. 큐는 head와 tail을 저장함으로써, 순차 탐색과 원소 추가 모두 쉽도록 만들었다. 0 ~ 24번 큐는 우선순위 스케줄링에 사용하는 큐이며, 25번 큐는 60ticks마다 모든 프로세스의 우선순위를 계산할 때 프로세스 목록을 임시로 저장할 때 사용한다.

```c
// linked list node
struct my_node{ struct proc *cur; int nxt, used; } node_pool[NPROC*2];

// queue implemented with linked list
struct my_queue{ int head, tail; } que[26];

// init queue
void queue_init(){ }

// same as que[idx].push(p)
void _queue_push(int idx, struct proc *p){ }

// same as que[p->priority/4].push(p)
void queue_push(struct proc *p){ _queue_push(p->priority / 4, p); }

// same as que[idx].clear()
void queue_clear(int idx){ }
```

### 3-2. proc 구조체 수정

struct proc 구조체에 (1) 프로세스의 우선순위를 저장하는 priority, (2) 현재 time quantum에서 CPU를 점유한 시간을 나타내는 proc_tick, (3) 프로세스가 실행되는 동안 CPU를 점유한 총 시간을 나타내는 cpu_used, (4) 마지막으로 우선순위를 계산한 이후로 CPU를 점유한 시간을 나타내는 priority_tick, (5) 프로세스의 종료 시점을 나타내는 end_tick까지 총 5개의 멤버 변수를 추가했다.

```c
// Per-process state
struct proc {
  // ...
  int priority;                // Assignment 3-2: process priority
  int proc_tick;               // Assignment 3-2: process run time in current TQ
  int cpu_used;                // Assignment 3-2: process total run time
  int priority_tick;           // Assignment 3-2: process run time brfore rescheduling
  int end_tick;                // Assignment 3-2: process deadline
};
```

### 3-3. 우선순위의 계산

프로세스의 초기 우선순위를 계산하는 get_initial_priority() 함수와 60ticks 마다 모든 프로세스의 우선순위를 계산하기 위한 recalculate() 함수를 만들었다.

get_initial_priority() 함수는 큐에 존재하는 모든 프로세스를 25번 큐에 모은 다음, 이전 우선순위 재계산 이후로 얼마나 CPU를 점유했는지 나타내는 priority_tick의 값을 이용해 새로운 우선순위를 계산해서 다시 올바른 큐에 삽입했다. get_initial_priority() 함수는 큐를 차례대로 보면서, 비어있지 않은 큐가 등장하면 해당 큐에서 우선순위가 가장 작은 프로세스를 구하는 방식으로 구현했다.

```c
// recalculate priority
void recalculate(){
  // 1. push every processes to que[25]
  // 2. recalculate priority and push to queue
}

// get initial priority
int get_initial_priority(){
  int res = 100;
  // 1. iterate every processes
  // 2. get minimum priority among the processes
  return res
}
```

또한, recalculate() 함수를 trap() 함수에서 호출하기 위해 proc.h에 recalculate() 함수를 선언하는 부분을 추가했다.

### 3-4. 우선순위의 초기화

과제 명세에 따르면, get_initial_priority() 함수를 이용해 우선순위를 초기화하는 경우는 (1) 프로세스가 생성될 때와(2) SLEEPING 상태에서 RUNNABLE 상태로 바뀌는 wakeup이 발생할 때, 총 두 가지가 있다. (1)을 처리하기 위해 allocproc() 함수 내에서 프로세스를 생성하는 것과 동시에 우선순위를 초기화해서 큐에 삽입하는 것을 구현했고, (2)를 처리하기 위해 wakeup1() 함수 내에서 우선순위를 다시 계산하도록 구현했다.

프로세스의 초기 우선순위를 계산하는 get_initial_priority() 함수와 60ticks 마다 모든 프로세스의 우선순위를 계산하기 위한 recalculate() 함수를 만들었다.

```c
static struct proc* allocproc(void){
  // ...
found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  // init process info for Assignment #3, and push to queue
  p->priority = get_initial_priority();
  if(p->pid <= 2) p->priority = 99; // idle process's priority is always 99
  p->proc_tick = 0;
  p->cpu_used = 0;
  p->priority_tick = 0;
  p->end_tick = 0x3f3f3f3f;
  queue_push(p);
  // ...
}

static void wakeup1(void *chan){
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      // recalculate priority when wakeup(SLEEPING -> RUNNABLE)
      p->priority = get_initial_priority();
      if(p->pid <= 2) p->priority = 99; // idle process's priority is always 99
    }
}
```

### 3-5. trap() 함수 수정

60ticks마다 우선순위를 다시 계산하기 위해 trap() 함수 내에 static 변수 sche_tick를 선언했다. 이 변수는 매 tick마다 1씩 증가하면서 60이 되면 recalculate() 함수를 호출하고 다시 0이 된다. 또한, 매 tick마다 실행되고 있는 프로세스의 proc_tick, priority_tick, cpu_used의 값을 1씩 증가시켰다.

이후, 프로세스의 CPU 총 사용 시간이 종료 시간 이상이 되면 exit()을 이용해 프로세스를 종료했고, sche_tick의 값이 60이 되면 recalculate() 함수를 이용해 우선순위를 재계산하고, proc_tick()이 30이 되면 yield()를 이용해 제어권을 운영체제에 넘겨서 스케줄러를 통해 실행할 프로세스를 선택하도록 구현했다.

이때, 스케줄러의 선택 과정을 조금 더 명확하게 볼 수 있도록, pid가 0, 1, 2인 프로세스와 parent의 pid가 2인 프로세스, 즉 scheduler_test 프로세스는 tick을 계산하는데 포함하지 않고, 매 틱마다 yield()를 호출하도록 구현했다.

```c
void trap(struct trapframe *tf){
  static int sche_tick = 0; // timer for recalculate priority

  ...
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    ...
    // CPU use!
    if(myproc() && myproc()->pid >= 3 && myproc()->state == RUNNING){
      myproc()->proc_tick += 1;
      myproc()->priority_tick += 1;
      myproc()->cpu_used += 1;
      if(myproc()->parent->pid != 2) sche_tick += 1;
    }
    ...
  }
  ...
  // terminate process
  // if pid >= 3 and cpu_used >= end_tick then exit();

  // recalculate priority
  // if state = RUNNING and sche_tick >= 60 then sche_tick <- 0 and recalculate priority
  
  // yield and select process to run
  // if state = RUNNING and proc_tick >= 30 then yield();
  ...
}
```

### 3-6. 스케줄링 알고리즘 구현

번호가 작은 큐부터 차례대로 보면서, 우선순위가 가장 작은(높은) 프로세스를 찾아서 컨텍스트 스위칭을 하도록 구현했다.

```c
void scheduler(void){
  ...
  // calculate priority of processes initially
  recalculate();

  for(;;){
    // ...
    // find minimum priority process
    for(int q=0; q<25; q++){
      for(int x=que[q].head; x!=-1; x=node_pool[x].nxt){
        struct proc *now = node_pool[x].cur;
        if(now->state != RUNNABLE) continue;
        if(!p || p->state != RUNNABLE  || p->priority > now->priority) p = now;
        else if(p->priority == now->priority){
          if(p->pid < 2) p = now;
          else if(p->pid <= 2 && now->pid > 2) p = now;
        }
      }
    }
    // ...
  }
}
```

# 4. 결과

생략

# 5. 소스 코드

### 5-1. Makefile 수정

```makefile
...
ifeq ($(debug), 1)
  CFLAGS += -DDEBUG
endif
...
UPROGS=\
  ...
  _scheduler_test\
...
EXTRA=\
  ...
  _scheduler_test\
```

### 5-2. scheduler_test.c 구현

```c
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
```

### 5-3. proc.c 수정

```c
// Per-process state
struct proc {
  // ...
  int priority;                // Assignment 3-2: process priority
  int proc_tick;               // Assignment 3-2: process run time in current TQ
  int cpu_used;                // Assignment 3-2: process total run time
  int priority_tick;           // Assignment 3-2: process run time brfore rescheduling
  int end_tick;                // Assignment 3-2: process deadline
};

void recalculate();
```

### 5-4. proc.c 수정

```c
// linked list node
// cur : node data
// nxt : index of next node
// used : 1 if node has been used
struct my_node{
  struct proc *cur;
  int nxt, used;
};

struct my_node node_pool[NPROC*2];

// queue implemented with linked list
// 0 ~ 24 : queue for scheduling
// 25 : temp queue for priority recalculate
struct my_queue{
  int head, tail;
} que[26];

void queue_init(){
  for(int i=0; i<NPROC*2; i++){
    node_pool[i].cur = 0;
    node_pool[i].nxt = -1;
    node_pool[i].used = 0;
  }
  for(int i=0; i<26; i++){
    que[i].head = que[i].tail = -1;
  }
}

void _queue_push(int idx, struct proc *p){
  int now = -1;
  for(int i=0; i<NPROC*2; i++) if(!node_pool[i].used) { now = i; break; }
  node_pool[now].cur = p;
  node_pool[now].nxt = -1;
  node_pool[now].used = 1;
  if(que[idx].head == -1) que[idx].head = que[idx].tail = now;
  else node_pool[que[idx].tail].nxt = now, que[idx].tail = now;
}

void queue_push(struct proc *p){
  _queue_push(p->priority / 4, p);
}

void queue_clear(int idx){
  for(int i=que[idx].head; i!=-1; i=node_pool[i].nxt) node_pool[i].used = 0;
  que[idx].head = que[idx].tail = -1;
}

void
pinit(void)
{
  queue_init(); // init queue
  initlock(&ptable.lock, "ptable");
}
...
// recalculate priority
void recalculate(){
  // push every processes to que[25]
  for(int q=0; q<25; q++){
    for(int x=que[q].head; x!=-1; x=node_pool[x].nxt) _queue_push(25, node_pool[x].cur);
    queue_clear(q);
  }
  // recalculate priority
  for(int x=que[25].head; x!=-1; x=node_pool[x].nxt){
    struct proc *p = node_pool[x].cur;
    p->priority += p->priority_tick / 10;
    if(p->pid <= 2 || p->priority > 99) p->priority = 99;
    p->priority_tick = 0;
    queue_push(p);
  }
  queue_clear(25);
}

// get initial priority
int get_initial_priority(){
  int res = 100;
  for(int q=0; q<25; q++){
    for(int x=que[q].head; x!=-1; x=node_pool[x].nxt){
      struct proc *p = node_pool[x].cur;
      if(p->pid >= 3 && res > p->priority) res = p->priority;
    }
    if(res < 100) return res;
  }
  return 0;
}

// set_sche_info syscall implement
int set_sche_info(int priority, int end_tick){
  if(myproc() != 0){
    cprintf("set_sche_info() pid = %d\n", myproc()->pid);
    myproc()->priority = priority;
    myproc()->end_tick = end_tick;
  }
  return 0;
}
...
static struct proc* allocproc(void){
  ...  
  found:
  ...
  // init process info for Assignment #3, and push to queue
  p->priority = get_initial_priority();
  if(p->pid <= 2) p->priority = 99; // idle process's priority is always 99
  p->proc_tick = 0;
  p->cpu_used = 0;
  p->priority_tick = 0;
  p->end_tick = 0x3f3f3f3f;
  queue_push(p);
  ...
}

void scheduler(void){
  ...
  // calculate priority of processes initially
  recalculate();
  for(;;){
    // Enable interrupts on this processor.
    sti();
    acquire(&ptable.lock);
    // find minimum priority process
    for(int q=0; q<25; q++){
      for(int x=que[q].head; x!=-1; x=node_pool[x].nxt){
        struct proc *now = node_pool[x].cur;
        if(now->state != RUNNABLE) continue;
        if(!p || p->state != RUNNABLE  || p->priority > now->priority) p = now;
        else if(p->priority == now->priority){
          if(p->pid < 2) p = now;
          else if(p->pid <= 2 && now->pid > 2) p = now;
        }
      }
    }
    // ...
  }
}

void yield(void){
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  myproc()->proc_tick = 0; // clear proc_tick when yield
  sched();
  release(&ptable.lock);
}

static void wakeup1(void *chan){
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      // recalculate priority when wakeup(SLEEPING -> RUNNABLE)
      p->priority = get_initial_priority();
      if(p->pid <= 2) p->priority = 99; // idle process's priority is always 99
    }
}
```

### 5-5. trap.c 수정

```c
void
trap(struct trapframe *tf)
{
  static int sche_tick = 0; // timer for recalculate priority
  ...
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    ...
    // CPU use!
    if(myproc() != 0 && myproc()->pid >= 3 && myproc()->state == RUNNING){
      myproc()->proc_tick += 1;
      myproc()->priority_tick += 1;
      myproc()->cpu_used += 1;
      if(myproc()->parent->pid != 2) sche_tick += 1; // for convenience of output, we don't count ticks from scheduler_test
    }
    ...
  }
  ...
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
```

### 5-6. syscall.h 수정

```c
...
#define SYS_set_sche_info 24
```

### 5-7. syscall.c 수정

```c
...
extern int sys_set_sche_info(void);

static int (*syscalls[])(void) = {
...
[SYS_set_sche_info] sys_set_sche_info,
};
```

### 5-8. user.h 수정

```c
// system calls
...
int set_sche_info(int, int);
```

### 5-9. usys.S 수정

```
...
SYSCALL(set_sche_info)
```

### 5-10. sysproc.c 수정

```c
int
sys_set_sche_info(void){
  int priority, end_tick;
  if(argint(0, &priority) < 0) return -1;
  if(argint(1, &end_tick) < 0) return -1; 
  return set_sche_info(priority, end_tick);
}
```

### 5-11. defs.h 수정

```c
int             set_sche_info(int priority, int end_tick);
```

