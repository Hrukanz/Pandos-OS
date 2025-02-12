/*
 * Date: 20/9/2024
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 * System Call Functions
 * 
 * This file contains implementations of various system calls which is
 * called by exceptions.c
 * 
 * SYS1: Create Process
 * - Allocates and initializes a new process control block (PCB).
 * - Adds the new process to the ready queue and makes it a child of the 
 *   current process.
 * - Returns success or failure based on resource availability.
 * 
 * SYS2: Terminate Process
 * - Recursively terminates the current process and its children.
 * - Removes processes from the ready queue and releases any held resources.
 * - Calls the scheduler to handle remaining processes.
 * 
 * SYS3: Passeren (P)
 * - Performs a P (wait) operation on a semaphore.
 * - If the semaphore count is less than zero, the process is blocked 
 *   and moved from the running state to the blocked queue.
 * 
 * SYS4: Verhogen (V)
 * - Performs a V (signal) operation on a semaphore.
 * - If processes are blocked on the semaphore, one is moved to the ready queue.
 * 
 * SYS5: Wait for IO Device
 * - Blocks the current process while waiting for an I/O device.
 * - Handles terminal devices separately for read and write operations.
 * 
 * SYS6: Get CPU Time
 * - Returns the total CPU time used by the current process in microseconds.
 * 
 * SYS7: Wait for Clock
 * - Blocks the current process until the pseudo-clock semaphore signals.
 * 
 * SYS8: Get SUPPORT Data
 * - Returns the pointer to the support structure of the current process.
 * 
 * Helper Functions:
 * - termProcessRecursive(): Recursively terminates a process and its 
 *   descendants, releasing resources and handling blocked processes.
 */



#include <umps3/umps/libumps.h>

#include "../h/const.h"

#include "../h/asl.h"
#include "../h/pcb.h"

#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/syscalls.h"

HIDDEN void termProcRecursive(pcb_t *p);

/* 
 * SYS1: Create Process
 * - Allocate and initialize a new pcb
 * - Handle resource allocation failure
 * - Place new pcb on the Ready Queue
 */
void createProc(state_t * statep, support_t * supportp) {
	pcb_PTR newProc = allocPcb();
	unsigned int retValue = -1;

	if (newProc != NULL) {
		process_count++;
		newProc->p_supportStruct = supportp;
		newProc->p_s = *statep;
		insertChild(current_proc, newProc);
		insertProcQ(&ready_tp, newProc);
		retValue = 0;
	}
	EXCSTATE->s_v0 = retValue;
}

void terminateProc() {
	outChild(current_proc);
	termProcRecursive(current_proc);

	current_proc = NULL;

	scheduler();
}


HIDDEN void termProcRecursive(pcb_t *p) {
	pcb_PTR child;

	while ((child = removeChild(p)) != NULL) {
		outProcQ(&ready_tp, child);
		termProcRecursive(child);
	}


	bool blockedOnDevice =
		(p->p_semAdd >= (int *) device_sems &&
		 p->p_semAdd <
		 ((int *) device_sems +
		  (sizeof(int) * DEVICE_TYPES * DEVICE_INSTANCES)))
		|| (p->p_semAdd == (int *) &IntervalTimerSem);

	pcb_PTR removedPcb = outBlocked(p);

	if (!blockedOnDevice && removedPcb != NULL) {
		(*(p->p_semAdd))++;
	}

	freePcb(p);
	process_count--;
}

/* 
 * SYS3: Passeren (P)
 * - Perform a P operation on a semaphore
 * - Transition the Current Process from running to blocked state
 * - Call the Scheduler
 */
void passeren(int *semAdd) {
	(*semAdd)--;

	if (*semAdd < 0) {
		current_proc->p_s = *EXCSTATE;
		current_proc->p_time += timePassed();
		insertBlocked((int *) semAdd, current_proc);
		current_proc = NULL;
		scheduler();
	}
}


/* 
 * SYS4: Verhogen (V)
 * - Perform a V operation on a semaphore
 */
pcb_PTR verhogen(int *semAdd) {
	(*semAdd)++;

	pcb_PTR unblockedProcess = NULL;

	if (*semAdd <= 0) {

		unblockedProcess = removeBlocked(semAdd);
		if (unblockedProcess != NULL) {
			insertProcQ(&ready_tp, unblockedProcess);
		}
	}
	return unblockedProcess;
}

/* 
 * SYS5: Wait for IO Device
 * - Transition the Current Process from running to blocked state
 * - Call the Scheduler
 * - Handle terminal devices as independent sub-devices
 */
void waitIO(int intLine, int deviceNum, bool waitForTermRead) {
	
	current_proc->p_s = *EXCSTATE;
	soft_block_count++;

	switch (intLine) {
	case DISKINT:
	case FLASHINT:
	case NETWINT:
	case PRNTINT:
		passeren(&device_sems[intLine - DISKINT][deviceNum]);
		break;
	case TERMINT:
		if (waitForTermRead)
			passeren(&device_sems[4][deviceNum]);
		else
			passeren(&device_sems[5][deviceNum]);
		break;
	default:
		terminateProc();
		break;
	}
}


/* 
 * SYS6: Get CPU Time
 * - Return accumulated CPU time in microseconds
 */
void cpuTime(cpu_t *resultAddress) {
	*resultAddress = current_proc->p_time + timePassed();
}

/* 
 * SYS7: Wait for Clock
 * - Perform a P operation on the Pseudo-clock semaphore
 * - Transition the Current Process from running to blocked state
 * - Call the Scheduler
 */
void waitClk() {
	soft_block_count++;
	passeren(&IntervalTimerSem);
}

/* 
 * SYS8: Get SUPPORT Data
 * - Return pointer to the Current Process's Support Structure
 */
void getSupportData(support_t **resultAddress) {
	*resultAddress = current_proc->p_supportStruct;
}