/*
 * private.c
 *
 * Description:
 * This translation unit implements routines which are private to
 * the implementation and may be used throughout it.
 *
 * Pthreads-win32 - POSIX Threads Library for Win32
 * Copyright (C) 1998
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

#ifndef _UWIN
#   include <process.h>
#endif
#ifndef NEED_FTIME
#include <sys/timeb.h>
#endif
#include "pthread.h"
#include "semaphore.h"
#include "implement.h"


int
ptw32_processInitialize (void)
     /*
      * ------------------------------------------------------
      * DOCPRIVATE
      *      This function performs process wide initialization for
      *      the pthread library.
      *
      * PARAMETERS
      *      N/A
      *
      * DESCRIPTION
      *      This function performs process wide initialization for
      *      the pthread library.
      *      If successful, this routine sets the global variable
      *      ptw32_processInitialized to TRUE.
      *
      * RESULTS
      *              TRUE    if successful,
      *              FALSE   otherwise
      *
      * ------------------------------------------------------
      */
{
	if (ptw32_processInitialized) {
		/* 
		 * ignore if already initialized. this is useful for 
		 * programs that uses a non-dll pthread
		 * library. such programs must call ptw32_processInitialize() explicitely,
		 * since this initialization routine is automatically called only when
		 * the dll is loaded.
		 */
		return TRUE;
	}

  ptw32_processInitialized = TRUE;

  /*
   * Initialize Keys
   */
  if ((pthread_key_create (&ptw32_selfThreadKey, NULL) != 0) ||
      (pthread_key_create (&ptw32_cleanupKey, NULL) != 0))
    {

      ptw32_processTerminate ();
    }

  /* 
   * Set up the global test and init check locks.
   */
  InitializeCriticalSection(&ptw32_mutex_test_init_lock);
  InitializeCriticalSection(&ptw32_cond_test_init_lock);
  InitializeCriticalSection(&ptw32_rwlock_test_init_lock);
  InitializeCriticalSection(&ptw32_spinlock_test_init_lock);

  return (ptw32_processInitialized);

}				/* processInitialize */

void
ptw32_processTerminate (void)
     /*
      * ------------------------------------------------------
      * DOCPRIVATE
      *      This function performs process wide termination for
      *      the pthread library.
      *
      * PARAMETERS
      *      N/A
      *
      * DESCRIPTION
      *      This function performs process wide termination for
      *      the pthread library.
      *      This routine sets the global variable
      *      ptw32_processInitialized to FALSE
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  if (ptw32_processInitialized)
    {

      if (ptw32_selfThreadKey != NULL)
	{
	  /*
	   * Release ptw32_selfThreadKey
	   */
	  pthread_key_delete (ptw32_selfThreadKey);

	  ptw32_selfThreadKey = NULL;
	}

      if (ptw32_cleanupKey != NULL)
	{
	  /*
	   * Release ptw32_cleanupKey
	   */
	  pthread_key_delete (ptw32_cleanupKey);

	  ptw32_cleanupKey = NULL;
	}

      /* 
       * Destroy the global test and init check locks.
       */
      DeleteCriticalSection(&ptw32_spinlock_test_init_lock);
      DeleteCriticalSection(&ptw32_rwlock_test_init_lock);
      DeleteCriticalSection(&ptw32_cond_test_init_lock);
      DeleteCriticalSection(&ptw32_mutex_test_init_lock);

      ptw32_processInitialized = FALSE;
    }

}				/* processTerminate */

#ifdef __CLEANUP_SEH

static DWORD
ExceptionFilter (EXCEPTION_POINTERS * ep, DWORD * ei)
{
  switch (ep->ExceptionRecord->ExceptionCode)
    {
      case EXCEPTION_PTW32_SERVICES:
        {
          DWORD param;
          DWORD numParams = ep->ExceptionRecord->NumberParameters;

          numParams = (numParams > 3) ? 3 : numParams;

          for (param = 0; param < numParams; param++)
            {
              ei[param] = ep->ExceptionRecord->ExceptionInformation[param];
            }

          return EXCEPTION_EXECUTE_HANDLER;
          break;
        }
      default:
        {
          /*
           * A system unexpected exception has occurred running the user's
           * routine. We need to cleanup before letting the exception
           * out of thread scope.
           */
          pthread_t self = pthread_self();

          (void) pthread_mutex_destroy(&self->cancelLock);
          ptw32_callUserDestroyRoutines(self);

          return EXCEPTION_CONTINUE_SEARCH;
          break;
        }
    }
}

