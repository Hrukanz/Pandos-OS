#include <umps3/umps/libumps.h>

#include "../h/const.h"

#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/interrupts.h"
#include "../h/scheduler.h"
#include "../h/syscalls.h"

/*
 * Date: 31/7/2024
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 * This module implements the TLB, ProgramTrap ,and SYSCALL exception handlers. 
 * Furthermore, this module will contain the provided skeleton TLB-Refill event
 * handler (e.g. uTLB RefillHandler).
 * 
 * All syscall services are only available in kernel mode. If this is not met,
 * the program trap exception handler is called.
 * 
 * Syscalls that block (SYS3, SYS5, SYS7) all require special steps:
 * - PC incremented by wordsize to avoid loop of syscalls
 * - Saved processor state is copied into current processes pcb (p_s)
 * - Update CPU time for current process
 * - Block current process
 * - Call scheduler
 * 
 */



cpu_t timePassed() {
	volatile cpu_t clockTime;

	STCK(clockTime);
	return clockTime - timeSlice;
}

HIDDEN void passUpOrDie(int index) {
	support_t *supportStructure = current_proc->p_supportStruct;
	/* If no support structure is present, terminate the current process */
	if (supportStructure == NULL) {
		terminateProc();
	}
	else {
		/* Save the current processor state in the appropriate location within the support structure */
		supportStructure->sup_exceptState[index] = *EXCSTATE;
		/* Load the appropriate exception handler context from the support structure and switch to it */
		context_t *context = &(supportStructure->sup_exceptContext[index]);
		LDCXT(context->c_stackPtr, context->c_status, context->c_pc);
	}
}

void* memcpy(void *dest, const void *src, size_t len) {
	char *d = dest;
	const char *s = src;

	while (len--) {
		*d++ = *s++;
	}
	return dest;
}

/* 
 * TLB Exception Handler
 * - Pass Up or Die based on the Current Process's support structure
 * - Copy saved exception state to the appropriate sup_exceptState field
 * - Perform LDCXT using the sup_exceptContext field
 */
HIDDEN void TLBExceptionHandler() {
	passUpOrDie(PGFAULTEXCEPT);
}

/* 
 * Program Trap Exception Handler
 * - Pass Up or Die based on the Current Process's support structure
 * - Copy saved exception state to the appropriate sup_exceptState field
 * - Perform LDCXT using the sup_exceptContext field
 */
HIDDEN void trapHandler() {
	passUpOrDie(GENERALEXCEPT);
}

/* 
 * SYSCALL Exception Handler
 * - Handle SYS1 to SYS8 requests in kernel-mode
 * - Trigger Program Trap for user-mode requests
 * - Pass Up or Die for SYSCALLs numbered 9 and above
 */
HIDDEN void syscallHandler(unsigned int KUp) {
	volatile unsigned int sysId = EXCSTATE->s_a0;

	volatile unsigned int arg1 = EXCSTATE->s_a1;
	volatile unsigned int arg2 = EXCSTATE->s_a2;
	volatile unsigned int arg3 = EXCSTATE->s_a3;
	memaddr resultAddress = (memaddr) &(EXCSTATE->s_v0);

	if (sysId <= 8) {

		if (KUp == 0) {
			/* Execute commands in kernal mode */
			EXCSTATE->s_pc += WORDLEN;
			switch (sysId) {
			case CREATEPROCESS:
				createProc((state_t *) arg1, (support_t *) arg2);
				break;
			case TERMINATEPROCESS:
				terminateProc();
				break;
			case PASSEREN:
				passeren((semaphore *) arg1);
				break;
			case VERHOGEN:
				verhogen((semaphore *) arg1);
				break;
			case WAITIO:
				waitIO(arg1, arg2, arg3);
				break;
			case GETTIME:
				cpuTime((cpu_t *) resultAddress);
				break;
			case CLOCKWAIT:
				waitClk();
				break;
			case GETSUPPORTPTR:
				getSupportData((support_t **) resultAddress);
				break;
			default:
				terminateProc();
				break;
			}
			if (current_proc == NULL)
				scheduler();
			else
				LDST(EXCSTATE);
		}
		else {
			/* Terminate if in user mode*/
			EXCSTATE->s_cause &= ~GETEXECCODE;
			EXCSTATE->s_cause |= EXCODESHIFT << CAUSESHIFT;
			trapHandler();
		}
	}
	else {
		passUpOrDie(GENERALEXCEPT);
	}
}

/* 
 * Exception handler 
 * - Get exception code from Cause.ExcCode
 * - Code 0: Interrupts
 * - Code 1-3: TLB
 * - Code 4-7, 9-12: Program Traps
 * - Code 8: Syscall
 *  
 */
void exceptionHandler()
{

    /* ExcCode from cause register */
    int exc_code = EXCCODE(EXCSTATE->s_cause);
    /* Code 0, int handler */
    if (exc_code == 0) {
        intExceptionHandler(EXCSTATE);
    /* TLB codes of 1-3, tlb handler */
    } else if (((exc_code >= 1) && (exc_code <= TLBS))) {
        TLBExceptionHandler();
    /* Syscall code 8 */
    } else if (exc_code == 8) {
		unsigned int KUp = KUP(EXCSTATE->s_status);
        syscallHandler(KUp);
    /* All other codes from 4-7 and 9-12 */
    } else {
        trapHandler();
    }
    
}

