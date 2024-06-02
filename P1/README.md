# 설계과제 #1 - xv6 Tour

### 과제 목표

* xv6 설치 및 컴파일
* `Hello xv6 World` 출력하는 응용 프로그램 `helloxv6`를 위한 `helloxv6.c` 구현
* 주어진 파일의 마지막 라인부터 주어진 라인 수만큼 역순으로 출력하는 응용 프로그램 `htac`를 위한 `htac.c` 구현

### 1. xv6 설치 및 컴파일

* xv6 다운로드 - `git clone https://github.com/mit-pdos/xv6-public`
* QEMU 다운로드 및 설치 - `apt-get install qemu-kvm`
* xv6 컴파일 및 실행 - `make & make qemu`

### 2. (구현) helloxv6 응용 프로그램 작성

* helloxv6 응용 프로그램 작성
* `Hello xv6 World`를 출력하는 `helloxv6.c` 응용 프로그램 작성
* `Makefile`을 수정하여 `helloxv6.c` 파일도 `make` 명령 실행 시 컴파일하도록 변경
* `xv6` 컴파일&실행 후 `helloxv6` 응용 프로그램이 잘 실행되는지 확인

### 3. (구현) htac 응용 프로그램 작성

* 첫 번째 인자로 주어진 파일의 마지막 행부터 두 번째 인자로 주어진 행 만큼 역순으로 출력하는 `htac.c` 응용 프로그램 작성
* 기존의 `cat.c` 파일 참고해서 `htac(int fd;)` 함수 구현
* `int line;`은 `htac.c`에서 전역 변수로 선언
* `xv6` 컴파일&실행 수 `htac` 응용프로그램이 잘 실행되는지 확인

### 배점

1. 30점(필수)
2. 20점(필수)
3. 50점(필수)
