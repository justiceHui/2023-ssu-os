# 설계과제 #2 - system calls in xv6

### 과제 목표

* xv6에 새로운 시스템 콜 추가
* Cross Compile 방법 학습 - xv6에는 텍스트 편집기와 컴파일러가 없기 때문에 리눅스 시스템에서 프로그램 작성&컴파일해서 나온 실행 파일을 xv6에서 실행

### 1. (구현) date 시스템 콜 추가

* `date()` 시스템 콜 추가
  * `date.h`에 있는 `rtcdate`와 커널 모드의 `cmostime()` 사용
* `datetest` 쉘 프로그램 구현
  * `date()`의 실행을 확인하는 쉘 프로그램 구현
  * `Current time : 2023-9-11 7:58:57` 과 같은 형식으로 출력

### 2. (구현) alarm 시스템 콜 추가

* `alarm()` 시스템 콜 추가
  * 입력받은 시간(초 단위)이 지날 시 `SSU_Alarm!`과 종료 시각을 출력한 뒤 프로그램을 종료하는 시스템 콜 `alarm()` 구현
  * 타이머 인터럽트를 사용하기 위해 `trap.c` 내부에 있는 `trap(struct trapframe *tf)` 수정
  * 각 프로세스마다 `alarmticks`를 저장할 수 있도록  `proc.h`에서 `struct proc`을 수정
* `alarm_test` 쉘 프로그램을 이용해 확인
  * 제공된 `alarm_test.c` 코드를 **수정하지 않고** 그대로 사용

### 배점

1. 10점(필수)
2. 90점
