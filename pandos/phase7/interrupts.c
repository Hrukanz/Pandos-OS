/*
 * Date: 21/8/2024
 * Authors: Haruka Yamamoto & James Rushworth
 * This module implements the interrupt exception handler. Specifically it:
 * Handles the Processor Local Timer (PLT) interrupt
 * Handles the System-wide Interval Timer and Pseudo-clock interrupt
 * Handles device interrupts
 */

#include "../h/const.h"
#include "../h/types.h"

#include "../h/asl.h"
#include "../h/pcb.h"

#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/interrupts.h"
#include "../h/scheduler.h"
#include "../h/syscalls.h"

#include <umps3/umps/libumps.h>

/* Helper function */
/* Locate the correct interupt line for the device*/
int findIntLine(unsigned int map) {
    int i;
	for (i = 0; i < 32; i++) {
        if (map & (1 << i)) {
            return i;
        }
    }
    PANIC();
    return -1;
}

/* 
 * V's the semaphore and unblocks the process
 * - Get the returned process
 * - Put the interrupt status CODE in the v0 reg
 * - Decrement soft block count
 */
void unblockLoad(int deviceType, int deviceInstance, unsigned int status) {
	pcb_PTR unblockedProc;

	unblockedProc = verhogen(&(device_sems[deviceType][deviceInstance]));

	if (unblockedProc != NULL) {
		unblockedProc->p_s.s_v0 = status;
		soft_block_count--;
	}
}

/* 
 * Non-Timer Interrupt Handler
 * 1. Calculate the device register address
 * 2. Save the status code from the device register
 * 3. Acknowledge the interrupt
 * 4. Perform a V operation on the device semaphore
 * 5. Place the status code in the unblocked pcb's v0 register
 * 6. Insert the unblocked pcb into the Ready Queue
 * 7. Return control to the Current Process (LDST on saved exception state)
 */
void nonTimerInterrupt(int deviceType) {
	int instanceMap = DEVREGADDR->interrupt_dev[deviceType];

	instanceMap &= -instanceMap;
	int deviceInstance = findIntLine(instanceMap);
	unsigned int status;

	if (deviceType == (TERMINT-DISKINT)) {
		device_t *termStatus = &(DEVREGADDR->devreg[deviceType][deviceInstance]);

		if ((termStatus->d_status & TERMSTATUSMASK) == RECVD_CHAR) {
			status = termStatus->d_status;
			DEVREGADDR->devreg[deviceType][deviceInstance].d_command = ACK;
			unblockLoad(deviceType, deviceInstance, status);
		}
		if ((termStatus->d_data0 & TERMSTATUSMASK) == TRANS_CHAR) {
			status = termStatus->d_data0;
			DEVREGADDR->devreg[deviceType][deviceInstance].d_data1 = ACK;
			unblockLoad(deviceType + 1, deviceInstance, status);
		}
	}
	else {
		status = DEVREGADDR->devreg[deviceType][deviceInstance].d_status;
		DEVREGADDR->devreg[deviceType][deviceInstance].d_command = ACK;
		unblockLoad(deviceType, deviceInstance, status);
	}
	if (current_proc == NULL)
		scheduler();
	else
		LDST(EXCSTATE);
}

/* 
 * Processor Local Timer (PLT) Interrupt Handler
 * 1. Acknowledge the PLT interrupt by reloading the timer
 * 2. Copy the saved processor state into the Current Process's pcb
 * 3. Place the Current Process on the Ready Queue
 * 4. Call the Scheduler
 */
void pltInterrupt() {
	setTIMER(TICKCONVERT(MAXPLT));
	current_proc->p_s = *EXCSTATE;
	current_proc->p_time += timePassed();
	insertProcQ(&ready_tp, current_proc);
	current_proc = NULL;
	scheduler();
}

/* 
 * System-wide Interval Timer and Pseudo-clock Handler
 * 1. Acknowledge the Interval Timer interrupt by reloading it
 * 2. Unblock all pcbs blocked on the Pseudo-clock semaphore
 * 3. Reset the Pseudo-clock semaphore to zero
 * 4. Return control to the Current Process (LDST on saved exception state)
 */
void intervalTimerInterrupt() {
	LDIT(INTIMER);
	pcb_t *blockedProcess = NULL;

	while ((blockedProcess = removeBlocked(&IntervalTimerSem)) != NULL) {
		insertProcQ(&ready_tp, blockedProcess);
	}

	soft_block_count += IntervalTimerSem;
	IntervalTimerSem = 0;

	if (current_proc == NULL)
		scheduler();
	else
		LDST(EXCSTATE);
}

/* 
 * Device/Timer Interrupt Exception Handler
 * Handles all device and timer interrupts, converting them into V operations
 * on the appropriate semaphores.
 */
void intExceptionHandler(state_t *exceptionState) {
	int pending_int = (exceptionState->s_cause & GETIP);

	pending_int &= -pending_int;

	switch (pending_int) {
	case LOCALTIMERINT:
		pltInterrupt();
		break;
	case TIMERINTERRUPT:
		intervalTimerInterrupt();
		break;
	case DISKINTERRUPT:
	case FLASHINTERRUPT:
	case NETWINTERRUPT:
	case PRINTINTERRUPT:
	case TERMINTERRUPT:
		nonTimerInterrupt(findIntLine(pending_int >> IPSHIFT) - DISKINT);
		break;
	default:
		break;
	}
}