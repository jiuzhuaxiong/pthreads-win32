/* 
 * mutex6.c
 *
 * Test the default (type not set) mutex type.
 * Should be the same as PTHREAD_MUTEX_NORMAL.
 * Thread locks mutex twice (recursive lock).
 * Locking thread should deadlock on second attempt.
 *
 * Depends on API functions: 
 *	pthread_mutex_lock()
 *	pthread_mutex_trylock()
 *	pthread_mutex_unlock()
 */

#include "test.h"

static int lockCount = 0;

static pthread_mutex_t mutex;

void * locker(void * arg)
{
  assert(pthread_mutex_lock(&mutex) == 0);
  lockCount++;
  assert(pthread_mutex_lock(&mutex) != 0);
  lockCount++;
  Sleep(1000);
  assert(pthread_mutex_unlock(&mutex) == 0);
  assert(pthread_mutex_unlock(&mutex) == 0);

  return 0;
}
 
int
main()
{
  pthread_t t;

  assert(pthread_mutex_init(&mutex, NULL) == 0);

  assert(pthread_create(&t, NULL, locker, NULL) == 0);

  Sleep(2000);

  assert(lockCount == 1);

  exit(0);

  /* Never reached */
  return 0;
}
