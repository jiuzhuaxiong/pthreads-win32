/* 
 * spin2.c
 *
 *
 * Pthreads-win32 - POSIX Threads Library for Win32
 * Copyright (C) 1998 Ben Elliston and Ross Johnson
 * Copyright (C) 1999,2000,2001 Ross Johnson
 *
 * Contact Email: rpj@ise.canberra.edu.au
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * --------------------------------------------------------------------------
 *
 * Declare a spinlock object, lock it, trylock it, 
 * and then unlock it again.
 *
 */

#include "test.h"
 
pthread_spinlock_t lock = NULL;

static int washere = 0;

void * func(void * arg)
{
  assert(pthread_spin_trylock(&lock) == EBUSY);

  washere = 1;

  return 0; 
}
 
int
main()
{
  pthread_t t;

  assert(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE) == 0);

  assert(pthread_spin_lock(&lock) == 0);

  assert(pthread_create(&t, NULL, func, NULL) == 0);
  assert(pthread_join(t, NULL) == 0);

  assert(pthread_spin_unlock(&lock) == 0);

  assert(pthread_spin_destroy(&lock) == 0);

  assert(washere == 1);

  return 0;
}
