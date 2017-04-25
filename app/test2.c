#include <unistd.h>
#include <stdio.h>
int main(int argc, char *argv[])
{
  printf("test2\n");
  usleep(6000000);
  printf("test2 exit 5\n");
  return 5;
}
