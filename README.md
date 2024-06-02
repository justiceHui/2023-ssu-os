# 2023-ssu-os

> 2023년 2학기 숭실대학교 컴퓨터학부 운영체제 (홍지만 교수님) 설계 과제<br>평점 A+, 과제 구현 점수 만점

### 리포지토리 구조

* `README.md`: 과제 명세 요약
* `solution.md`: 보고서(과제 개요, 상세 설계, 소스 코드 수정 내역)
* `src/`: 기존 xv6에서 변경된 파일

### 설계 과제 요약 (과제 목표)

1. xv6 Tour
   * xv6 설치 및 컴파일
   * `Hello xv6 World` 출력하는 응용 프로그램 `helloxv6`를 위한 `helloxv6.c` 구현
   * 주어진 파일의 마지막 라인부터 주어진 라인 수만큼 역순으로 출력하는 응용 프로그램 `htac`를 위한 `htac.c` 구현
2. system calls in xv6
   * xv6에 새로운 시스템 콜 추가
   * Cross Compile 방법 학습 - xv6에는 텍스트 편집기와 컴파일러가 없기 때문에 리눅스 시스템에서 프로그램 작성&컴파일해서 나온 실행 파일을 xv6에서 실행
3. xv6 SSU Scheduler
   * xv6의 프로세스 관리 및 스케줄링 기법 이해
   * xv6의 스케줄러를 FreeBSD 5.4 이후 구현된 ULE(non-interactive) 스케줄러를 기반으로 하는 SSU 스케줄러로 수정 및 테스트 프로그램 구현
   * 프로세스의 우선순위와 프로세스가 사용한 CPU 시간을 스케줄링에 반영
   * CPU 코어 1개만 사용 → `Makefile` 수정 필요
4. SSU Memory Allocation and Multi-level File System
   * xv6 메모리 및 파일 시스템 구조 이해와 응용

### 관련 자료

* xv6: [https://github.com/mit-pdos/xv6-public](https://github.com/mit-pdos/xv6-public)
* 교재: [https://pages.cs.wisc.edu/~remzi/OSTEP/Korean/](https://pages.cs.wisc.edu/~remzi/OSTEP/Korean/)
* MIT 6.1810 Operating System Engineering: [https://pdos.csail.mit.edu/6.1810/](https://pdos.csail.mit.edu/6.1810/)

