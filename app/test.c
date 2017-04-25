#include <stdio.h>
#include <unistd.h>
pid_t child;
int main(int argc, char *argv[])
{
  printf("test\n");
  if (!(child = fork()))
    execve("/test2", NULL, NULL);
  else{
    printf("test fork child = %d\n", child);
    usleep(3000000);
    printf("test exit 1\n");
  }
  return 1;
}
