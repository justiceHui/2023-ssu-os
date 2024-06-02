# 1. 개요

본 과제는 (1) 교육용 오픈소스 운영체제인 xv6을 설치 및 컴파일하고, (2) helloxv6와 htac 쉘 프로그램을 작성하는 과정을 통해 xv6에서의 응용 프로그램 개발 방법을 공부하기 위한 실습 과제이다.

helloxv6 응용 프로그램은 “Hello xv6 World”를 출력하는 프로그램으로, 이 프로그램을 개발하는 과정을 통해 xv6 응용 프로그램 개발을 위한 C언어 작성 방법과 Makefile 편집 방법을 배울 수 있다.

htac 쉘 프로그램은 출력할 행의 수 n과 파일 이름이 주어지면, 파일의 마지막 행부터 n개의 행을 역순으로 출력하는 프로그램으로, 이 프로그램을 개발하는 과정을 통해 xv6 운영체제에서의 파일 입력 방법과 메모리 동적 할당 함수 malloc 사용법에 대해 배울 수 있다.

# 2. 상세 설계

### 2-1. helloxv6 응용 프로그램 작성

printf 함수를 이용해 콘솔 창에 문자열을 출력하고, exit 함수를 이용해 프로그램을 종료했다. 이때, printf는 stdio.h가 아닌 user.h에 있는 printf를 사용했으며, 함수 원형이 printf(int fd, char *fmt, ...) 꼴이기 때문에 첫 번째 인자로는 stdout의 file descriptor인 1을, 두 번째 인자는 출력하고자 하는 문자열인 “Hello xv6 World\n”을 넘겨주었다.

### 2-2. htac 쉘 프로그램 작성

먼저, 구현의 편의를 위해 C++ 표준 라이브러리의 `std::vector<int>` 와 같은 32비트 정수형 데이터를 저장하는 동적 배열 struct _vector_int를 vector.h에 구현했다.    

struct _vector_int 구조체는 다음과 같이 생겼다. ptr은 할당받은 공간의 주소를 저장하는 int형 포인터, size와 capacity는 각각 실제로 저장되어 있는 원소의 개수와 할당되어 있는 공간의 크기를 의미한다.

```c
typedef struct _vector_int{
    int *ptr;
    int size, capacity;
} vector_int;
```

동적 배열의 구현을 위해 배열을 생성/소멸시키는 함수, 할당되어 있는 공간의 크기를 조절하는 함수, 배열의 맨 뒤에 원소를 삽입/삭제하는 함수를 구현했다.

```c
vector_int* new_vector_int();
void delete_vector_int(vector_int*);
void _vector_int_reserve(vector_int*, int);
void _vector_int_double_up(vector_int*);
void _vector_int_double_down(vector_int*);
void push_back_int(vector_int*, int);
void pop_back_int(vector_int*);
```

new_vector_int 함수와 delete_vector_int 함수는 동적 배열의 생성과 소멸을 수행하는 함수이다.

_vector_int_reserve 함수는 동적 배열의 capacity를 조절하는 함수로, 인자로 받은 개수 만큼의 int형 데이터를 저장할 수 있는 공간을 새로 생성한 다음, 기존에 저장되어 있는 값을 모두 복사하는 방식으로 수행된다.

_vector_int_double_up 함수는 각각 배열이 가득 찼을 때 배열의 크기를 2배로 확장하는 함수,  _vector_int_double_down 함수는 원소 개수가 할당받은 공간의 1/4 이하가 되었을 때 배열의 크기를 절반으로 축소하는 함수이다. 이 두 함수를 이용하면 원소의 삽입/삭제를 amortized O(1) 시간에 수행할 수 있으며, 공간 복잡도 또한 선형에 비례하는, 최적에 가까운 효율을 얻을 수 있다.

push_back_int 함수와 pop_back_int 함수는 동적 배열의 맨 뒤에 원소를 삽입/삭제하는 함수로, 위에서 언급한 array doubling 방법을 이용해 상수 시간에 동작하도록 구현되어 있다.    

htac 쉘 프로그램의 실제 구현은 파일의 내용을 전부 읽은 다음, 각 행이 몇 번째 글자부터 몇 번째 글자를 담당하는지 효율적으로 구한 뒤, 마지막 n줄을 출력하는 방식으로 구현되어 있다. char＊ 타입으로 주어지는 n을 정수로 변환하기 위해 str_to_unsigned_int 함수를, htac 기능 구현을 위해 file descriptor를 받아서 파일의 모든 내용을 읽어 동적 배열에 저장하는 read_line 함수를 구현해서 사용했다.

```c
int str_to_unsigned_int(char*)
int read_line(int fd);
```

str_to_unsigned_int 함수는 문자열을 받은 다음, 문자열이 숫자로만 구성되어 있으면 이를 수로 변환한 값을, 그렇지 않으면 –1을 반환하는 함수다.

read_line 함수는 file descriptor를 받아 파일의 내용을 동적 배열 file에, 각 행이 몇 글자로 구성되어 있는지를 동적 배열 line_length에 저장한 다음, 파일의 행 개수를 반환하는 함수다. 이 함수를 이용해 각 행의 길이를 구한 다음, 이 배열의 누적 합을 이용하면 (0-based) i번째 행이 [ line_length[i-1], line_length[i] ) 구간에 있음을 알 수 있다. 따라서 출력하고자 하는 행의 번호가 주어지면, 구간의 시작점과 끝점을 상수 시간에 구해서 효율적으로 파일 내용을 출력할 수 있다.

# 3. 결과

생략

# 4. 소스 코드

### 4-1. helloxv6.c

