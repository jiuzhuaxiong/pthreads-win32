/* This is the POSIX thread API (POSIX 1003).
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

#if !defined( PTHREAD_H )
#define PTHREAD_H

/*
 * -------------------------------------------------------------
 *
 *
 * Module: pthread.h
 *
 * Purpose:
 *      Provides an implementation of PThreads based upon the
 *      standard:
 *
 *              POSIX 1003.1c-1995      (POSIX.1c)
 *
 * Authors:
 *      Contributors are listed in the file "CONTRIBUTORS".
 *
 * -------------------------------------------------------------
 */

#ifdef _MSC_VER
/*
 * Disable following warnings when including Windows headers
 *
 * warning C4115: named type definition in parentheses
 * warning C4116: unnamed type definition in parentheses
 * warning C4127: conditional expression is constant
 * warning C4201: nonstandard extension used : nameless struct/union
 * warning C4214: nonstandard extension used : bit field types other than int
 * warning C4514: unreferenced inline function has been removed
 */
#pragma warning( disable : 4115 4116 4127 4201 4214 4514)
#endif

/*
 * -----------------
 * autoconf switches
 * -----------------
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <windows.h>

#ifndef NEED_FTIME
#include <time.h>
#else /* NEED_FTIME */
/* use native WIN32 time API */
#endif /* NEED_FTIME */

#if HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#include <malloc.h>

#ifndef HAVE_STRUCT_TIMESPEC
struct timespec {
	long tv_sec;
	long tv_nsec;
};
#endif /* HAVE_STRUCT_TIMESPEC */

#ifndef SIG_BLOCK
#define SIG_BLOCK 0
#endif /* SIG_BLOCK */

#ifndef SIG_UNBLOCK 
#define SIG_UNBLOCK 1
#endif /* SIG_UNBLOCK */

#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif /* SIG_SETMASK */

#include <process.h>

/*
 * note: ETIMEDOUT is correctly defined in winsock.h
 */
#include <winsock.h>

#ifdef NEED_ERRNO
#include "need_errno.h"
#else
#include <errno.h>
#endif

/*
 * In case ETIMEDOUT hasn't been defined above somehow.
 */
#ifndef ETIMEDOUT
#define ETIMEDOUT 10060     /* This is the value in winsock.h. */
#endif

/*
 * If ENOTSUP is not defined, define it to a value that will never occur.
 * This is the value used in the Win32 TCL port.
 */
#ifndef ENOTSUP
#define ENOTSUP  -1030507
#endif

#ifdef __MINGW32__
#define PT_STDCALL
#else
#ifdef __cplusplus
#define PT_STDCALL __stdcall
#else
#define PT_STDCALL __stdcall
#endif
#endif

#ifdef _MSC_VER
/*
 * Re-enable all but 4127, 4514
 */
#pragma warning( default : 4115 4116 4201 4214)
#endif

