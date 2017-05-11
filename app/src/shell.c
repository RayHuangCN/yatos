#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>

#define BUFFER_LEN 4096
#define CMD_NAME_LEN 128
#define ARG_MAX_NUM 4096
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
char ** environ;

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
    perror("cd failed!");
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
int setup_args(char * arg_str)
{
  char * cur = arg_str;
  int cur_args_p = 0;
  int brac1 = 0;
  int brac2 = 0;
  char * cur_cp = arg_buffer;
  int start = 0;
  int append_c = -1;
  while (1){
    if (!*cur)
      return 1;
    append_c = -1;
    if (*cur == '\\'){
      cur++;
      if (*cur)
        append_c = *cur;
      else
        return 1;
    }else if (*cur == '\"'){
      if (brac2)
        append_c = *cur;
      else{
        brac1 = !brac1;
        append_c = -1;
      }
    }else if (*cur == '\''){
      if (brac1)
        append_c = *cur;
      else{
        brac2 = !brac2;
        append_c = -1;
      }
    }else if (*cur == ' ' || *cur == '\t'){
      if (brac1 || brac2)
        append_c = *cur;
      else
        append_c = '\0';
    }else if (*cur == '>' || *cur == '<' || *cur == '|'){
      if (brac1 || brac2)
        append_c = *cur;
      else{
        if (start){
          *cur_cp++ = '\0';
          start = 0;
        }
        args[cur_args_p++] = cur_cp;
        *cur_cp++ = *cur;
        if (*cur == '>' && *(cur + 1) == '>'){
          *cur_cp++ = '>';
          cur++;
        }
        *cur_cp++ = '\0';
        cur++;
        continue;
      }
    }else if (*cur == '\n'){
      if (brac1 || brac2)
        append_c = *cur;
      else
        break;
    }else {
      append_c = *cur;
    }
    if (append_c != -1){
      *cur_cp = append_c;
      if (append_c && !start){
        args[cur_args_p++] = cur_cp;
        start = 1;
      }
      if (!append_c && start)
        start = 0;
      cur_cp++;
    }
    cur++;
  }

  if (brac1 || brac2)
    return 1;

  *cur_cp = '\0';
  args[cur_args_p] = NULL;
  if (cur_args_p && !strcmp(args[cur_args_p - 1], "|")){
    if (*cur == '\n')
      *cur = ' ';
    return 1;
  }

  return 0;
}

int run_task(char ** argv)
{
  if (!argv || !argv[0] || !argv[0][0])
    return 1;

  pid_t child = fork();
  if (child < 0)
    return 1;
  if (!child){
    if ((argv[0][0] == '.' && argv[0][1] =='/')
        ||
        (argv[0][0] == '.' && argv[0][1] == '.' && argv[0][2] == '/')
        ||
        (argv[0][0] == '/'))
      strcpy(cmd_name, argv[0]);
    else
      //we use default PATH = /bin/
      sprintf(cmd_name, "/bin/%s", argv[0]);

    execve(cmd_name, argv, environ);
    char buffer[32];
    sprintf(buffer, "can not execve %s", argv[0]);
    perror(buffer);
    exit(1);
  }
  return 0;
}

int str_is_reio(const char * str)
{
  if (!strcmp(str, "<") ||
      !strcmp(str, ">") ||
      !strcmp(str, ">>") ||
      !strcmp(str, "|"))
    return 1;
  return 0;
}


