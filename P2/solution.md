# 1. 개요

본 과제는 (1) date() 시스템 콜과 이를 호출하는 datetest 쉘 프로그램의 작성을 통해 새로운 시스템 콜을 추가하는 방법을 배우고, (2) alarm() 시스템 콜과 이를 호출하는 alarm_test 쉘 프로그램을 작성하는 것을 통해 proc 구조체와 trap 함수의 작동 방법, 그리고 kernel mode와 user mode 사이의 전환을 배우며, (3) 리눅스 시스템에서 컴파일한 결과물을 xv6 상에서 실행하는 크로스 컴파일을 공부하기 위한 실습 과제이다.

date 시스템 콜과 datetest 쉘 프로그램은 rtcdate 구조체와 cmostime을 이용해 날짜와 시간 정보를 출력하는 프로그램으로, xv6 운영체제에 시스템 콜을 추가하는 과정과 argptr 함수를 이용해 user mode에서 kernel mode로 함수 인자를 전달하는 방법을 배울 수 있다.

alarm 시스템 콜과 alarm_test 쉘 프로그램은 일정 시간이 지나면 “SSU_Alarm!”을 출력하고 종료되는 프로그램으로, 프로세스의 정보 추적을 위해 struct proc 구조체를 수정하는 방법, 틱마다 발생하는 인터럽트를 처리하기 위한 trap 함수를 수정하는 방법, 그리고 커널 모드에서 콘솔창에 문자열을 출력하기 위한 cprintf 함수의 사용법을 배울 수 있다.

# 2. 상세 설계

### 2-1. date 설계

시스템 콜은 kernel mode에서 실행되지만, user mode에서 kernel mode로 직접 함수 인자를 넘길 수 없기 때문에 sys_date 함수의 파라미터는 void로 지정하고, argptr 함수를 이용해 struct rtcdate* 타입의 인자를 받아왔다. 그 후, cmostime 함수를 이용해 현재 날짜와 시간을 불러오는 방식으로 구현했다.

### 2-2. alarm 설계

처음에는 타이머가 시작하고 나서 흐른 ticks을 기준으로 시간을 측정하려고 했지만, tick의 주기가 일정하지 않아서 cmostime을 이용해 시작 시각과의 차이를 기준으로 타이머를 종료하는 방식으로 구현했다. 이런 방식으로 구현하기 위해서는 프로세스마다 (1) 타이머의 시작 여부(alarm_timer), (2) 타이머가 종료되어야 하는 시각(alarmticks), 총 두 가지 정보를 저장하고 있어야 한다. 따라서 struct proc 안에 int 타입의 필드를 2개 추가했다.

```c
struct proc{
  ...
  int alarmticks;
  int alarm_timer;
};
```

alarm_timer는 타이머의 시작 여부를 나타내는 변수로, 0이면 아직 타이머가 시작하지 않은 상태, 1이면 타이머가 돌아가고 있는 상태, 2면 타이머가 종료된 상태를 의미한다. 따라서 프로세스를 초기화할 때 alarm_timer의 값을 0으로 초기화했다.

```c
void userinit(void)
{
  ...
  p->alarmticks = 0;
  p->alarm_timer = 0; // 0 means not started
  ...
}
```

타이머의 종료 시각은 단순히 2020년 1월 1일부터 타이머가 시작하는 시점까지 흐른 초 단위 시간 + 타이머가 기다려야 하는 초 단위 시간(count)으로 지정했다.

```c
int sys_alarm(void){
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
```

타이머의 종료 여부를 확인하기 위해 매 틱마다(case T_IRQ0 + IRQ_TIMER) 현재 시각을 확인해서 타이머의 종료 시각 이후의 시점인지 확인해서 alarm_timer의 값을 2로 변경했다.

```c
void trap(struct trapframe *tf){
 ...
 switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    ...
    // if alarm is activated
    if(myproc() != 0 && myproc()->alarm_timer == 1){
      struct rtcdate d;
      cmostime(&d);
      // 현재 시간 계산(초 단위, cur_time)
      // ...

      // 종료 시간 이후면 알람 종료 후 출력
      if(myproc()->alarmticks <= cur_time){
        myproc()->alarmticks = 0;
        myproc()->alarm_timer = 2; // deactivate
        cprintf("SSU_Alarm!\n");
        cprintf("Current time : %d-%d-%d %d:%d:%d\n", d.year, d.month, d.day, d.hour, d.minute, d.second);
      }
    }
    ...
  }
  ...
}
```

이후, switch-case 문이 종료된 다음에 alarm_timer의 값이 2이면 exit()하는 것을 통해 프로세스를 종료했다.

```c
void trap(struct trapframe *tf){
  ..
  switch(tf->trapno){
    ...
  }
  // if timer is ended
  if(myproc() && myproc()->alarm_timer == 2){
    exit();
  }
  ...
}
```



# 3. 결과

생략

# 4. 소스 코드

### 4-1. Makefile 수정

```makefile
UPROGS=\
  ...
  _datetest\
  _alarm_test\

...

EXTRA=\
  ...
  datetest.c\
  alarm_test.c\
```

### 4-2. proc.h 수정

```c
struct proc {
  int alarmticks;              // Assignment 2-2: how many ticks have to pass
  int alarm_timer;             // Assignment 2-2: 0 means not started, 1 means activate, 2 means deactivate
};
```

### 4-3. proc.c 수정

```c
void
userinit(void)
{
  ...
  p->alarmticks = 0;
  p->alarm_timer = 0; // 0 means not started

  release(&ptable.lock);
}
```

### 4-4. syscall.h 수정

```c
...
#define SYS_date   22
#define SYS_alarm  23
```

### 4-5. syscall.c 수정

```c
...
extern int sys_date(void);
extern int sys_alarm(void);

...

static int (*syscalls[])(void) = {
...
[SYS_date]    sys_date,
[SYS_alarm]   sys_alarm,
};
```

### 4-6. user.h 수정

```c
// system calls
...
int date(struct rtcdate*);
int alarm(int);
```

### 4-7. usys.S 수정

```
...
SYSCALL(date)
SYSCALL(alarm)
```

### 4-8. sysproc.h 수정

```c
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
```

### 4-9. trap.c 수정

```c
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      ...
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
  ...
  }
  // if timer is ended
  if(myproc() && myproc()->alarm_timer == 2){
    exit();
  }
  // Force process exit if it has been killed and is in user space.
  ...
```

### 4-10. datetest.c

```c
#include "types.h"
#include "user.h"
#include "date.h"
int main(int argc, char *argv[]){
  struct rtcdate r;
  if(date(&r)){
    printf(1, "error\n");
    exit();
  }
  printf(1, "Current time: %d-%d-%d %d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);
  exit();
}
```