#if !defined( TRUE )
#define TRUE	!FALSE
#define FALSE	0
#endif /* !TRUE */

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/*
 * -------------------------------------------------------------
 *
 * POSIX 1003.1c-1995 Options
 * ===========================
 *
 * _POSIX_THREADS (set)
 *                      If set, you can use threads
 *
 * _POSIX_THREAD_ATTR_STACKSIZE (set)
 *                      If set, you can control the size of a thread's
 *                      stack
 *                              pthread_attr_getstacksize
 *                              pthread_attr_setstacksize
 *
 * _POSIX_THREAD_ATTR_STACKADDR (not set)
 *                      If set, you can allocate and control a thread's
 *                      stack. If not supported, the following functions
 *                      will return ENOSYS, indicating they are not
 *                      supported:
 *                              pthread_attr_getstackaddr
 *                              pthread_attr_setstackaddr
 *
 * _POSIX_THREAD_PRIORITY_SCHEDULING (not set)
 *                      If set, you can use realtime scheduling.
 *                      Indicates the availability of:
 *                              pthread_attr_getinheritsched
 *                              pthread_attr_getschedparam
 *                              pthread_attr_getschedpolicy
 *                              pthread_attr_getscope
 *                              pthread_attr_setinheritsched
 *                              pthread_attr_setschedparam
 *                              pthread_attr_setschedpolicy
 *                              pthread_attr_setscope
 *                              pthread_getschedparam
 *                              pthread_setschedparam
 *                              pthread_get_priority_max
 *                              pthread_get_priority_min
 *
 * _POSIX_THREAD_PRIO_INHERIT (not set)
 *                      If set, you can create priority inheritance
 *                      mutexes.
 *                              pthread_mutexattr_getprotocol +
 *                              pthread_mutexattr_setprotocol +
 *
 * _POSIX_THREAD_PRIO_PROTECT (not set)
 *                      If set, you can create priority ceiling mutexes
 *                      Indicates the availability of:
 *                              pthread_mutex_getprioceiling
 *                              pthread_mutex_setprioceiling
 *                              pthread_mutexattr_getprioceiling
 *                              pthread_mutexattr_getprotocol     +
 *                              pthread_mutexattr_setprioceiling
 *                              pthread_mutexattr_setprotocol     +
 *
 * _POSIX_THREAD_PROCESS_SHARED (not set)
 *                      If set, you can create mutexes and condition
 *                      variables that can be shared with another
 *                      process.If set, indicates the availability
 *                      of:
 *                              pthread_mutexattr_getpshared
 *                              pthread_mutexattr_setpshared
 *                              pthread_condattr_getpshared
 *                              pthread_condattr_setpshared
 *
 * _POSIX_THREAD_SAFE_FUNCTIONS (set)
 *                      If set you can use the special *_r library
 *                      functions that provide thread-safe behaviour
 *
 *      + These functions provide both 'inherit' and/or
 *        'protect' protocol, based upon these macro
 *        settings.
 *
 * POSIX 1003.1c-1995 Limits
 * ===========================
 *
 * PTHREAD_DESTRUCTOR_ITERATIONS
 *                      Maximum number of attempts to destroy
 *                      a thread's thread-specific data on
 *                      termination (must be at least 4)
 *
 * PTHREAD_KEYS_MAX
 *                      Maximum number of thread-specific data keys
 *                      available per process (must be at least 128)
 *
 * PTHREAD_STACK_MIN
 *                      Minimum supported stack size for a thread
 *
 * PTHREAD_THREADS_MAX
 *                      Maximum number of threads supported per
 *                      process (must be at least 64).
 *
 * -------------------------------------------------------------
 */

/*
 * POSIX Options
 */
#ifndef _POSIX_THREADS
#define _POSIX_THREADS
#endif
#define _POSIX_THREAD_SAFE_FUNCTIONS

#define _POSIX_THREAD_ATTR_STACKSIZE

#if defined( KLUDGE )
/*
 * The following are not supported
 */
#define _POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_PRIORITY_SCHEDULING
#define _POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PROCESS_SHARED

#endif				/* KLUDGE */

/*
 * POSIX Limits
 *
 *      PTHREAD_DESTRUCTOR_ITERATIONS
 *              Standard states this must be at least
 *              4.
 *
 *      PTHREAD_KEYS_MAX
 *              WIN32 permits only 64 TLS keys per process.
 *              This limitation could be worked around by
 *              simply simulating keys.
 *
 *      PTHREADS_STACK_MIN
 *              POSIX specifies 0 which is also the value WIN32
 *              interprets as allowing the system to
 *              set the size to that of the main thread. The
 *              maximum stack size in Win32 is 1Meg. WIN32
 *              allocates more stack as required up to the 1Meg
 *              limit.
 *
 *      PTHREAD_THREADS_MAX
 *              Not documented by WIN32. Wrote a test program
 *              that kept creating threads until it failed
 *              revealed this approximate number.
 *
 */
#define PTHREAD_DESTRUCTOR_ITERATIONS	                   4
#define PTHREAD_KEYS_MAX				  64
#define PTHREAD_STACK_MIN				   0
#define PTHREAD_THREADS_MAX				2019


typedef struct pthread_t_ *pthread_t;
typedef struct pthread_attr_t_ *pthread_attr_t;
typedef struct pthread_once_t_ pthread_once_t;
typedef struct pthread_key_t_ *pthread_key_t;
typedef struct pthread_mutex_t_ *pthread_mutex_t;
typedef struct pthread_mutexattr_t_ *pthread_mutexattr_t;
typedef struct pthread_cond_t_ *pthread_cond_t;
typedef struct pthread_condattr_t_ *pthread_condattr_t;
typedef struct pthread_rwlock_t_ *pthread_rwlock_t;
typedef struct pthread_rwlockattr_t_ *pthread_rwlockattr_t;


