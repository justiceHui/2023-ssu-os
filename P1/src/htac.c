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