void fd_set_cloexec(int fd, int cloexec)
{
  int flag = fcntl(fd, F_GETFD);
  if (cloexec)
    flag |= FD_CLOEXEC;
  else
    flag &= ~FD_CLOEXEC;
  fcntl(fd,F_SETFD, flag);
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
        perror("read cmd failed!");
        return 1;
      }
      cmd_buffer[n] = '\0';
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

    // now we set up all child task
    int child_total = 0;
    char ** cur_arg = args;
    char ** cur_run_cmd = args;
    int fd[2] ; //for pipe
    char errbuf[32];
    int openfd;
    int ret, i;
    //save stdio
    int save_stdin = dup(STDIN_FILENO);
    int save_stdout = dup(STDOUT_FILENO);

    while (*cur_arg){
      if (str_is_reio(*cur_arg)){
        dup3(save_stdout, STDOUT_FILENO, 0);
        printf("syntax error at \'%s\'!\n", *cur_arg);
        goto wait_for_finish;
      }
      cur_run_cmd = cur_arg;
      //find reio
      //we use "while" because ">" "<" and ">>" can appear more than once
      while (*cur_arg && strcmp(*cur_arg, "|")){
        //it's stdout reopen?
        if (!strcmp(*cur_arg, ">") || !strcmp(*cur_arg, ">>")){
          *cur_arg++;
          if (!*cur_arg || !strlen(*cur_arg) || str_is_reio(*cur_arg)){
            dup3(save_stdout, STDOUT_FILENO, 0);
            printf("syntax error at \'%s\'!\n", *(cur_arg - 1));
            goto wait_for_finish;
          }
          //append?
          if (!strcmp(*(cur_arg - 1), ">"))
            openfd = open(*cur_arg, O_WRONLY | O_CREAT | O_TRUNC, 0666);
          else
            openfd = open(*cur_arg, O_WRONLY | O_CREAT | O_APPEND, 0666);
          //open error?
          if (openfd < 0){
            dup3(save_stdout, STDOUT_FILENO, 0);
            sprintf(errbuf, "can not open %s", *cur_arg);
            perror(errbuf);
            goto wait_for_finish;
          }
          *(cur_arg - 1) = NULL;
          dup3(openfd, STDOUT_FILENO, 0);
          close(openfd);
        }
        //it's stdin reopen?
        if (!strcmp(*cur_arg, "<")){
          *cur_arg++;
          if (!*cur_arg || !strlen(*cur_arg) || str_is_reio(*cur_arg)){
            dup3(save_stdout, STDOUT_FILENO, 0);
            printf("syntax error at \'%s\'!\n", *(cur_arg - 1));
            goto wait_for_finish;
          }
          openfd = open(*cur_arg, O_RDONLY);
          if (openfd < 0){
            dup3(save_stdout, STDOUT_FILENO, 0);
            sprintf(errbuf, "can not open %s", *cur_arg);
            perror(errbuf);
            goto wait_for_finish;
          }
          *(cur_arg - 1) = NULL;
          dup3(openfd, STDIN_FILENO, 0);
          close(openfd);
        }
        cur_arg++;
      }

      //if we find a "|" we should build a pipe
      if (*cur_arg && !strcmp(*cur_arg, "|")){
        *cur_arg++ = NULL;
        if (pipe(fd) < 0){
          dup3(save_stdout, STDOUT_FILENO, 0);
          perror("can not build pipe!");
          goto wait_for_finish;
        }

        dup3(fd[1], STDOUT_FILENO, 0);
        close(fd[1]);
        fd_set_cloexec(fd[0], 1);
        ret = run_task(cur_run_cmd);
        dup3(save_stdout, STDOUT_FILENO, 0);
        dup3(fd[0], STDIN_FILENO, 0);
        close(fd[0]);

      }else
        ret = run_task(cur_run_cmd);

      if (ret){
        dup3(save_stdout, STDOUT_FILENO, 0);
        sprintf(errbuf, "can not execve %s!", *cur_run_cmd);
        perror(errbuf);
        goto wait_for_finish;
      }
      child_total++;
    }
    //restore stdio
    dup3(save_stdin, STDIN_FILENO, 0);
    dup3(save_stdout, STDOUT_FILENO, 0);

  wait_for_finish:
    //wait for all childs
    for (i = 0; i < child_total; i++)
      waitpid(-1, &ret, 0);
  }
  return 0;
}