/*
 * ====================
 * ====================
 * POSIX Threads
 * ====================
 * ====================
 */

/*
 * pthread_attr_{get,set}detachstate
 */
#define PTHREAD_CREATE_JOINABLE		0
#define PTHREAD_CREATE_DETACHED		1

/*
 * pthread_attr{get,set}inheritsched
 */
#define PTHREAD_INHERIT_SCHED		0
#define PTHREAD_EXPLICIT_SCHED		1

/*
 * pthread_setcancelstate paramters
 */
#define PTHREAD_CANCEL_ENABLE		0
#define PTHREAD_CANCEL_DISABLE		1

/*
 * pthread_setcanceltype parameters
 */
#define PTHREAD_CANCEL_ASYNCHRONOUS	0
#define PTHREAD_CANCEL_DEFERRED		1

/*
 * pthread_mutexattr_{get,set}pshared
 * pthread_condattr_{get,set}pshared
 */
#define PTHREAD_PROCESS_PRIVATE		0
#define PTHREAD_PROCESS_SHARED		1

/*
 * pthread_mutexattr_(get,set}type
 */
#define PTHREAD_MUTEX_DEFAULT           0
#define PTHREAD_MUTEX_NORMAL            1
#define PTHREAD_MUTEX_ERRORCHECK        2
#define PTHREAD_MUTEX_RECURSIVE         3

/*
 * pthread_attr_(get,set}scope
 *
 * PTHREAD_SCOPE_SYSTEM is currently the only scope supported.
 */
#define PTHREAD_SCOPE_SYSTEM            0
#define PTHREAD_SCOPE_PROCESS           1

/*
 * ====================
 * ====================
 * Cancelation
 * ====================
 * ====================
 */
#define PTHREAD_CANCELED       ((void *) -1)


/*
 * ====================
 * ====================
 * Once Key
 * ====================
 * ====================
 */
#define PTHREAD_ONCE_INIT       { FALSE, -1 }

struct pthread_once_t_
{
  int done;                 /* indicates if user function executed  */
  long started;             /* First thread to increment this value */
                            /* to zero executes the user function   */
};


#define PTHREAD_MUTEX_INITIALIZER ((pthread_mutex_t) -1)

#define PTHREAD_COND_INITIALIZER ((pthread_cond_t) -1)

#define PTHREAD_RWLOCK_INITIALIZER ((pthread_rwlock_t) -1)


/*
 * ====================
 * ====================
 * Scheduling
 * ====================
 * ====================
 */

/* Thread scheduling policies */

#define SCHED_OTHER 0
#define SCHED_FIFO  1
#define SCHED_RR    2

#define SCHED_MIN   SCHED_OTHER
#define SCHED_MAX   SCHED_RR

struct sched_param {
  int sched_priority;
};


/*
 * ====================
 * ====================
 * Cancelation cleanup
 * ====================
 * ====================
 */

/* There are three implementations of cancel cleanup.
 * Note that pthread.h is included in both application
 * compilation units and also internally for the library.
 * The code here and within the library aims to work
 * for all reasonable combinations of environments.
 * For example, although the library itself can't be
 * built (yet) in C, an application written in C can
 * be linked and run against a library built using
 * either WIN32 SEH or C++ EH.
 *
 * The three implementations are:
 *
 *   WIN32 SEH
 *   C
 *   C++
 *
 * Please note that exiting a push/pop block via
 * "return", "exit", "break", or "continue" will
 * lead to different behaviour amongst applications
 * depending upon whether the library was built
 * using SEH, C++, or C. For example, a library built
 * with SEH will call the cleanup routine, while both
 * C++ and C built versions will not.
 */

typedef struct ptw32_cleanup_t ptw32_cleanup_t;

struct ptw32_cleanup_t
{
  void (*routine) (void *);
  void *arg;
#if !defined(_MSC_VER) && !defined(__cplusplus)
  ptw32_leanup_t *prev;
#endif /* !_MSC_VER && ! __cplusplus */
};

#if defined(_MSC_VER) && !defined(__cplusplus)
	/*
	 * WIN32 SEH version of cancel cleanup.
	 */

