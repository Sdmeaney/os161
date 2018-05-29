/*
 * Copyright (c) 2013
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
 * Process support.
 *
 * There is (intentionally) not much here; you will need to add stuff
 * and maybe change around what's already present.
 *
 * p_lock is intended to be held when manipulating the pointers in the
 * proc structure, not while doing any significant work with the
 * things they point to. Rearrange this (and/or change it to be a
 * regular lock) as needed.
 *
 * Unless you're implementing multithreaded user processes, the only
 * process that will have more than one thread is the kernel process.
 */

#include <types.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <vfs.h>
#include <synch.h>
#include <kern/fcntl.h>  

/*
 * The process for the kernel; this holds all the kernel-only threads.
 */
struct proc *kproc;
pid_t prosid = 2;

// defining proccess table
static struct proc ** proctable; // ** pointer to a pointer
								 // static is so it's not dynamically allocated and so we can reference

/*
 * Mechanism for making the kernel menu thread sleep while processes are running
 */
#ifdef UW
/* count of the number of processes, excluding kproc */
static unsigned int proc_count;
/* provides mutual exclusion for proc_count */
/* it would be better to use a lock here, but we use a semaphore because locks are not implemented in the base kernel */ 
static struct semaphore *proc_count_mutex;
static struct lock *proc_table_mutex;
/* used to signal the kernel menu thread when there are no processes */
struct semaphore *no_proc_sem;   
#endif  // UW
int MAXARRAY = 256;
void proctable_create(void){
 proc_table_mutex = lock_create("proc_table_mutex");
 proctable = kmalloc (sizeof(struct proc *)*MAXARRAY);
 	for ( int i= 0; i < MAXARRAY; ++i){
		proctable[i] = NULL;
	}
	//kprintf("proctable_create\n");
}

void proctable_add(struct proc* p){
	if (proc_table_mutex == NULL){
		p->p_pid = 1;
		p->parent = NULL;
		return;
	}
	lock_acquire(proc_table_mutex);
	p->zombie = 0;
	for ( int i= 2; i < MAXARRAY; ++i){

	if (proctable[i] == NULL){
	//ADD IT

		p->p_pid = i;
		p->parent = curproc;
		proctable[i] = p;
		//kprintf("proctable_added\n");
		lock_release(proc_table_mutex);
		return;
		}
	}

	// WE'VE LOOPED, NO NULLS
	lock_release(proc_table_mutex);
	proctable_resize();
	proctable_add(p);
	return;
}

void proctable_resize(void){
	//kprintf("proc_resize\n");
	lock_acquire(proc_table_mutex);
	//make the size bigger
	MAXARRAY = MAXARRAY*2;
	struct proc** newproctable = kmalloc (sizeof(struct proc *)*MAXARRAY);
	//null out all the values
 	for ( int i= 0; i < MAXARRAY; ++i){
		newproctable[i] = NULL;
	}
	//copy over the values
	for ( int i= 0; i < MAXARRAY/2; i++ ){
		newproctable[i] = proctable[i];
	}

	proctable = newproctable;
	lock_release(proc_table_mutex);
}

void proctable_remove(struct proc* p){
		kprintf("proc_remove\n");
		lock_acquire(proc_table_mutex);
		// look through the table to find if it has kids
		for ( int i= 0; i < MAXARRAY; i++ ){
			if(proctable[i]!= NULL;){
				if(proctable[i]->p_pid == p->p_pid) {
					proctable[i] = NULL;
					lock_release(proc_table_mutex);
				}
			}
		}
}


