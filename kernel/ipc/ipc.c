/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/10
 *   Email : rayhuang@126.com
 *   Desc  : ipc
 ************************************************/
#include <yatos/pipe.h>
#include <yatos/signal.h>
void ipc_init()
{
  pipe_init();
  sig_init();
}
