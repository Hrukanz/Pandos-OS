/* 
 * Delay Facility Module
 * 
 * This module implements the SYS18 Delay facility, allowing a U-proc to be delayed for a specified number of seconds.
 * It maintains an Active Delay List (ADL) of sleeping processes and a 
 * Delay Daemon that wakes up processes when their delay has expired.
 * 
 * The module includes:
 * - Initialisation of the ADL and launching of the Delay Daemon.
 * - Implementation of SYS18 (Delay service).
 * - Functions for managing the ADL and the free list of delay event descriptor nodes.
 * 
 * The Delay Daemon runs as a kernel process, 
 * waking up every 100 milliseconds to check for processes whose delay has expired.
 */

#include "../h/types.h"
#include "../h/const.h"

#include <umps3/umps/libumps.h>

#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/deviceSupportDMA.h"
#include "../h/delayDaemon.h"

int delaySemaphore; 

delay_PTR delaydFree; /* Delay free list */
delay_PTR delaydFree_h; /* Head of delay free list*/

/* 
 * Initialises the Active Delay List (ADL) and starts the Delay Daemon process.
 *
 * This function performs the following steps:
 * - Initialises the delay event descriptor nodes and sets up the free list (delaydFree).
 * - Initialises the ADL (delaydFree_h) with a dummy tail node.
 * - Initialises the delay semaphore (delaySemaphore) for mutual exclusion over the ADL.
 * - Sets up the initial state for the Delay Daemon process and creates it using SYSCALL CREATEPROCESS.
 *
 * If the Delay Daemon process cannot be created, the system is terminated.
 */
void initADL () 
{
    static delayd_t delayEvents[MAXPROC +1];
    state_t initial_state;
    memaddr currentRAMTOP;
    int reset_code;

    delaySemaphore = 1;

    RAMTOP(currentRAMTOP);

    /* Set up the initial state for the Delay Daemon process */
    initial_state.s_sp = currentRAMTOP;
    initial_state.s_pc = (memaddr) delayDaemon;                      
    initial_state.s_t9 = (memaddr) delayDaemon;                     

    /* Set the status register to kernel mode with all interrupts enabled */
    initial_state.s_status = ALLOFF | IEPON | IMON | TEBITON;
    initial_state.s_entryHI = (DELAYASID << ASIDSHIFT);             

    reset_code = SYSCALL(CREATEPROCESS, (int) &initial_state, NULL, 0);

    if (reset_code != OK) {
        SYSCALL(TERMINATEPROCESS, 0, 0, 0);
    }

    delaydFree = &delayEvents[0];

    int i;
    for (i = 1; i < MAXPROC; i++) {
        delayEvents[i-1].d_next = &delayEvents[i];
    }

    /* Initialise the Active Delay List (ADL) with a dummy tail node */
    delayEvents[MAXPROC - 1].d_next = NULL;
    delaydFree_h = &delayEvents[MAXPROC];         
    delaydFree_h->d_next = NULL;
    delaydFree_h->d_supStruct = NULL;
    delaydFree_h->d_wakeTime = NEVER;      

}

/* 
 * Implements the SYS18 Delay service for delaying the current U-proc.
 *
 * This function performs the following steps:
 * - Retrieves the number of seconds to delay from the U-proc's exception state.
 * - If the delay time is valid (greater than zero), proceeds to:
 *   - Obtain mutual exclusion over the ADL by performing a SYS3 PASSEREN on delaySemaphore.
 *   - Allocates a delay event descriptor node and inserts it into the ADL.
 *     - If allocation fails, terminates the U-proc.
 *   - Disables interrupts to ensure atomicity when releasing the semaphore and blocking the U-proc.
 *   - Releases mutual exclusion over the ADL by performing a SYS4 VERHOGEN on delaySemaphore.
 *   - Performs a SYS3 PASSEREN on the U-proc's private semaphore, blocking the U-proc.
 *   - Restores interrupts after blocking.
 */
void delayCurrentProc(support_t *current_support) {
    int sleepTime;

    /* Retrieve the number of seconds to delay from the U-proc's exception state */
    sleepTime = current_support->sup_exceptState[GENERALEXCEPT].s_a2;

    if (sleepTime > 0) {
        SYSCALL(PASSEREN, (int) &delaySemaphore, 0, 0);

        /* Check if delay node inserted correctly */
        if(insertDelayNode(current_support, sleepTime) == FALSE) {
            terminate(NULL);
        }

        setSTATUS(INTSOFF);

        SYSCALL(VERHOGEN, (int) &delaySemaphore, 0, 0);

        SYSCALL(PASSEREN, (int) &current_support->sup_privateSem, 0, 0);

        setSTATUS(INTSON);
    }
    else if (sleepTime < 0) {
        terminate(NULL);
    }
}

