/*
 * ptw32_threadReuse.c
 *
 * Description:
 * This translation unit implements miscellaneous thread functions.
 *
 * --------------------------------------------------------------------------
 *
 *	Pthreads-win32 - POSIX Threads Library for Win32
 *	Copyright(C) 1998 John E. Bossom
 *	Copyright(C) 1999,2002 Pthreads-win32 contributors
 * 
 *	Contact Email: rpj@ise.canberra.edu.au
 * 
 *	The current list of contributors is contained
 *	in the file CONTRIBUTORS included with the source
 *	code distribution. The list can also be seen at the
 *	following World Wide Web location:
 *	http://sources.redhat.com/pthreads-win32/contributors.html
 * 
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2 of the License, or (at your option) any later version.
 * 
 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 * 
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with this library in the file COPYING.LIB;
 *	if not, write to the Free Software Foundation, Inc.,
 *	59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "pthread.h"
#include "implement.h"


/*
 * The thread reuse stack is a simple LIFO stack managed through a singly
 * linked list element in the pthread_t struct.
 *
 * All thread structs on the stack are clean and ready for reuse.
 *
 * The pthread_t_ struct's prevReuse element can be tested to check for an invalid
 * thread ID. A NULL value indicates a valid thread. Applications should use the
 * pthread_kill() function with a zero signal value to test for a valid thread ID.
 */

/*
 * Pop a clean pthread_t struct off the reuse stack.
 */
pthread_t
ptw32_threadReusePop (void)
{
  pthread_t t;

  EnterCriticalSection(&ptw32_thread_reuse_lock);

  t = ptw32_threadReuseTop;

  if (PTW32_THREAD_REUSE_BOTTOM != t)
    {
      ptw32_threadReuseTop = t->prevReuse;
      t->prevReuse = NULL;
    }
  else
    {
      t = NULL;
    }

  LeaveCriticalSection(&ptw32_thread_reuse_lock);

  return t;

}

/*
 * Push a clean pthread_t struct onto the reuse stack.
 */
void
ptw32_threadReusePush (pthread_t thread)
{
  EnterCriticalSection(&ptw32_thread_reuse_lock);

  memset(thread, 0, sizeof (*thread));
  thread->prevReuse = ptw32_threadReuseTop;
  ptw32_threadReuseTop = thread;

  LeaveCriticalSection(&ptw32_thread_reuse_lock);
}

