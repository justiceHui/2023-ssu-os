# 설계과제 #3 - xv6 SSU Scheduler

### 과제 목표

* xv6의 프로세스 관리 및 스케줄링 기법 이해
* xv6의 스케줄러를 FreeBSD 5.4 이후 구현된 ULE(non-interactive) 스케줄러를 기반으로 하는 SSU 스케줄러로 수정 및 테스트 프로그램 구현
* 프로세스의 우선순위와 프로세스가 사용한 CPU 시간을 스케줄링에 반영
* CPU 코어 1개만 사용 → `Makefile` 수정 필요

### 1. (분석) 기존 xv6 스케줄러 분석

* 함수 단위로 상세하게 분석
* 함수 간의 콜 그래프 필수 포함

### 2. (구현) SSU Scheduler 구현

* 스케줄링 방식
  * 모든 프로세스는 CPU 사용량에 따라 우선순위(priority)를 가짐
  * priority 값이 작을수록 우선순위가 높음
  * time quantum(time slice)는 30ticks로 설정
  * 우선순위는 0 이상 99 이하의 정수
  * 25개의 run queue는 각각 4개의 priority 값을 관리 (0 ≤ x < 25번 큐는 priority 값이 4x ~ 4x+3 인 프로세스 관리)
  * 우선순위가 높은 프로세스를 관리하는 비어있지 않은 큐 탐색 후 RUNNABLE 상태이면서 우선순위가 가장 높은 프로세스 선택
* 프로세스 정보
  * 각 프로세스는 PID(프로세스 아이디), state(상태), priority(우선순위), proc_tick, priority_tick 값을 가짐
  * 어떤 프로세스가 실행되고 있다면, 각 tick마다 해당 프로세스의 proc_tick 값을 증가시켜서 CPU 사용량 계산
  * priority_tick은 마지막 우선순위 재계산 이후 CPU를 사용한 시간을 나타냄
* 우선순위 조정
  * 어떤 프로세스가 실행되면, 각 tick마다 proc_tick 값 증가시켜서 CPU 사용량 계산
  * priority 값은 우선순위 재계산 함수가 호출될 때마다 다음 규칙에 따라 갱신됨
    * new_priority = old_priority + priroty_tick / 10
  * 우선순위 재계산 함수는 60ticks마다 실행
* 우선순위 초기화
  * 프로세스 생성 또는 wake up 시 priority를 0으로 부여하면 해당 프로세스가 CPU를 독점할 수 있음
  * 따라서 run queue에 존재하는 프로세스의 priority 값의 최솟값으로 부여

### 3. (구현) 스케줄링 테스트

* 스케줄링을 테스트할 수 있는 `scheduler_test.c` 프로그램 구현
  * `fork()`로 생성할 프로세스의 개수 `PNUM` 선언 (기본값 3)
  * `scheduler_test`는 생성된 프로세스가 모두 종료되도록 구현
  * 아래 이벤트가 발생할 때마다 PID, priority, proc_tick, total_cpu_usage와 함께 이벤트 번호를 출력
    1. time quantum 종료 시점
    2. context switch 직전
    3. 프로세스 종료 시점
  * `PID : 5, priority : 12, proc_tick : 30 ticks, total_cpu_usage : 60 ticks (3)`과 같이 출력
*  초기 priority를 설정할 수 있는 `set_sche_info()` 시스템 콜 추가
  * 초기 priority와 종료 타이머(tick)을 인자로 받아서 스케줄링에 반영
  * 초기 priority를 설정하지 않으면 현재 runQ 내에서 관리하는 프로세스의 priority 값 중 가장 작은 값 부여 (PID 1, 2) 제외

### 4. (구현) 프로세스 선정 과정 디버깅 기능 구현

* 스케줄링 과정에서 다음에 실행될 프로세스를 선택할 때마다 프로세스의 PID, 프로세스 이름, CPU 사용 시간(proc_tick), 프로세스 우선순위 출력
* `cprintf("PID: %d, NAME: %s\n", p->pid, p->name);`과 같이 출력

### 5. (분석) 기존 xv6 스케줄러와 SSU 스케줄러의 기능 및 성능 비교 분석

* 기능의 차이는 도표, 성능의 차이는 그래프 포함

### 배점

1. 20점(분석)
2. 30점(구현, 필수)
3. 20점(구현, 필수)
4. 20점(구현, 필수)
5. 10점(분석)