/*
 * The Delay Daemon process that manages the waking up of delayed U-proc's.
 *
 * The Delay Daemon runs in an infinite loop, performing the following steps:
 * - Waits for the next 100 millisecond pseudo-clock tick by performing a SYS7 CLOCKWAIT.
 * - Obtains mutual exclusion over the ADL by performing a SYS3 PASSEREN on delaySemaphore.
 * - Checks the ADL for any U-proc's whose wake-up time has passed.
 *   - For each such U-proc:
 *     - Performs a SYS4 VERHOGEN on the U-proc's private semaphore to wake it up.
 *     - Removes the delay event descriptor node from the ADL and returns it to the free list.
 * - Releases mutual exclusion over the ADL by performing a SYS4 VERHOGEN on delaySemaphore.
 */
void delayDaemon () {
    cpu_t currentTime;

    while (TRUE) {
        SYSCALL(CLOCKWAIT, 0, 0, 0);

        SYSCALL(PASSEREN, (int) &delaySemaphore, 0, 0);

        STCK(currentTime);

        while (delaydFree_h->d_wakeTime <= currentTime) {
            SYSCALL(VERHOGEN, (int) &delaydFree_h->d_supStruct->sup_privateSem, 0, 0);
            freeNode(delaydFree_h);
        }

        SYSCALL(VERHOGEN, (int) &delaySemaphore, 0, 0);

    }
}

/*
 * Inserts a new delay event descriptor into the Active Delay List (ADL).
 *
 * Parameters:
 * - current_support: The support structure of the U-proc requesting the delay.
 * - sleepTime: The number of seconds to delay.
 *
 * Returns:
 * - TRUE if the insertion is successful.
 * - FALSE if the insertion fails due to lack of available delay event descriptor nodes.
 *
 * The function performs the following steps:
 * - Allocates a new delay event descriptor node from the free list.
 *   - If allocation fails, returns FALSE.
 * - Calculates the wake-up time based on the current time and the requested sleep time.
 * - Inserts the new node into the ADL in ascending order of wake-up time.
 */
int insertDelayNode(support_t *current_support, int sleepTime) {
    delay_PTR tempNode;
    delay_PTR newNode;
    cpu_t currentTime;

    /* Allocate a new delay event descriptor node */
    newNode = activateASL();

    if (newNode == NULL) {
        return FALSE;
    }

    /* Get the current time */
    STCK(currentTime);

    newNode->d_supStruct = current_support;
    newNode->d_wakeTime = (SECOND * sleepTime) + currentTime;

    /* Insert the new node into the ADL in ascending order of wake-up time */
    if(newNode->d_wakeTime < delaydFree_h->d_wakeTime) {
        newNode->d_next = delaydFree_h;
        delaydFree_h = newNode;
    }
    else {
        /* Traverse the ADL to find the correct position */
        tempNode = delaydFree_h;

        while (tempNode->d_next->d_wakeTime < newNode->d_wakeTime) {
            tempNode = tempNode->d_next;
        }

        newNode->d_next = tempNode->d_next;
        tempNode->d_next = newNode;
    }

    return(TRUE);

}

/*
 * Allocates a new delay event descriptor node from the free list.
 *
 * Returns:
 * - A pointer to the allocated delay event descriptor node.
 * - NULL if no nodes are available.
 *
 * The function performs the following steps:
 * - Checks if the free list (delaydFree) is empty.
 *   - If empty, returns NULL.
 * - Removes a node from the free list.
 * - Initialises the node's fields.
 */
delay_PTR activateASL () {
    delay_PTR tempNode;

    if (delaydFree == NULL) {
        return NULL;
    }
    else {
        tempNode = delaydFree;
        delaydFree = delaydFree->d_next;

        tempNode->d_next = NULL;
        tempNode->d_wakeTime = 0;
        tempNode->d_supStruct = NULL;

        return tempNode;
    }
}

/*
 * Returns a delay event descriptor node to the free list.
 *
 * Parameters:
 * - delayEvent: A pointer to the delay event descriptor node to be freed.
 *
 * The function performs the following steps:
 * - Adds the node back to the free list (delaydFree).
 */
void freeNode(delay_PTR delayEvent) {
    delayEvent->d_next = delaydFree;
    delaydFree = delayEvent;
}