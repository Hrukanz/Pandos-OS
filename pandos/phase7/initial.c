/*
* Date: 31/7/2024
* Authors: Haruka Yamamoto & James Rushworth
* 
*/

#include "../h/types.h"
#include "../h/const.h"

#include "../h/asl.h"
#include "../h/pcb.h"

#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"

#include "/usr/include/umps3/umps/libumps.h"

/* There are 8 instances of each terminal, printer, ethernet, flash drive, and disk devices */
/* An additional semaphore is required for the psuedo clock. Each terminal requires not one, but two sems */
/* Total of 16 + 4*8 + 1 = 49 semaphores required */

unsigned int process_count; /* Number of started but not yet terminated processes */
unsigned int soft_block_count; /* Number of blocked proccesses */
pcb_PTR ready_tp; /* Tail pointer to a queue of ready pcbs */
pcb_PTR current_proc; /* Currently running process */

int device_sems[DEVICE_TYPES][DEVICE_INSTANCES];
int IntervalTimerSem;

extern void test();
extern void uTLB_RefillHandler();

int main(void) {
	/*Populate the Processor 0 Pass Up Vector.*/
	passupvector_t *passUpVector = (passupvector_t *) PASSUPVECTOR;

	passUpVector->tlb_refill_handler = (memaddr) &uTLB_RefillHandler;
	passUpVector->tlb_refill_stackPtr = (memaddr) STACKSTART;
	passUpVector->exception_handler = (memaddr) &exceptionHandler;
	passUpVector->exception_stackPtr = (memaddr) STACKSTART;

	initPcbs();
	initASL();

	ready_tp = mkEmptyProcQ();
	current_proc = NULL;

	LDIT(INTIMER);

	pcb_PTR process = allocPcb();

	process_count++;
	process->p_s.s_status = IEPON | TEBITON | IMON;
	RAMTOP(process->p_s.s_sp);

	process->p_s.s_pc = (memaddr) &test;
	process->p_s.s_t9 = (memaddr) &test;

	insertProcQ(&ready_tp, process);

	scheduler();
	return 1;
}

