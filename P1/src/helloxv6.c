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