#define pthread_cleanup_push( _rout, _arg ) \
	{ \
	    ptw32_cleanup_t	_cleanup; \
	    \
            _cleanup.routine	= (_rout); \
	    _cleanup.arg	= (_arg); \
	    __try \
	      { \

#define pthread_cleanup_pop( _execute ) \
	      } \
	    __finally \
		{ \
		    if( _execute || AbnormalTermination()) \
		      { \
			  (*(_cleanup.routine))( _cleanup.arg ); \
		      } \
		} \
	}

#else /* _MSC_VER && ! __cplusplus */

#ifndef __cplusplus

	/*
	 * C implementation of PThreads cancel cleanup
	 */

#define pthread_cleanup_push( _rout, _arg ) \
	{ \
	    ptw32_cleanup_t	_cleanup; \
            \
	    pthread_push_cleanup( &_cleanup, (_rout), (_arg) ); \

#define pthread_cleanup_pop( _execute ) \
	    (void) pthread_pop_cleanup( _execute ); \
	}

#else /* !__cplusplus */

	/*
	 * C++ version of cancel cleanup.
	 * - John E. Bossom.
	 */

	class PThreadCleanup {
	  /*
	   * PThreadCleanup
	   *
	   * Purpose
	   *      This class is a C++ helper class that is
	   *      used to implement pthread_cleanup_push/
	   *      pthread_cleanup_pop.
	   *      The destructor of this class automatically
	   *      pops the pushed cleanup routine regardless
	   *      of how the code exits the scope
	   *      (i.e. such as by an exception)
	   */
	  void            (PT_STDCALL *cleanUpRout)( void * );
	  void    *       obj;
	  int             executeIt;

	public:
	  PThreadCleanup() :
	    cleanUpRout( NULL ),
	    obj( NULL ),
	    executeIt( 0 )
	    /*
	     * No cleanup performed
	     */
	    {
	    }

	  PThreadCleanup(
			 void            (PT_STDCALL *routine)( void * ),
			 void    *       arg ) :
	    cleanUpRout( routine ),
	    obj( arg ),
	    executeIt( 1 )
	    /*
             * Registers a cleanup routine for 'arg'
             */
	    {
	    }

	  ~PThreadCleanup()
	    {
	      if ( executeIt && ((void *) cleanUpRout != NULL) )
		{
                  (void) (*cleanUpRout)( obj );
		}
	    }

	  void execute( int exec )
	    {
	      executeIt = exec;
	    }
	};

	/*
	 * C++ implementation of PThreads cancel cleanup;
	 * This implementation takes advantage of a helper
	 * class who's destructor automatically calls the
	 * cleanup routine if we exit our scope weirdly
	 */
#define pthread_cleanup_push( _rout, _arg ) \
        { \
	    PThreadCleanup  cleanup((void (PT_STDCALL *)(void *))(_rout), \
				    (void *) (_arg) );

#define pthread_cleanup_pop( _execute ) \
	    cleanup.execute( _execute ); \
	}

#endif /* !__cplusplus) */

#endif /* _MSC_VER && ! __cplusplus */

/*
 * ===============
 * ===============
 * Methods
 * ===============
 * ===============
 */

/*
 * PThread Attribute Functions
 */
int pthread_attr_init (pthread_attr_t * attr);

int pthread_attr_destroy (pthread_attr_t * attr);

int pthread_attr_getdetachstate (const pthread_attr_t * attr,
				 int *detachstate);

int pthread_attr_getstackaddr (const pthread_attr_t * attr,
			       void **stackaddr);

int pthread_attr_getstacksize (const pthread_attr_t * attr,
			       size_t * stacksize);

int pthread_attr_setdetachstate (pthread_attr_t * attr,
				 int detachstate);

int pthread_attr_setstackaddr (pthread_attr_t * attr,
			       void *stackaddr);

int pthread_attr_setstacksize (pthread_attr_t * attr,
			       size_t stacksize);

int pthread_attr_getschedparam (const pthread_attr_t *attr,
				struct sched_param *param);

int pthread_attr_setschedparam (pthread_attr_t *attr,
				const struct sched_param *param);

int pthread_attr_setscope (pthread_attr_t *,
                       int);

int pthread_attr_getscope (const pthread_attr_t *,
                       int *);

/*
 * PThread Functions
 */
int pthread_create (pthread_t * tid,
		    const pthread_attr_t * attr,
		    void *(*start) (void *),
		    void *arg);

int pthread_detach (pthread_t tid);

int pthread_equal (pthread_t t1,
		   pthread_t t2);

