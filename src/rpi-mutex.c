#include "rpi-mutex.h"
#include "rpi-thread.h"

#include <malloc.h>


//
// Mutex definition
//
struct mutex_t
{
	thread_id_t		owner;		// Owning thread
	uint32_t		count;		// Owning thread recursive lock count
	uint32_t		waits;		// Number of threads waiting for the mutex
};



//
// Create a mutex
//
mutex_t* mutex_create()
{
	mutex_t* mutex = (mutex_t*)malloc(sizeof(mutex_t));
	mutex->owner = 0;
	mutex->count = 0;
	return mutex;
}



//
// Destroy a mutex
//
void mutex_destroy(mutex_t* mutex)
{
	ASSERT(mutex->owner == 0);
	ASSERT(mutex->count == 0);

	free(mutex);
}



//
// Try to lock a mutex. Implementation, not declared in header. Used by thread scheduler.
//
uint32_t mutex_trylock_thread(mutex_t* mutex, thread_id_t thread_id)
{
	ASSERT(mutex->waits > 0);

	if (mutex->owner == 0)
	{
		ASSERT(mutex->count == 0);

		// The thread is now owner of the mutex
		mutex->owner = thread_id;

		// Update counters
		mutex->count++;
		mutex->waits--;

		// Mutex locked successfully
		return 1;
	}
	else if (mutex->owner == thread_id)
	{
		ASSERT(mutex->count > 0);

		// Update counters
		mutex->count++;
		mutex->waits--;

		// Mutex locked successfully
		return 1;
	}
	else
	{
		// Mutex lock failed
		return 0;
	}
}



//
// Try to lock a mutex
//
uint32_t mutex_trylock(mutex_t* mutex)
{
	// Add to wait count
	mutex->waits++;

	// If the lock succeeds, the wait count will have been removed
	if (mutex_trylock_thread(mutex, thread_current()))
		return 1;

	// Remove from wait count
	mutex->waits--;
}



//
// Lock a mutex
//
void mutex_lock(mutex_t* mutex)
{
	// Try to lock the mutex directly
	if (!mutex_trylock(mutex))
	{
		// Add this thread to the wait count
		mutex->waits++;

		// Yield the thread until the mutex is free
		thread_wait_mutex(mutex);
	}
}



//
// Unlock a mutex
//
void mutex_unlock(mutex_t* mutex)
{
	ASSERT(mutex->owner == thread_current());
	ASSERT(mutex->count > 0);

	// Update count
	if (--mutex->count == 0)
		mutex->owner = 0;
}