void proc_terminator(struct proc *proc){
	kprintf("XXXX$$$XXXX");
			KASSERT(proc != NULL);
			KASSERT(proc != kproc);
			proctable_remove(proc);
			/*
			 * We don't take p_lock in here because we must have the only
			 * reference to this structure. (Otherwise it would be
			 * incorrect to destroy it.)
			 */

			/* VFS fields */
			if (proc->p_cwd) {
				VOP_DECREF(proc->p_cwd);
				proc->p_cwd = NULL;
			}


		#ifndef UW  // in the UW version, space destruction occurs in sys_exit, not here
			if (proc->p_addrspace) {
				/*
				 * In case p is the currently running process (which
				 * it might be in some circumstances, or if this code
				 * gets moved into exit as suggested above), clear
				 * p_addrspace before calling as_destroy. Otherwise if
				 * as_destroy sleeps (which is quite possible) when we
				 * come back we'll be calling as_activate on a
				 * half-destroyed address space. This tends to be
				 * messily fatal.
				 */
				struct addrspace *as;

				as_deactivate();
				as = curproc_setas(NULL);
				as_destroy(as);
			}
		#endif // UW

		#ifdef UW
			if (proc->console) {
			  vfs_close(proc->console);
			}
		#endif // UW



//	lock_destroy(proc->proc_lock);
//	cv_destroy(proc->proc_cv);
//	kfree(proc->p_name);
//	threadarray_cleanup(&proc->p_threads);
//	spinlock_cleanup(&proc->p_lock);
	kfree(proc);

	#ifdef UW
	/* decrement the process count */
        /* note: kproc is not included in the process count, but proc_destroy
	   is never called on kproc (see KASSERT above), so we're OK to decrement
	   the proc_count unconditionally here */
	P(proc_count_mutex); 
	KASSERT(proc_count > 0);
	proc_count--;
	/* signal the kernel menu thread if the process count has reached zero */
	if (proc_count == 0) {
	  V(no_proc_sem);
	}
	V(proc_count_mutex);
#endif // UW
}

/*
 * Create a proc structure.
 */
static
struct proc *
proc_create(const char *name)
{
	struct proc *proc;

	proc = kmalloc(sizeof(*proc));
	if (proc == NULL) {
		return NULL;
	}
	proc->p_name = kstrdup(name);
	if (proc->p_name == NULL) {
		kfree(proc);
		return NULL;
	}

	threadarray_init(&proc->p_threads);
	spinlock_init(&proc->p_lock);

	/* VM fields */
	proc->p_addrspace = NULL;

	/* VFS fields */
	proc->p_cwd = NULL;

	/* PID field */
	//proc->p_pid = prosid;
	prosid += 1;

	proc->proc_cv = cv_create("proc_cv"); // added cv
	proc->proc_lock = lock_create("proc_lock");


#ifdef UW
	proc->console = NULL;
#endif // UW
	if (kproc != NULL){
		proctable_add(proc);
	}
	return proc;
}

struct proc * proc_number(pid_t pid){
	lock_acquire(proc_table_mutex);
	struct proc *proc_number_var =  proctable[pid];
	lock_release(proc_table_mutex);
	return proc_number_var;
}
/*
 * Destroy a proc structure.
 */
void
proc_destroy(struct proc *proc)
{
	kprintf("OOOOOOOOOOOOOOOOOO");
	KASSERT(proc != NULL);
	KASSERT(proc != kproc);

// set all child parent pids to null
		lock_acquire(proc_table_mutex);
			for ( int i= 0; i < MAXARRAY; i++ ){
			kprintf("@");

			if(proctable[i] != NULL){			
				if(proctable[i]->parent->p_pid == proc->p_pid) {
					struct proc *cur_child = proctable[i];
					lock_acquire(cur_child->proc_lock);
					cur_child->parent = NULL; //if we have a child set their pids to null	
					lock_release(cur_child->proc_lock);
					// if child is zombie, call pid_remove
					if(cur_child->zombie == 1){
						proctable_remove(cur_child); // delete it from the table
						proc_terminator(cur_child); // erase it from existance

					}	

				}
			}
		}
		lock_release(proc_table_mutex);

kprintf("CCCCCCCCCCCCCCCC");

	// do I have any parent?
	if(proc->parent != NULL){		//yes, have a parent	
		lock_acquire(proc->proc_lock);
		proc->zombie = 1;
		cv_broadcast(proc->proc_cv, proc->proc_lock);
		lock_release(proc->proc_lock);
	}
	else{		//no, I have no parent
		proctable_remove(proc); // delete it from the table
		proc_terminator(proc); // erase it from existance
	}

}

/*
 * Create the process structure for the kernel.
 */
void
proc_bootstrap(void)
{
  proctable_create();
  kproc = proc_create("[kernel]");
  if (kproc == NULL) {
    panic("proc_create for kproc failed\n");
  }
#ifdef UW
  proc_count = 0;
  proc_count_mutex = sem_create("proc_count_mutex",1);
  if (proc_count_mutex == NULL) {
    panic("could not create proc_count_mutex semaphore\n");
  }
  no_proc_sem = sem_create("no_proc_sem",0);
  if (no_proc_sem == NULL) {
    panic("could not create no_proc_sem semaphore\n");
  }
#endif // UW 
}

