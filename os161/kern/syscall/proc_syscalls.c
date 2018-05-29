#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <sync.h>


  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {
  //kprintf("sysexit running\n");
  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  

  lock_acquire(p->proc_lock);
  p->exitstatus = _MKWAIT_EXIT(exitcode);
  lock_release(p->proc_lock);

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}



//fork

int
sys_fork(pid_t *retval , struct trapframe *passed_tf)
{
  //kprintf("sys_fork running\n");

  //follow sys_exit example
  struct proc *c_p;
    //kprintf("Starting child proc\n");
  struct addrspace *child_as = NULL;
    //kprintf("Setting up addrspace\n");
  c_p = proc_create_runprogram("newnastychild"); //god bless griffin for telling me proc_create_program exists

  
  // COPY SECTION
    // copy our address space
    KASSERT(child_as ==NULL); //This should be null, if it's not = bad things
    as_copy(curproc->p_addrspace, &child_as); // copy it over from curproc
    (c_p->p_addrspace = child_as);
  // make a copy of the trap frame variables
    struct trapframe* ctf; // how can you not use ctf as the name
    ctf = kmalloc(sizeof(struct trapframe)); //malloc our trapframe
    *ctf = *passed_tf; // assign our passed trap frame
  // END COPY SECTION


  //memory edit our trap frame
    int x;
    // threadfork(name, function poitner entry point for new, pass1 pass2)
    x = thread_fork(curthread->t_name, c_p,uproc_thread,ctf, *retval);
    //kprintf("Parent returning after thread fork\n");
   // stall until I implement wait-pid

  *retval = (c_p->p_pid);
  //kprintf("Parent finally leaving sys_fork\n");
  return 0; //test
}

void 
uproc_thread(void* temp_tf, unsigned long testvar){
  (void)testvar;
  struct trapframe cur_trapframe = *((struct trapframe*)temp_tf); 
  kfree(temp_tf);
  //child = 0
  // +4 to move past syscall
  cur_trapframe.tf_v0 = 0; 
  cur_trapframe.tf_epc += 4;
  //kprintf("**SCREAMING CHILD NOISES**"); //my annoying child is running
  mips_usermode(&cur_trapframe);
  thread_exit();
}



/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = (curproc->p_pid);

  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }

  struct proc *p = proctable[pid];
  
  lock_aquire(p->proc_lock);

  while (p->zombie != 1) {
      cv_wait(p->proc_cv,p->proc_lock);
      }
  lock_release(p->proc_lock);
  exitstatus = p->exitstatus;

  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