```c
#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv){
  // stdio.h 가 아닌 user.h 에 있는 printf 사용
  // printf(int fd, char *fmt, ...)
  // 1은 stdout의 file descriptor
  printf(1, "Hello xv6 World\n");
  exit();
}
```

### 4-2. vector.h

```c
#include "types.h"
#include "stat.h"
#include "user.h"

// C++의 std::vector<int>와 유사한 형태의 동적 배열을 구현
// 실제로 저장된 원소의 개수 `cnt`와 할당되어 있는 공간의 크기 `capacity`를 관리
// push_back_int와 pop_back_int 모두 amortized O(1) 시간에 동작함
typedef struct _vector_int{
  int *ptr;
  int size, capacity;
} vector_int;

// 새로운 동적 배열 생성
vector_int* new_vector_int(){
  vector_int* res = (vector_int*)malloc(sizeof(vector_int));
  res->ptr = (int*)malloc(sizeof(int) * 4);
  res->size = 0;
  res->capacity = 4;
  return res;
}

// 동적 배열 할당 해제
void delete_vector_int(vector_int* vec){
  free(vec->ptr);
  free(vec);
}

// capacity 변경 후 재할당 & 복사
void _vector_int_reserve(vector_int *vec, int cap){
  vec->capacity = cap;
  int *new_ptr = (int*)malloc(sizeof(int) * vec->capacity);
  for(int i=0; i<vec->size; i++) new_ptr[i] = vec->ptr[i];
  free(vec->ptr);
  vec->ptr = new_ptr;
}

// 원소를 추가할 공간이 없는 경우 기존의 2배 크기로 확장
void _vector_int_double_up(vector_int *vec){
  if(vec->size == vec->capacity) _vector_int_reserve(vec, vec->capacity * 2);
}

// 원소의 개수가 할당받은 공간의 1/4 이하가 되면 기존 크기의 절반으로 축소
void _vector_int_double_down(vector_int *vec){
  if(vec->capacity > 4 && vec->size * 4 <= vec->capacity) _vector_int_reserve(vec, vec->capacity / 2);
}

// 맨 위에 원소 추가
// 시간 복잡도: amortized O(1)
void push_back_int(vector_int* vec, int v){
  _vector_int_double_up(vec);
  vec->ptr[vec->size++] = v;
}

// 맨 뒤에 있는 원소 제거
// 시간 복잡도: amortized O(1)
void pop_back_int(vector_int* vec){
  vec->size -= 1;
  _vector_int_double_down(vec);
}
```

### 4-3. htac.c

```cpp
#include "vector.h"

// 파일 입력 버퍼 크기 지정
#define BUFFER_SIZE 512

// 문자열을 32비트 정수로 변환
// 음이 아닌 정수가 주어지지 않은 경우 -1 반환
int str_to_unsigned_int(char *s){
  int res = 0;
  for(int i=0; s[i]; i++){
    if('0' <= s[i] && s[i] <= '9') res = res * 10 + s[i] - '0';
    else return -1;
  }
  return res;
}

int line; // 출력해야 하는 줄의 개수
char buf[BUFFER_SIZE]; // 입력 버퍼

vector_int* line_length; // 파일의 각 줄의 길이
vector_int* file; // 파일 내용

// 파일을 읽고 라인 개수를 반환
// 각 줄의 길이를 `line_length`에 저장
// 파일의 내용을 `file`에 저장
int read_file(int fd){
  int eol_count = 0, len;
  line_length = new_vector_int();
  file = new_vector_int();

  push_back_int(line_length, 0); // 첫 번째 줄

  while((len = read(fd, buf, sizeof(buf))) > 0){
    for(int i=0; i<len && buf[i]; i++){
      push_back_int(file, buf[i]);
      line_length->ptr[eol_count]++;

      if(buf[i] == '\n'){ // 개행
        eol_count++;
        push_back_int(line_length, 0);
      }
    }
  }
  if(len < 0){ // 파일 읽기 실패
    printf(1, "htac: read error\n");
    exit();
  }

  return eol_count + 1; // 줄 개수 = 개행 횟수 + 1
}

int main(int argc, char *argv[]){
  // 인자가 주어지지 않은 경우 아무것도 수행하지 않고 종료
  if(argc <= 2){
    exit();
  }

  line = str_to_unsigned_int(argv[1]);


  int fd = open(argv[2], 0);
  if(fd < 0){ // 파일 열기 실패
    printf(1, "htac: cannot open %s\n", argv[2]);
    exit();
  }

  int total_line = read_file(fd);

  // 파일의 마지막에 개행 문자 추가
  line_length->ptr[total_line-1]++;
  push_back_int(file, '\n');

  // 줄 길이의 누적 합을 구함
  // (0-based) i번째 줄은 [ line_length->ptr[i-1], line_length->ptr[i] ) 번째 글자
  for(int i=1; i<total_line; i++) line_length->ptr[i] += line_length->ptr[i-1];

  // 뒤에서부터 line 개의 줄 출력
  for(int i=0; i<line && i<total_line; i++){
    int idx = total_line - i - 1;
    int st = idx ? line_length->ptr[idx-1] : 0;
    int ed = line_length->ptr[idx];
    for(int j=st; j<ed; j++) printf(1, "%c", file->ptr[j]);
  }

  // 동적 배열 할당 해제
  delete_vector_int(line_length);
  delete_vector_int(file);

  exit();
}
```

### 4-4. Makefile 수정

```makfile
UPROGS=\
	...
	_helloxv6\
	_htac\

...

EXTRA=\
	...
	hellovx6.c\
	vector.h htac.c\
```

