/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, int initial_count)
{
        struct semaphore *sem;

        KASSERT(initial_count >= 0);

        sem = kmalloc(sizeof(struct semaphore));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void 
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 * Bridge to the wchan lock, so if someone else comes
		 * along in V right this instant the wakeup can't go
		 * through on the wchan until we've finished going to
		 * sleep. Note that wchan_sleep unlocks the wchan.
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_lock(sem->sem_wchan);
		spinlock_release(&sem->sem_lock);
                wchan_sleep(sem->sem_wchan);

		spinlock_acquire(&sem->sem_lock);
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
        struct lock *lock;



        lock = kmalloc(sizeof(struct lock));
        if (lock == NULL) {
                return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }
        
        // add stuff here as needed
        // taking from our semaphore example above but making it "lock"
	//things to do:
	//1. initialize our lock status to nullc
	//2. create our wait channel
    //3. start our spinlock
	//
    //set currently locked status to null (since we just made it )
    // this is a struct *thread
    lock->lock_locked = NULL;
	//wchan is wait channel
	lock -> lk_wchan = wchan_create(lock -> lk_name);
	//if it goes wrong, delete the evidence and return
    if(lock -> lk_wchan == NULL)
	{
	  kfree(lock -> lk_name);
  	  kfree(lock);
	  return NULL;
	}
    //  spinlock_init(&sem->sem_lock);
    spinlock_init(&lock->lk_lock);

	//end custom
        return lock;
}

void
lock_destroy(struct lock *lock)
{       // if something goes horribly wrong
        KASSERT(lock != NULL);

        // add stuff here as needed

    /* wchan_cleanup will assert if anyone's waiting on it */
        spinlock_cleanup(&lock->lk_lock);
        wchan_destroy(lock->lk_wchan);
            //lets not do this before we set lock_locked to null
            //kfree(lock->lk_name);
            //kfree(lock);

         //lock_locked is struct * thread    
        lock->lock_locked = NULL;

        //end added stuff here
        
        kfree(lock->lk_name);
        kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
        // Write this
        //int release_flag;

        // Operations:
        // lock_acquire - Get the lock. Only one thread can hold the lock at the
        // same time.
        //spinlock_acquire(&sem->sem_lock);

        //priorities:
        //1 spinlocks around lock_locked so it can't get tampered with
        //2 can't path to a sleep while holding the spinlock and freeze everything
        //3 1-to-1 for spinlock aq/rel
        //4 don't aquire same spinlock more than once

       while(true){ //infinite loop it until we just break on spinlock release
       spinlock_acquire(&lock->lk_lock); //protect the compare for the if statement
       if(lock->lock_locked == NULL) //if it's empty
       {
        //lock our thread in        
        lock->lock_locked = curthread; // god bless curthread
        //releasing our spinlock section
        spinlock_release(&lock->lk_lock);
        break; //removed flag val for break
       }
       //so we don't have something not be null, then sleep with the spinlock
       //this won't be touched by if null twice because it breaks71
        spinlock_release(&lock->lk_lock);

        // it's not your turn so sleep
        wchan_sleep(lock->lk_wchan);
        }

        //end write this

       // (void)lock;  // suppress warning until code gets written
}

void
lock_release(struct lock *lock)
{
        // Write this
        //lock_release - Free the lock. Only the thread holding the lock may do
        //  this.
 //   KASSERT(lock->lock_locked == curthread);
    KASSERT(lock != NULL);

  //  if(lock_do_i_hold(lock)){ //give our lock to lock do I hold to see if it's curthread
        spinlock_acquire(&lock->lk_lock); //protect since we have to mess with 
        wchan_wakeall(lock->lk_wchan); // let them all fight for the next slot
        //lock->lock_locked = curthread; this is how we aquired
        // just like in aquire, we release 
        lock->lock_locked = NULL;
        //KASSERT(lock == NULL);

        spinlock_release(&lock->lk_lock); //end spinlock protect since we're done
           // (void)lock;  // suppress warning until code gets written
   // }

}

bool
lock_do_i_hold(struct lock *lock)
{
        // Write this

       // (void)lock;  // suppress warning until code gets written
        if(lock->lock_locked == curthread){ // if am the curthread return true
            return true;
        }
        else{ //if I'm not the cur thread ret false
            return false;
        }
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(struct cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }
        
        // add stuff here as needed
        
        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed
        
        kfree(cv->cv_name);
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        // Write this
        (void)cv;    // suppress warning until code gets written
        (void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
        // Write this
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
}
