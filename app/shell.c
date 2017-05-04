#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ioctl.h>

#define BUFFER_LEN 4096
#define CMD_NAME_LEN 128
#define ARG_MAX_NUM 32
#define MAX_CUR_DIR 4096

#define SHELL_LOG "Ray@yatos:"
#define SHELL_LOG_LEN 10
#define SHELL_LOG_COLOR 0x2
#define SHELL_DIR_COLOR 0X6
#define SHELL_NORMAL_COLOR 0x7

char cmd_buffer[BUFFER_LEN];
char arg_buffer[BUFFER_LEN];
char cmd_name[CMD_NAME_LEN];
char * args[ARG_MAX_NUM];
char cur_dir[MAX_CUR_DIR];
pid_t child;


static void parse_real_path(const char * path, char *ans)
{
  if (!path || !*path)
    return ;
  if (path[0] != '/')
    return ;
  char *cur_ans = ans;
  *cur_ans = '/';

  const char * cur_name = path + 1;
  int len = 0;
  while (*cur_name){
    if (*cur_name != '/' && *(cur_name - 1) == '/'){
      len = 0;
      while (cur_name[len] && cur_name[len] != '/')
        len++;
      if (len == 2 && !strncmp(cur_name, "..", len)){
        //backto parent dir
        while (*cur_ans != '/')
          cur_ans--;
        if (cur_ans != ans )
          cur_ans--;
      }else if (len != 1 || strncmp(cur_name, "." , len)){
        //normal
        if (*cur_ans != '/')
          * ++cur_ans = '/';
        memcpy(cur_ans + 1, cur_name, len);
        cur_ans += len;
      }
      cur_name += len;
      continue;
    }
    cur_name++;
  }
  cur_ans[1] = '\0';
}

void do_cmd_cd(const char * path)
{
  if (chdir(path) < 0){
    printf("can not chdir to %s\n", path);
    return ;
  }
  if (!path || !*path)
    return ;

  char buffer[MAX_CUR_DIR];
  if (path[0] == '/')
    parse_real_path(path, cur_dir);
  else{
    sprintf(buffer, "%s/%s", cur_dir, path);
    parse_real_path(buffer, cur_dir);
  }
}

void do_cmd_clear()
{
  ioctl(0, 3, 0);
}

//return 1 mean we should read more string
int setup_args(const char * arg_str)
{
  const char * cur = arg_str;
  int cur_args_p = 0;
  int brac1 = 0;
  int brac2 = 0;
  char * cur_cp = arg_buffer;
  int start = 0;

  while (*cur){

    if (*cur == '\\'){
      cur++;
      if (*cur){
        *cur_cp = *cur;
        if (!start){
          args[cur_args_p++] = cur_cp;
          start = 1;
        }
        cur_cp++;
      }else
        return 1;
    } else if (*cur == '\"'){
      if (brac2)
        *cur_cp++ = *cur;
      else
        brac1 = !brac1;
    }else if (*cur == '\''){
      if (brac1)
        *cur_cp++ = *cur;
      else
        brac2 = !brac2;
    }else if (*cur == ' ' || *cur == '\t'){
      if (brac1 || brac2)
        *cur_cp++ = *cur;
      else{
        if (start){
          *cur_cp++ = '\0';
          start = 0;
        }
      }
    }else {
      *cur_cp = *cur;
      if (!start){
        args[cur_args_p++] = cur_cp;
        start = 1;
      }
      cur_cp++;
    }
    cur++;
  }

  if (brac1 || brac2)
    return 1;

  *cur_cp = '\0';
  args[cur_args_p] = NULL;
  return 0;
}



int main(int argc, char *argv[])
{
  int n;
  int str_len;
  sprintf(cur_dir, "%s", "/");
  while (1){
    cmd_buffer[0] = '\0';
    //set color and print something
    ioctl(0, 4, SHELL_LOG_COLOR);
    write(STDOUT_FILENO, SHELL_LOG, SHELL_LOG_LEN);

    ioctl(0, 4, SHELL_DIR_COLOR);
    write(STDOUT_FILENO, cur_dir, strlen(cur_dir));
    write(STDOUT_FILENO, "# ", 2);

    ioctl(0, 4, SHELL_NORMAL_COLOR);

    //read commands
    n = 0;
    while (1){
      n += read(STDIN_FILENO, cmd_buffer + strlen(cmd_buffer), BUFFER_LEN);
      if (n < 0){
        printf("read cmd error\n");
        return 1;
      }
      cmd_buffer[n] = '\0';
      str_len = strlen(cmd_buffer);
      if (cmd_buffer[str_len - 1] == '\n'){
        cmd_buffer[str_len - 1] = '\0';
        n--;
      }
      if (setup_args(cmd_buffer))
        write(STDOUT_FILENO, ">", 1);
      else
        break;
    }
    if (args[0] == NULL)
      continue;

    if (!strcmp(args[0], "cd")){
      do_cmd_cd(args[1]);
      continue;
    }
    if (!strcmp(args[0], "clear")){
      do_cmd_clear();
      continue;
    }


    if ((args[0][0] == '.' && args[0][1] =='/')
        ||
        (args[0][0] == '.' && args[0][1] == '.' && args[0][2] == '/')
        ||
        (args[0][0] == '/'))
      strcpy(cmd_name, args[0]);
    else
      //we use default PATH = /bin/
      sprintf(cmd_name, "/bin/%s", args[0]);

    //for child to execve cmd
    child = fork();
    if (child < 0){
      printf("fork error\n");
      return 1;
    }

    if (child == 0){
      execve(cmd_name, args, args);
      printf("can not execve %s\n", cmd_name);
      return  1;
    }
    int ret;
    waitpid(-1, &ret, 0);
  }
  return 0;
}