void pthread_exit (void *value_ptr);

int pthread_join (pthread_t thread,
		  void **value_ptr);

pthread_t pthread_self (void);

int pthread_cancel (pthread_t thread);

int pthread_setcancelstate (int state,
			    int *oldstate);

int pthread_setcanceltype (int type,
			   int *oldtype);

void pthread_testcancel (void);

int pthread_once (pthread_once_t * once_control,
		  void (*init_routine) (void));

ptw32_cleanup_t *pthread_pop_cleanup (int execute);

void pthread_push_cleanup (ptw32_cleanup_t * cleanup,
			   void (*routine) (void *),
			   void *arg);

/*
 * Thread Specific Data Functions
 */
int pthread_key_create (pthread_key_t * key,
			void (*destructor) (void *));

int pthread_key_delete (pthread_key_t key);

int pthread_setspecific (pthread_key_t key,
			 const void *value);

void *pthread_getspecific (pthread_key_t key);


/*
 * Mutex Attribute Functions
 */
int pthread_mutexattr_init (pthread_mutexattr_t * attr);

int pthread_mutexattr_destroy (pthread_mutexattr_t * attr);

int pthread_mutexattr_getpshared (const pthread_mutexattr_t
				  * attr,
				  int *pshared);

int pthread_mutexattr_setpshared (pthread_mutexattr_t * attr,
				  int pshared);

int pthread_mutexattr_settype (pthread_mutexattr_t * attr,
					 int type);

int pthread_mutexattr_gettype (pthread_mutexattr_t * attr,
					 int * type);

/*
 * Mutex Functions
 */
int pthread_mutex_init (pthread_mutex_t * mutex,
			const pthread_mutexattr_t * attr);

int pthread_mutex_destroy (pthread_mutex_t * mutex);

int pthread_mutex_lock (pthread_mutex_t * mutex);

int pthread_mutex_trylock (pthread_mutex_t * mutex);

int pthread_mutex_unlock (pthread_mutex_t * mutex);

/*
 * Condition Variable Attribute Functions
 */
int pthread_condattr_init (pthread_condattr_t * attr);

int pthread_condattr_destroy (pthread_condattr_t * attr);

int pthread_condattr_getpshared (const pthread_condattr_t * attr,
				 int *pshared);

int pthread_condattr_setpshared (pthread_condattr_t * attr,
				 int pshared);

/*
 * Condition Variable Functions
 */
int pthread_cond_init (pthread_cond_t * cond,
		       const pthread_condattr_t * attr);

int pthread_cond_destroy (pthread_cond_t * cond);

int pthread_cond_wait (pthread_cond_t * cond,
		       pthread_mutex_t * mutex);

int pthread_cond_timedwait (pthread_cond_t * cond,
			    pthread_mutex_t * mutex,
			    const struct timespec *abstime);

int pthread_cond_signal (pthread_cond_t * cond);

int pthread_cond_broadcast (pthread_cond_t * cond);

/*
 * Scheduling
 */
int pthread_setschedparam (pthread_t thread,
			   int policy,
			   const struct sched_param *param);

int pthread_getschedparam (pthread_t thread,
			   int *policy,
			   struct sched_param *param);

int pthread_setconcurrency (int);

int pthread_getconcurrency (void);

/*
 * Read-Write Lock Functions
 */
int pthread_rwlock_init(pthread_rwlock_t *lock,
                        const pthread_rwlockattr_t *attr);

int pthread_rwlock_destroy(pthread_rwlock_t *lock);

int pthread_rwlock_tryrdlock(pthread_rwlock_t *);

int pthread_rwlock_trywrlock(pthread_rwlock_t *);

int pthread_rwlock_rdlock(pthread_rwlock_t *lock);

int pthread_rwlock_wrlock(pthread_rwlock_t *lock);

int pthread_rwlock_unlock(pthread_rwlock_t *lock);


/*
 * Non-portable functions
 */

/* Possibly supported by other POSIX threads implimentations */
int pthread_delay_np (struct timespec * interval);

/*
 * Returns the Win32 thread HANDLE associated
 * with the given POSIX thread.
 */
HANDLE pthread_getw32threadhandle_np(pthread_t thread);

/*
 * Useful if an application wants to statically link
 * the lib rather than load the DLL at run-time.
 */
int pthread_win32_process_attach_np(void);
int pthread_win32_process_detach_np(void);
int pthread_win32_thread_attach_np(void);
int pthread_win32_thread_detach_np(void);


