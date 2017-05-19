/*
 *  Top level of all IPC
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/5/10 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/pipe.h>
#include <yatos/signal.h>

void ipc_init()
{
  pipe_init();
  sig_init();
}
