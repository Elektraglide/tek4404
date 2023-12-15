/*
 * UniFLEX "controlled Task" interface
 */

/*
 * Create a controlled [sub]task
 *
 * This routine creates a new task which is marked
 * as "controlled" by the operating system.  The parent
 * task of a controlled task may perform certain operations
 * relative to the controlled task while that task runs.
 * These operations include:
 *   Create        - Create a "controlled" task and give
 *                   the parent access to it.  This process
 *                   is analagous to the "fork" operation
 *                   except that whenever a controlled task
 *                   performs an "exec" system call, the task
 *                   is halted and the parent task notified.
 *                   The controlled task may only then be
 *                   resumed (via the single-step or execute
 *                   options).
 *   Single-step   - Allow the controlled task to execute a
 *                   single 68xxx machine instruction.  System
 *                   calls (using TRAP) are treated as a single
 *                   instruction.
 *   Execute       - Allow the controlled task to execute until
 *                   it terminates, has a signal sent to it, or
 *                   executes a breakpoint instruction.  The OS
 *                   interprets the "illegal" instruction ($4AFC)
 *                   as the breakpoint instruction.
 *   Kill          - Terminate the controlled task.
 *   Halt          - Halt the execution of the controlled task.
 *                   This will take place "soon", but not a any
 *                   pre-determined time.
 *   Resume        - Continue execution of a task which has been "halted".
 *   Clear Signals - Remove any pending signals for the task.
 *   Examine/Change- The parent task has full access to the controlled
 *                   task's memory space, including its registers.
 *                   These may be modified as well, giving the
 *                   parent task complete control over the child/
 *                   controlled task.
 *
 *  Each of these functions is encapsulated in a "C" routine which
 *  in turn will request the action by the operating system.  All
 *  of the routines require a pointer to a "controlled task" structure
 *  which contains all the information about the task.  The structure
 *  is created by the "create_controlled_task" function.  More than
 *  one task could thus be controlled by a single task.
 *
 */

#ifndef ctask_h
#define ctask_h

struct ctask {
  short task_id;  /* Task ID of controlled task */
  long  task_fd;  /* File descriptor used to access task image */
  int   task_state;  /* Described below */
  short task_SR;  /* Image of the task's registers follow */
  long  task_PC;
  long  task_REGS[16];  /* D0..D7, A0..A7 */
  long  task_flags;  /* Used by internal routines */
  short task_control; /* Also used internally */
};

#define CTASK_INIT  0x00000001

#define CTASK_HALT    0   /* Halt task at next execution */
#define CTASK_RESUME  1   /* Resume task (must be halted) */
#define CTASK_STEP    2   /* Single step task */
#define CTASK_EXECUTE 3   /* Execute task until termination or breakpoint */
#define CTASK_CREATE  4   /* Create controlled sub-task image */
#define CTASK_CLEAR   5   /* Clear any signals waiting for the task */

/*
 * The "task_state" long is four bytes:
 *   Byte 0 - Lowest numbered signal pending for task (0=none)
 *   Byte 1 - Task priority (informative only)
 *   Byte 2 - Not used
 *   Byte 3 - Task state - one of the TASK_XXX values below.
 */

#define TASK_RUNNING    0x01
#define TASK_STOPPED    0x02
#define TASK_TERMINATED 0x05

/* Routine definitions */
extern struct ctask *create_controlled_task();
extern               step_controlled_task();
extern               kill_controlled_task();
extern               halt_controlled_task();
extern               resume_controlled_task();
extern               execute_controlled_task();
extern               clear_controlled_task_signals();
extern               get_controlled_task_registers();
extern               update_controlled_task_registers();
extern               get_controlled_task_memory();
extern               update_controlled_task_memory();
#endif
