/*
 * sync.c
 *
 * Description:
 * This translation unit implements functions related to thread
 * synchronisation.
 */

/* POSIX STANDARD: A thread may pass a value pointer to some data via
   pthread_exit(). That pointer will be stored in a location supplied
   as an argument to pthread_join().

   IMPLEMENTATION: The value_ptr is stored in the thread entry. When
   pthread_join() wakes up after waiting, or immediately if the target
   thread has already terminated but is not detached, the value
   pointer from pthread_exit() will be copied to *value_ptr.

   If the target thread does not become detached in the mean time, all
   waiting joins on that thread will get the value pointer. The last
   waiting join will delete the target thread entry.

   ----

   POSIX STANDARD: The results of multiple simultaneous calls to
   pthread_join() specifying the same target thread are undefined.

   IMPLEMENTATION: Any such join that occurs before the first such
   join wakes up, or the thread is otherwise detached (by a call to
   pthread_detach), will return successfully with the value that was
   passed to pthread_exit(). After the last such join returns, the
   target thread will have be detached and it's entry removed from the
   thread table.
  
   Until the target thread entry is deleted it will be counted against
   {PTHREAD_COUNT_MAX}.

   ----

   ----

   POSIX STANDARD: It is unspecified whether a thread that has exited
   but remains unjoined counts against {PTHREAD_COUNT_MAX}.

   IMPLEMENTATION: A thread that has exited but remains unjoined will
   be counted against {PTHREAD_COUNT_MAX}. The first call to
   pthread_join() or pthread_detach() will remove the target thread's
   table entry and decrement the count.

   ---- */

#include <windows.h>
#include "pthread.h"
#include "implement.h"

int
pthread_join(pthread_t thread, void ** valueptr)
{
  LPDWORD exitcode;
  int detachstate;
  _pthread_threads_thread_t * target;

  /* First check if we are trying to join to ourselves. */
  if (thread == pthread_self())
    {
      return EDEADLK;
    }

  /* Find the thread. */
  target = _pthread_find_thread_entry(thread);

  if (target != NULL)
    {
      pthread_mutex_t * target_thread_mutex;
      int ret;

      target_thread_mutex = _PTHREAD_THREAD_MUTEX(target);

      /* CRITICAL SECTION */
      pthread_mutex_lock(target_thread_mutex);

      /* If the thread is in DETACHED state, then join will return
	 immediately. */

      if (target->detach == TRUE)
	{
	  return EINVAL;
	}

      target->join_count++;

      pthread_mutex_lock(target_thread_mutex);
      /* END CRITICAL SECTION */

      /* Wait on the kernel thread object. */
      switch (WaitForSingleObject(thread, INFINITE))
	{
	case WAIT_FAILED:
	  /* The thread does not exist. */
	  return ESRCH;
	case WAIT_OBJECT_0:
	  /* The thread has finished. */
	  break;
	default:
	  /* This should never happen. */
	  break;
	}

      /* We know the target thread entry still exists at this point
	 because we incremented join_count above after checking. The
	 thread entry will not be removed until join_count == 0 again,
	 ie. when the last waiting join has passed through the
	 following critical section. */

      /* CRITICAL SECTION */
      pthread_mutex_lock(target_thread_mutex);

      /* Collect the value pointer passed to pthread_exit().  If
	 another thread detaches our target thread while we're
	 waiting, then we report a deadlock as it likely that storage
	 pointed to by target->joinvalueptr has been freed or
	 otherwise no longer valid. */

      if (target->detach == TRUE)
	{
	  ret = EDEADLK;
	}
      else
	{
	  *value_ptr = target->joinvalueptr;
	  ret = 0;
	}

      target->join_count--;

      /* If we're the last join to return then we are responsible for
	 removing the target thread's table entry. */
      if (target->join_count == 0)
	{
	  _pthread_delete_thread_entry(target);
	}

      pthread_mutex_lock(target_thread_mutex);
      /* END CRITICAL SECTION */

      return ret;
    }

  /* Thread not found. */
  return ESRCH;
}

int
pthread_detach(pthread_t thread)
{
  _pthread_threads_thread_t * this;
  int detachstate;

  this = _pthread_find_thread_entry(thread);

  if (this == NULL)
    {
      return ESRCH;
    }

  /* Check that we can detach this thread. */
  if (pthread_attr_getdetachedstate(&(this->attr), &detachstate) != 0 
      || detachstate == PTHREAD_CREATE_DETACHED)
    {
      return EINVAL;
    }

  /* This is all we do here - the rest is done either when the thread
     exits or when pthread_join() exits. */
  this->detach = TRUE;

  return 0;
}