/*
 * Protected Methods
 *
 * This function blocks until the given WIN32 handle
 * is signaled or pthread_cancel had been called.
 * This function allows the caller to hook into the
 * PThreads cancel mechanism. It is implemented using
 *
 *              WaitForMultipleObjects
 *
 * on 'waitHandle' and a manually reset WIN32 Event
 * used to implement pthread_cancel. The 'timeout'
 * argument to TimedWait is simply passed to
 * WaitForMultipleObjects.
 */
int pthreadCancelableWait (HANDLE waitHandle);
int pthreadCancelableTimedWait (HANDLE waitHandle, DWORD timeout);

/*
 * Thread-Safe C Runtime Library Mappings.
 */
#if 1
#if (! defined(NEED_ERRNO)) || (! defined( _REENTRANT ) && (! defined( _MT ) || ! defined( _MD )))
int * _errno( void );
#endif
#else
#if (! defined(NEED_ERRNO)) || (! defined( _REENTRANT ) && (! defined( _MT ) || ! defined( _MD )))
#if defined(PTW32_BUILD)
__declspec( dllexport ) int * _errno( void );
#else
int * _errno( void );
#endif
#endif
#endif

/*
 * WIN32 C runtime library had been made thread-safe
 * without affecting the user interface. Provide
 * mappings from the UNIX thread-safe versions to
 * the standard C runtime library calls.
 * Only provide function mappings for functions that
 * actually exist on WIN32.
 */

#if !defined(__MINGW32__)
#define strtok_r( _s, _sep, _lasts ) \
	( *(_lasts) = strtok( (_s), (_sep) ) )
#endif /* !__MINGW32__ */

#define asctime_r( _tm, _buf ) \
	( strcpy( (_buf), asctime( (_tm) ) ), \
	  (_buf) )

#define ctime_r( _clock, _buf ) \
	( strcpy( (_buf), ctime( (_clock) ) ),  \
          (_buf) )

#define gmtime_r( _clock, _result ) \
	( *(_result) = *gmtime( (_clock) ), \
	  (_result) )

#define localtime_r( _clock, _result ) \
	( *(_result) = *localtime( (_clock) ), \
	  (_result) )

#define rand_r( _seed ) \
	rand()


#ifdef __cplusplus

/*
 * Internal exceptions
 */
class ptw32_exception {};
class ptw32_exception_cancel : public ptw32_exception {};
class ptw32_exception_exit   : public ptw32_exception {};

#endif

/* FIXME: This is only required if the library was built using SEH */
/*
 * Get internal SEH tag
 */
DWORD ptw32_get_exception_services_code(void);

#ifndef PTW32_BUILD

#if defined(_MSC_VER) && !defined(__cplusplus)

/*
 * Redefine the SEH __except keyword to ensure that applications
 * propagate our internal exceptions up to the library's internal handlers.
 */
#define __except( E ) \
        __except( ( GetExceptionCode() == ptw32_get_exception_services_code() ) \
		 ? EXCEPTION_CONTINUE_SEARCH : ( E ) )

#endif /* _MSC_VER && ! __cplusplus */

#ifdef __cplusplus

/*
 * Redefine the C++ catch keyword to ensure that applications
 * propagate our internal exceptions up to the library's internal handlers.
 */
#ifdef _MSC_VER
        /*
         * WARNING: Replace any 'catch( ... )' with 'PtW32CatchAll'
         * if you want Pthread-Win32 cancelation and pthread_exit to work.
         */

#ifndef PtW32NoCatchWarn

#pragma message("When compiling applications with MSVC++ and C++ exception handling:")
#pragma message("  Replace any 'catch( ... )' with 'PtW32CatchAll' in POSIX threads")
#pragma message("  if you want POSIX thread cancelation and pthread_exit to work.")

#endif

#define PtW32CatchAll \
        catch( ptw32_exception & ) { throw; } \
        catch( ... )

#else /* _MSC_VER */

#define catch( E ) \
        catch( ptw32_exception & ) { throw; } \
        catch( E )

#endif /* _MSC_VER */

#endif /* __cplusplus */

#endif /* ! PTW32_BUILD */

#ifdef __cplusplus
}				/* End of extern "C" */
#endif				/* __cplusplus */

#endif /* PTHREAD_H */
