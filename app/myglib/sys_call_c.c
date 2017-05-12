/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/12
 *   Email : rayhuang@126.com
 *   Desc  : sys_call helper
 ************************************************/
#include "sys_call.h"
#include <errno.h>
#include <unistd.h>

int sys_call_1_c(unsigned long call_num)
{
  int ret;
  ret = __sys_call_1(call_num);
  if (ret < 0){
    (*__errno_location()) = -ret;
    return -1;
  }
  return ret;
}


int sys_call_2_c(unsigned long call_num, unsigned long arg1)
{
  int ret;
  ret = __sys_call_2(call_num, arg1);
  if (ret < 0){
    (*__errno_location()) = -ret;
    return -1;
  }
  return  ret;
}

int sys_call_3_c(unsigned long call_num, unsigned long arg1, unsigned long arg2)
{
  int ret;
  ret = __sys_call_3(call_num, arg1, arg2);
  if (ret < 0){
    (*__errno_location()) = -ret;
    return -1;
  }
  return ret;
}
int sys_call_4_c(unsigned long call_num, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
  int ret;
  ret = __sys_call_4(call_num, arg1, arg2, arg3);
  if (ret < 0){
    (*__errno_location()) = -ret;
    return -1;
  }
  return ret;
}