#elif defined(__CLEANUP_CXX)

#if defined(_MSC_VER)
#include <eh.h>
static terminate_function ptw32_oldTerminate;
#else
#include <new.h>
static terminate_handler ptw32_oldTerminate;
#endif

#if 0
#include <stdio.h>
static pthread_mutex_t termLock = PTHREAD_MUTEX_INITIALIZER;
#endif

void
ptw32_terminate ()
{
  pthread_t self = pthread_self();
#if 0
  FILE * fp;
  pthread_mutex_lock(&termLock);
  fp = fopen("pthread.log", "a");
  fprintf(fp, "Terminate\n");
  fclose(fp);
  pthread_mutex_unlock(&termLock);
#endif
  set_terminate(ptw32_oldTerminate);
  (void) pthread_mutex_destroy(&self->cancelLock);
  ptw32_callUserDestroyRoutines(self);
  terminate();
}

#endif /* _MSC_VER */

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
unsigned __stdcall
#else
void
#endif
ptw32_threadStart (void * vthreadParms)
{
  ThreadParms *threadParms = (ThreadParms *) vthreadParms;
  pthread_t self;
  void *(*start) (void *);
  void *arg;

#ifdef __CLEANUP_SEH
  DWORD ei[] = {0,0,0};
#endif

#ifdef __CLEANUP_C
  int setjmp_rc;
#endif

  void * status = (void *) 0;

  self = threadParms->tid;
  start = threadParms->start;
  arg = threadParms->arg;

  free (threadParms);

#if defined (__MINGW32__) && ! defined (__MSVCRT__)
  /*
   * beginthread does not return the thread id and is running
   * before it returns us the thread handle, and so we do it here.
   */
  self->thread = GetCurrentThreadId ();
  if (pthread_mutex_lock(&self->cancelLock) == 0)
    {
      /* 
       * We got the lock which means that our creator has
       * our thread handle. Unlock and continue on.
       */
      (void) pthread_mutex_unlock(&self->cancelLock);
    }
#endif

  pthread_setspecific (ptw32_selfThreadKey, self);

#ifdef __CLEANUP_SEH

  __try
  {
    /*
     * Run the caller's routine;
     */
    status = self->exitStatus = (*start) (arg);
#ifdef _UWIN
	if (--pthread_count <= 0)
		exit(0);
#endif

  }
  __except (ExceptionFilter(GetExceptionInformation(), ei))
  {
     switch (ei[0])
       {
        case PTW32_EPS_CANCEL:
          status = PTHREAD_CANCELED;
#ifdef _UWIN
		if (--pthread_count <= 0)
			exit(0);
#endif
          break;
        case PTW32_EPS_EXIT:
          status = self->exitStatus;
          break;
        default:
          status = PTHREAD_CANCELED;
          break;
       }
  }

#else /* __CLEANUP_SEH */

#ifdef __CLEANUP_C

  setjmp_rc = setjmp( self->start_mark );

  if( 0 == setjmp_rc ) {

	  /*
	   * Run the caller's routine;
	   */
	  status = self->exitStatus = (*start) (arg);
  }

  else {

     switch (setjmp_rc)
       {
        case PTW32_EPS_CANCEL:
          status = PTHREAD_CANCELED;
          break;
        case PTW32_EPS_EXIT:
          status = self->exitStatus;
          break;
        default:
          status = PTHREAD_CANCELED;
          break;
       }
  }

#else /* __CLEANUP_C */

#ifdef __CLEANUP_CXX

  ptw32_oldTerminate = set_terminate(&ptw32_terminate);

  try
  {
    /*
     * Run the caller's routine in a nested try block so that we
     * can run the user's terminate function, which may call
     * pthread_exit() or be canceled.
     */
    try
      {
        status = self->exitStatus = (*start) (arg);
      }
    catch (ptw32_exception &)
      {
        /*
         * Pass these through to the outer block.
         */
        throw;
      }
    catch(...)
     {
       /*
        * We want to run the user's terminate function if supplied.
        * That function may call pthread_exit() or be canceled, which will
        * be handled by the outer try block.
        * 
        * ptw32_terminate() will be called if there is no user supplied function.
        */

       //Original invocation:
       //(void) terminate();


       //New invocation:
       //  a) get pointer to the termination function
#if defined(_MSC_VER)
       terminate_function term_func = set_terminate(0);
#else
       terminate_handler term_func = set_terminate(0);
#endif

       set_terminate(term_func);

       //  b) call the termination function (if any)
       if (term_func != 0) {
           term_func();
       }

       //  c) if there was no termination function or the termination function did
       //     not exit thread/process, (we got this far), propagate the exception on!
       //     (should be caught by the second level try/catch block below)
       throw;
     }
  }
  catch (ptw32_exception_cancel &)
    {
      /*
       * Thread was cancelled.
       */
      status = self->exitStatus = PTHREAD_CANCELED;
    }
  catch (ptw32_exception_exit &)
    {
      /*
       * Thread was exited via pthread_exit().
       */
      status = self->exitStatus;
    }
  catch (...)
    {
      /*
       * A system unexpected exception has occurred running the user's
       * terminate routine. We get control back within this block - cleanup
       * and release the exception out of thread scope.
       */
      status = self->exitStatus = PTHREAD_CANCELED;
      (void) pthread_mutex_destroy(&self->cancelLock);
      (void) set_terminate(ptw32_oldTerminate);
      ptw32_callUserDestroyRoutines(self);
      throw;

      /*
       * Never reached.
       */
    }

  (void) set_terminate(ptw32_oldTerminate);

#else

#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.

#endif /* __CLEANUP_CXX */
#endif /* __CLEANUP_C */
#endif /* __CLEANUP_SEH */


  (void) pthread_mutex_destroy(&self->cancelLock);

#if 1
  if (self->detachState == PTHREAD_CREATE_DETACHED)
    {
      /*
       * We need to cleanup the pthread now in case we have
       * been statically linked, in which case the cleanup
       * in dllMain won't get done. Joinable threads will
       * be cleaned up by pthread_join().
       *
       * Note that implicitly created pthreads (those created
       * for Win32 threads which have called pthreads routines)
       * must be cleaned up explicitly by the application 
       * (by calling pthread_win32_thread_detach_np()) if
       * this library has been statically linked. For the dll,
       * dllMain will do the cleanup automatically.
       */
      (void) pthread_win32_thread_detach_np ();
    }
  else
    {
      ptw32_callUserDestroyRoutines (self);
    }
#else
  ptw32_callUserDestroyRoutines (self);
#endif

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
  _endthreadex ((unsigned) status);
#else
  _endthread ();
#endif

  /*
   * Never reached.
   */

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
  return (unsigned) status;
#endif

}				/* ptw32_threadStart */

