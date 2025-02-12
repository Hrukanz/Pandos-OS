/*
 * Date: 21/8/2024
 * Authors: Haruka Yamamoto & James Rushworth
 * Simple preemptive round-robin scheduling algorithm 
 * with a time slice value of 5 milliseconds.
 * 
 * The Scheduler should dispatch the “next” process in the Ready Queue.
 * This only occurs if the Ready Queue isn't empty, otherwise the program will either:
 * HALT: If process_count is 0
 * WAIT: If Soft-block Count > 0 
 * PANIC: If process_count > 0 and the Soft-block Count = 0
 */

#include "../h/types.h"
#include "../h/const.h"

#include "../h/pcb.h"
#include "../h/asl.h"

#include "../h/scheduler.h"
#include "../h/initial.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"

#include "/usr/include/umps3/umps/libumps.h"

volatile cpu_t timeSlice;

void scheduler() {
	if (emptyProcQ(ready_tp)) {
		if (process_count == 0) {
			HALT();
		}

		if (process_count > 0 && soft_block_count > 0) {
			unsigned int status = getSTATUS();
            setTIMER(TICKCONVERT(MAXPLT));
            setSTATUS((status) | IECON | IMON);

			WAIT();

			setSTATUS(status);
		}

		if (process_count > 0 && soft_block_count == 0) {
			PANIC();
		}
	}

	current_proc = removeProcQ(&ready_tp);

	setTIMER(TICKCONVERT(TIMESLICE));

	STCK(timeSlice);

	LDST(&(current_proc->p_s));
}

