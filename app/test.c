#include <stdio.h>

int main(int argc, char *argv[])
{

  printf("argc = %d\n", argc);
  int i;
  for (i = 0; i < argc; i++)
    printf("arg%d = %s\n", i, argv[i]);
  while (1);

  return 0;
}
