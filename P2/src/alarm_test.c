#include "types.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]){
  int seconds;
  struct rtcdate r;

  if(argc <= 1)
    exit();

  seconds = atoi(argv[1]);

  alarm(seconds);

  date(&r);
  printf(1, "SSU_Alarm Start\n");
  printf(1, "Current time : %d-%d-%d %d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);

  while(1)
    ;
  exit();
}