/*
 * Create a fresh proc for use by runprogram.
 *
 * It will have no address space and will inherit the current
 * process's (that is, the kernel menu's) current directory.
 */
struct proc *
proc_create_runprogram(const char *name)
{
	struct proc *proc;
	char *console_path;

	proc = proc_create(name);
	if (proc == NULL) {
		return NULL;
	}

#ifdef UW
	/* open the console - this should always succeed */
	console_path = kstrdup("con:");
	if (console_path == NULL) {
	  panic("unable to copy console path name during process creation\n");
	}
	if (vfs_open(console_path,O_WRONLY,0,&(proc->console))) {
	  panic("unable to open the console during process creation\n");
	}
	kfree(console_path);
#endif // UW
	  
	/* VM fields */

	proc->p_addrspace = NULL;

	/* VFS fields */

#ifdef UW
	/* we do not need to acquire the p_lock here, the running thread should
           have the only reference to this process */
        /* also, acquiring the p_lock is problematic because VOP_INCREF may block */
	if (curproc->p_cwd != NULL) {
		VOP_INCREF(curproc->p_cwd);
		proc->p_cwd = curproc->p_cwd;
	}
#else // UW
	spinlock_acquire(&curproc->p_lock);
	if (curproc->p_cwd != NULL) {
		VOP_INCREF(curproc->p_cwd);
		proc->p_cwd = curproc->p_cwd;
	}
	spinlock_release(&curproc->p_lock);
#endif // UW

#ifdef UW
	/* increment the count of processes */
        /* we are assuming that all procs, including those created by fork(),
           are created using a call to proc_create_runprogram  */
	P(proc_count_mutex); 
	proc_count++;
	V(proc_count_mutex);
#endif // UW

	return proc;
}

/*
 * Add a thread to a process. Either the thread or the process might
 * or might not be current.
 */
int
proc_addthread(struct proc *proc, struct thread *t)
{
	int result;

	KASSERT(t->t_proc == NULL);

	spinlock_acquire(&proc->p_lock);
	result = threadarray_add(&proc->p_threads, t, NULL);
	spinlock_release(&proc->p_lock);
	if (result) {
		return result;
	}
	t->t_proc = proc;
	return 0;
}

/*
 * Remove a thread from its process. Either the thread or the process
 * might or might not be current.
 */
void
proc_remthread(struct thread *t)
{
	struct proc *proc;
	unsigned i, num;

	proc = t->t_proc;
	KASSERT(proc != NULL);

	spinlock_acquire(&proc->p_lock);
	/* ugh: find the thread in the array */
	num = threadarray_num(&proc->p_threads);
	for (i=0; i<num; i++) {
		if (threadarray_get(&proc->p_threads, i) == t) {
			threadarray_remove(&proc->p_threads, i);
			spinlock_release(&proc->p_lock);
			t->t_proc = NULL;
			return;
		}
	}
	/* Did not find it. */
	spinlock_release(&proc->p_lock);
	panic("Thread (%p) has escaped from its process (%p)\n", t, proc);
}

/*
 * Fetch the address space of the current process. Caution: it isn't
 * refcounted. If you implement multithreaded processes, make sure to
 * set up a refcount scheme or some other method to make this safe.
 */
struct addrspace *
curproc_getas(void)
{
	struct addrspace *as;
#ifdef UW
        /* Until user processes are created, threads used in testing 
         * (i.e., kernel threads) have no process or address space.
         */
	if (curproc == NULL) {
		return NULL;
	}
#endif

	spinlock_acquire(&curproc->p_lock);
	as = curproc->p_addrspace;
	spinlock_release(&curproc->p_lock);
	return as;
}

/*
 * Change the address space of the current process, and return the old
 * one.
 */
struct addrspace *
curproc_setas(struct addrspace *newas)
{
	struct addrspace *oldas;
	struct proc *proc = curproc;

	spinlock_acquire(&proc->p_lock);
	oldas = proc->p_addrspace;
	proc->p_addrspace = newas;
	spinlock_release(&proc->p_lock);
	return oldas;
}
