/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : test
 ************************************************/
#include <unistd.h>
#include <sys_call.h>
pid_t child;

int main(int argc, char **argv)
{
  if ((child = fork())){
    //parent
    while (1){
      int i;
      for (i = 0 ; i < 1000000; i++)
        ;//delay

      sys_call_1(3);
    }

  }else{
    //child
    while (2){
      int i;
      for (i = 0 ; i < 1000000; i++)
        ;//delay

      sys_call_1(4);
    }
  }
  return 0;
}