void
ptw32_threadDestroy (pthread_t thread)
{
  if (thread != NULL)
    {
      ptw32_callUserDestroyRoutines (thread);

      if (thread->cancelEvent != NULL)
	{
	  CloseHandle (thread->cancelEvent);
	}

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
      /* See documentation for endthread vs endthreadex. */
      if( thread->threadH != 0 )
	{
	  CloseHandle( thread->threadH );
	}
#endif

      free (thread);
    }

}				/* ptw32_threadDestroy */

int
ptw32_tkAssocCreate (ThreadKeyAssoc ** assocP,
			pthread_t thread,
			pthread_key_t key)
     /*
      * -------------------------------------------------------------------
      * This routine creates an association that
      * is unique for the given (thread,key) combination.The association 
      * is referenced by both the thread and the key.
      * This association allows us to determine what keys the
      * current thread references and what threads a given key
      * references.
      * See the detailed description
      * at the beginning of this file for further details.
      *
      * Notes:
      *      1)      New associations are pushed to the beginning of the
      *              chain so that the internal ptw32_selfThreadKey association
      *              is always last, thus allowing selfThreadExit to
      *              be implicitly called by pthread_exit last.
      *
      * Parameters:
      *              assocP
      *                      address into which the association is returned.
      *              thread
      *                      current running thread. If NULL, then association
      *                      is only added to the key. A NULL thread indicates
      *                      that the user called pthread_setspecific prior
      *                      to starting a thread. That's ok.
      *              key
      *                      key on which to create an association.
      * Returns:
      *       0              - if successful,
      *       ENOMEM         - not enough memory to create assoc or other object
      *       EINVAL	     - an internal error occurred
      *       ENOSYS	     - an internal error occurred
      * -------------------------------------------------------------------
      */
{
  int result;
  ThreadKeyAssoc *assoc;

  /*
   * Have to create an association and add it
   * to both the key and the thread.
   */
  assoc = (ThreadKeyAssoc *) calloc (1, sizeof (*assoc));

  if (assoc == NULL)
    {
      result = ENOMEM;
      goto FAIL0;
    }

  /*
   * Initialise only when used for the first time.
   */
  assoc->lock = PTHREAD_MUTEX_INITIALIZER;
  assoc->thread = thread;
  assoc->key = key;

  /*
   * Register assoc with key
   */
  if ((result = pthread_mutex_lock (&(key->threadsLock))) != 0)
    {
      goto FAIL2;
    }

  assoc->nextThread = (ThreadKeyAssoc *) key->threads;
  key->threads = (void *) assoc;

  pthread_mutex_unlock (&(key->threadsLock));

  if (thread != NULL)
    {
      /*
       * Register assoc with thread
       */
      assoc->nextKey = (ThreadKeyAssoc *) thread->keys;
      thread->keys = (void *) assoc;
    }

  *assocP = assoc;

  return (result);

  /*
   * -------------
   * Failure Code
   * -------------
   */
FAIL2:
  pthread_mutex_destroy (&(assoc->lock));
  free (assoc);

FAIL0:

  return (result);

}				/* ptw32_tkAssocCreate */


