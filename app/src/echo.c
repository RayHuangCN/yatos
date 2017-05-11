#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
int flag_n;
int main(int argc, char *argv[])
{
  int i = 1;
  if (argc >= 2 && !strcmp(argv[1], "-n")){
    flag_n = 1;
    i++;
  }
  for (; i < argc; i++){
    if (i != argc - 1)
      printf("%s ", argv[i]);
    else
      printf("%s", argv[i]);
  }
  if (!flag_n)
    printf("\n");

  return 0;
}
