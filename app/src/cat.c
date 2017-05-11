#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
char buffer[4096];
int main(int argc, char *argv[])
{
  int fd, n;
  if (argc == 1){
    while ((n = read(STDIN_FILENO, buffer, 4096)) > 0){
      write(STDOUT_FILENO, buffer, n);
    }
    return 0;
  }
  int i;
  for (i = 1; i < argc; i++){
    fd = open(argv[i], O_RDONLY);
    if (fd < 0){
      perror("file open failed!");
      return 1;
    }

    while ((n = read(fd, buffer, 4096)) > 0)
      write(STDOUT_FILENO, buffer, n);
  }
  return 0;
}
