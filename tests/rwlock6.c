/*
 * rwlock6.c
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
 * Check that writer locks have priority.
 *
 * Depends on API functions: 
 *      pthread_rwlock_rdlock()
 *      pthread_rwlock_wrlock()
 *      pthread_rwlock_unlock()
 */

#include "test.h"

static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;

static int bankAccount = 0;

void * wrfunc(void * arg)
{
  assert(pthread_rwlock_wrlock(&rwlock1) == 0);
  Sleep(2000);
  bankAccount += 10;
  assert(pthread_rwlock_unlock(&rwlock1) == 0);

  return ((void *) bankAccount);
}

void * rdfunc(void * arg)
{
  int ba = 0;

  assert(pthread_rwlock_rdlock(&rwlock1) == 0);
  ba = bankAccount;
  assert(pthread_rwlock_unlock(&rwlock1) == 0);

  ba += 10;
  return ((void *) ba);
}

int
main()
{
  pthread_t wrt1;
  pthread_t wrt2;
  pthread_t rdt;
  int wr1Result = 0;
  int wr2Result = 0;
  int rdResult = 0;

  bankAccount = 0;

  assert(pthread_create(&wrt1, NULL, wrfunc, NULL) == 0);
  Sleep(500);
  assert(pthread_create(&wrt2, NULL, rdfunc, NULL) == 0);
  Sleep(500);
  assert(pthread_create(&rdt, NULL, wrfunc, NULL) == 0);

  assert(pthread_join(wrt1, (void **) &wr1Result) == 0);
  assert(pthread_join(wrt2, (void **) &wr2Result) == 0);
  assert(pthread_join(rdt, (void **) &rdResult) == 0);

  assert(wr1Result == 10);
  assert(wr2Result == 20);
  assert(rdResult == 20);

  return 0;
}