void
ptw32_tkAssocDestroy (ThreadKeyAssoc * assoc)
     /*
      * -------------------------------------------------------------------
      * This routine releases all resources for the given ThreadKeyAssoc
      * once it is no longer being referenced
      * ie) both the key and thread have stopped referencing it.
      *
      * Parameters:
      *              assoc
      *                      an instance of ThreadKeyAssoc.
      * Returns:
      *      N/A
      * -------------------------------------------------------------------
      */
{

  if ((assoc != NULL) &&
      (assoc->key == NULL && assoc->thread == NULL))
    {

      pthread_mutex_destroy (&(assoc->lock));

      free (assoc);
    }

}				/* ptw32_tkAssocDestroy */


void
ptw32_callUserDestroyRoutines (pthread_t thread)
     /*
      * -------------------------------------------------------------------
      * DOCPRIVATE
      *
      * This the routine runs through all thread keys and calls
      * the destroy routines on the user's data for the current thread.
      * It simulates the behaviour of POSIX Threads.
      *
      * PARAMETERS
      *              thread
      *                      an instance of pthread_t
      *
      * RETURNS
      *              N/A
      * -------------------------------------------------------------------
      */
{
  ThreadKeyAssoc **nextP;
  ThreadKeyAssoc *assoc;

  if (thread != NULL)
    {
      /*
       * Run through all Thread<-->Key associations
       * for the current thread.
       * If the pthread_key_t still exits (ie the assoc->key
       * is not NULL) then call the user's TSD destroy routine.
       * Notes:
       *      If assoc->key is NULL, then the user previously called
       *      PThreadKeyDestroy. The association is now only referenced
       *      by the current thread and must be released; otherwise
       *      the assoc will be destroyed when the key is destroyed.
       */
      nextP = (ThreadKeyAssoc **) & (thread->keys);
      assoc = *nextP;

      while (assoc != NULL)
	{

	  if (pthread_mutex_lock (&(assoc->lock)) == 0)
	    {
	      pthread_key_t k;
	      if ((k = assoc->key) != NULL)
		{
		  /*
		   * Key still active; pthread_key_delete
		   * will block on this same mutex before
		   * it can release actual key; therefore,
		   * key is valid and we can call the destroy
		   * routine;
		   */
		  void *value = NULL;

		  value = pthread_getspecific (k);
		  if (value != NULL && k->destructor != NULL)
		    {

#ifdef __cplusplus

		      try
		      {
			/*
			 * Run the caller's cleanup routine.
			 */
			(*(k->destructor)) (value);
		      }
		      catch (...)
		      {
			/*
			 * A system unexpected exception has occurred
			 * running the user's destructor.
			 * We get control back within this block in case
			 * the application has set up it's own terminate
			 * handler. Since we are leaving the thread we
			 * should not get any internal pthreads
			 * exceptions.
			 */
			terminate();
		      }

#else  /* __cplusplus */

			/*
			 * Run the caller's cleanup routine.
			 */
			(*(k->destructor)) (value);

#endif /* __cplusplus */
		    }
		}

	      /*
	       * mark assoc->thread as NULL to indicate the
	       * thread no longer references this association
	       */
	      assoc->thread = NULL;

	      /*
	       * Remove association from the pthread_t chain
	       */
	      *nextP = assoc->nextKey;

	      pthread_mutex_unlock (&(assoc->lock));

	      ptw32_tkAssocDestroy (assoc);

	      assoc = *nextP;
	    }
	}
    }

}				/* ptw32_callUserDestroyRoutines */



#ifdef NEED_FTIME

/*
 * time between jan 1, 1601 and jan 1, 1970 in units of 100 nanoseconds
 */
#define TIMESPEC_TO_FILETIME_OFFSET \
          ( ((LONGLONG) 27111902 << 32) + (LONGLONG) 3577643008 )

static void
timespec_to_filetime(const struct timespec *ts, FILETIME *ft)
     /*
      * -------------------------------------------------------------------
      * converts struct timespec
      * where the time is expressed in seconds and nanoseconds from Jan 1, 1970.
      * into FILETIME (as set by GetSystemTimeAsFileTime), where the time is
      * expressed in 100 nanoseconds from Jan 1, 1601,
      * -------------------------------------------------------------------
      */
{
	*(LONGLONG *)ft = ts->tv_sec * 10000000 + (ts->tv_nsec + 50) / 100 + TIMESPEC_TO_FILETIME_OFFSET;
}

static void
filetime_to_timespec(const FILETIME *ft, struct timespec *ts)
     /*
      * -------------------------------------------------------------------
      * converts FILETIME (as set by GetSystemTimeAsFileTime), where the time is
      * expressed in 100 nanoseconds from Jan 1, 1601,
      * into struct timespec
      * where the time is expressed in seconds and nanoseconds from Jan 1, 1970.
      * -------------------------------------------------------------------
      */
{
	ts->tv_sec = (int)((*(LONGLONG *)ft - TIMESPEC_TO_FILETIME_OFFSET) / 10000000);
	ts->tv_nsec = (int)((*(LONGLONG *)ft - TIMESPEC_TO_FILETIME_OFFSET - ((LONGLONG)ts->tv_sec * (LONGLONG)10000000)) * 100);
}

#endif /* NEED_FTIME */

int
ptw32_sem_timedwait (sem_t * sem, const struct timespec * abstime)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a semaphore possibly until
      *      'abstime' time.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *      abstime
      *              pointer to an instance of struct timespec
      *
      * DESCRIPTION
      *      This function waits on a semaphore. If the
      *      semaphore value is greater than zero, it decreases
      *      its value by one. If the semaphore value is zero, then
      *      the calling thread (or process) is blocked until it can
      *      successfully decrease the value or until interrupted by
      *      a signal.
      *
      *      If 'abstime' is a NULL pointer then this function will
      *      block until it can successfully decrease the value or
      *      until interrupted by a signal.
      *
      * RESULTS
      *              0               successfully decreased semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EINTR           the function was interrupted by a signal,
      *              EDEADLK         a deadlock condition was detected.
      *              ETIMEDOUT       abstime elapsed before success.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

#ifdef NEED_FTIME

  struct timespec currSysTime;

#else /* NEED_FTIME */

  struct _timeb currSysTime;

#endif /* NEED_FTIME */

  const DWORD NANOSEC_PER_MILLISEC = 1000000;
  const DWORD MILLISEC_PER_SEC = 1000;
  DWORD milliseconds;

  if (sem == NULL)
    {
      result = EINVAL;
    }
  else
    {
      if (abstime == NULL)
	{
	  milliseconds = INFINITE;
	}
      else
	{
	  /* 
	   * Calculate timeout as milliseconds from current system time. 
	   */

	  /* get current system time */

#ifdef NEED_FTIME

	  {
	    FILETIME ft;
	    SYSTEMTIME st;

	    GetSystemTime(&st);
            SystemTimeToFileTime(&st, &ft);
	    /*
             * GetSystemTimeAsFileTime(&ft); would be faster,
             * but it does not exist on WinCE
             */

	    filetime_to_timespec(&ft, &currSysTime);
	  }

	  /*
           * subtract current system time from abstime
           */
	  milliseconds = (abstime->tv_sec - currSysTime.tv_sec) * MILLISEC_PER_SEC;
	  milliseconds += ((abstime->tv_nsec - currSysTime.tv_nsec) + (NANOSEC_PER_MILLISEC/2)) / NANOSEC_PER_MILLISEC;

#else /* NEED_FTIME */
	  _ftime(&currSysTime);

	  /*
           * subtract current system time from abstime
           */
	  milliseconds = (abstime->tv_sec - currSysTime.time) * MILLISEC_PER_SEC;
	  milliseconds += ((abstime->tv_nsec + (NANOSEC_PER_MILLISEC/2)) / NANOSEC_PER_MILLISEC) -
	    currSysTime.millitm;

#endif /* NEED_FTIME */


	  if (((int) milliseconds) < 0)
	    milliseconds = 0;
	}

#ifdef NEED_SEM

      result = (pthreadCancelableTimedWait ((*sem)->event, milliseconds));

#else /* NEED_SEM */

      result = (pthreadCancelableTimedWait ((*sem)->sem, milliseconds));

#endif

    }

  if (result != 0)
    {

      errno = result;
      return -1;

    }

#ifdef NEED_SEM

  ptw32_decrease_semaphore(sem);

#endif /* NEED_SEM */

  return 0;

}				/* ptw32_sem_timedwait */


DWORD
ptw32_get_exception_services_code(void)
{
#ifdef __CLEANUP_SEH

  return EXCEPTION_PTW32_SERVICES;

#else

  return (DWORD) NULL;

#endif
}


void
ptw32_throw(DWORD exception)
{
#ifdef __CLEANUP_C
  pthread_t self = pthread_self();
#endif


#ifdef __CLEANUP_SEH
  DWORD exceptionInformation[3];
#endif

  if (exception != PTW32_EPS_CANCEL &&
      exception != PTW32_EPS_EXIT)
    {
      /* Should never enter here */
      exit(1);
    }

#ifdef __CLEANUP_SEH


  exceptionInformation[0] = (DWORD) (exception);
  exceptionInformation[1] = (DWORD) (0);
  exceptionInformation[2] = (DWORD) (0);

  RaiseException (
		  EXCEPTION_PTW32_SERVICES,
		  0,
		  3,
		  exceptionInformation);

#else /* __CLEANUP_SEH */

#ifdef __CLEANUP_C

  ptw32_pop_cleanup_all( 1 );

  longjmp( self->start_mark, exception );

#else /* __CLEANUP_C */

#ifdef __CLEANUP_CXX

  switch (exception)
    {
    case PTW32_EPS_CANCEL:
      throw ptw32_exception_cancel();
      break;
    case PTW32_EPS_EXIT:
      throw ptw32_exception_exit();
      break;
    }

#else

#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.

#endif /* __CLEANUP_CXX */

#endif /* __CLEANUP_C */

#endif /* __CLEANUP_SEH */

  /* Never reached */
}

void
ptw32_pop_cleanup_all(int execute)
{
	while( NULL != ptw32_pop_cleanup(execute) ) {
	}
}
