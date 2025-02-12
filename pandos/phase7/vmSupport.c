/*
 * vmSupport.c
 * Authors: Haruka Yamamoto & James Rushworth
 *
 * This module provides support for address translation/virtual memory and
 * character-oriented I/O devices for user processes (U-procs). Each U-proc
 * executes in its own logical address space with a unique Address Space Identifier (ASID).
 *
 * The Pager function handles page faults using the following steps:
 *
 * 1. Obtain the pointer to the Current Process's Support Structure.
 * 2. Determine the cause of the TLB exception.
 * 3. Handle TLB-Modification exceptions as program traps.
 * 4. Gain mutual exclusion over the Swap Pool table.
 * 5. Determine the missing page number.
 * 6. Select a frame from the Swap Pool using a page replacement algorithm.
 * 7. If the frame is occupied, update the Page Table, TLB, and backing store of the occupying process.
 * 8. Read the missing page from the backing store into the selected frame.
 * 9. Update the Swap Pool table and the Current Process's Page Table.
 * 10. Update the TLB.
 * 11. Release mutual exclusion over the Swap Pool table.
 * 12. Return control to the Current Process to retry the instruction that caused the page fault.
 *
 * Helper Functions
 *
 * initSwapStructs()
 *
 * Initializes the swap pool and device semaphores. It sets up:
 * - All device semaphores for mutual exclusion.
 * - Initializes the swap pool semaphore.
 * - Marks all frames in the swap pool as free by assigning `NOPROC` to the ASID field.
 *
 * selectFrame()
 *
 * Implements a round-robin frame selection algorithm to pick a frame from the swap pool.
 * It returns the index of the next frame to use in a cyclic manner.
 *
 * updateTLB()
 *
 * Updates the Translation Lookaside Buffer (TLB) with the given page table entry.
 * It first loads the entry into the TLB and, if valid, writes it to the appropriate TLB index.
 *
 * uTLB_RefillHandler()
 *
 * Handles TLB refill exceptions by loading the missing page into the TLB from
 * the current process's page table.
 */

#include <umps3/umps/libumps.h>

#include "../h/exceptions.h"
#include "../h/initial.h"

#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/deviceSupportDMA.h"  /* Include the device support header */

/* Constants */
#define FLASHSEM 1  /* Flash devices are at index 1 in the device semaphore array */

/* Swap pool and semaphore global variables */
swap_t swapPool[POOLSIZE];
semaphore swapSemaphore;

/* Initialise the swap pool and support device semaphores */
void initSwapPool() {
    int i, j, k;

    /* Initialise device semaphores to 1 */
    for (i = 0; i < DEVICE_TYPES; i++) {
        for (j = 0; j < DEVICE_INSTANCES; j++) {
            support_device_sems[i][j] = 1;
        }
    }

    swapSemaphore = 1;
    testSem = 0;

    /* Initialise swap pool with NOPROC */
    for (k = 0; k < POOLSIZE; k++) {
        swapPool[k].sw_asid = NOPROC;
    }
}

/* Helper function to select a frame from the Swap Pool */
/* Round robin design */
int selectFrame() {
    static int selectFrame = 0;
    selectFrame = (selectFrame + 1) % POOLSIZE;
    return selectFrame;
}

/* Helper function to update the TLB */
void updateTLB(pteEntry_t *pageTableEntry) {
    unsigned int prevEntry = getENTRYHI();

    setENTRYHI(pageTableEntry->pte_entryHI);
    TLBP();

    if ((getINDEX() & KUSEG) == 0) {
        setENTRYHI(pageTableEntry->pte_entryHI);
        setENTRYLO(pageTableEntry->pte_entryLO);
        TLBWI();
    }

    setENTRYHI(prevEntry);
}

/* Handle TLB refill exceptions */
void uTLB_RefillHandler() {
    int missingPageNum = (EXCSTATE->s_entryHI & MISSINGPAGESHIFT) >> VPNSHIFT;
    missingPageNum %= MAXPAGES;

    setENTRYHI(current_proc->p_supportStruct->sup_privatePgTbl[missingPageNum].pte_entryHI);
    setENTRYLO(current_proc->p_supportStruct->sup_privatePgTbl[missingPageNum].pte_entryLO);

    TLBWR();
    returnControl();
}

/* Pager function to handle page faults */
void pager() {
    int frameNum = 0;
    int blockID;
    unsigned int frameAddress;
    pteEntry_t *pageTableEntry;
    support_t *supportStruct;
    int deviceStatus;
    int flashNum;

    /* Obtain the pointer to the Current Process's Support Structure */
    supportStruct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);

    /* Determine the cause of the TLB exception */
    int cause = (supportStruct->sup_exceptState[PGFAULTEXCEPT].s_cause & GETEXECCODE) >> CAUSESHIFT;

    /* Handle TLB-Modification exceptions as program traps */
    if (cause == 1) {
        trapExcHandler(supportStruct);
    }

    /* Gain mutual exclusion over the Swap Pool table */
    SYSCALL(PASSEREN, (int)&swapSemaphore, 0, 0);

    int asid = supportStruct->sup_asid;
    /* Determine the missing page number */
    int missingPageNum = ((supportStruct->sup_exceptState[PGFAULTEXCEPT].s_entryHI) & MISSINGPAGESHIFT) >> VPNSHIFT;

    /* Pick a frame from the Swap Pool */
    frameNum = selectFrame();
    frameAddress = (frameNum * PAGESIZE) + FRAMEADDRSHIFT;

    /* If the frame is occupied, update the Page Table, TLB, and backing store of the occupying process */
    if (swapPool[frameNum].sw_asid != NOPROC) {
        /* Update TLB and swap pool atomically */
        setSTATUS(INTSOFF);

        swapPool[frameNum].sw_pte->pte_entryLO &= VALIDOFF;
        updateTLB(swapPool[frameNum].sw_pte);

        setSTATUS(INTSON);

        blockID = swapPool[frameNum].sw_pageNo;
        blockID = blockID % MAXPAGES;

        flashNum = swapPool[frameNum].sw_asid - 1;

        /* Mutex on flash device semaphore */
        SYSCALL(PASSEREN, (memaddr)&support_device_sems[FLASHSEM][flashNum], 0, 0);

        /* Perform the flash write operation */
        deviceStatus = flashOp(flashNum, blockID, frameAddress, FLASHWRITE);

        /* Release semaphore for the flash device */
        SYSCALL(VERHOGEN, (memaddr)&support_device_sems[FLASHSEM][flashNum], 0, 0);

        /* If device failed to execute command, call trap exception handler */
        if (deviceStatus != READY) {
            trapExcHandler(supportStruct);
        }
    }

    /* Calculate Block ID */
    blockID = missingPageNum;
    blockID = blockID % MAXPAGES;

    flashNum = asid - 1;

    /* Mutex on flash device semaphore */
    SYSCALL(PASSEREN, (memaddr)&support_device_sems[FLASHSEM][flashNum], 0, 0);

    /* Perform the flash read operation */
    deviceStatus = flashOp(flashNum, blockID, frameAddress, FLASHREAD);

    /* Release semaphore for the flash device */
    SYSCALL(VERHOGEN, (memaddr)&support_device_sems[FLASHSEM][flashNum], 0, 0);

    /* If device failed to execute command, call trap exception handler */
    if (deviceStatus != READY) {
        trapExcHandler(supportStruct);
    }

    /* Update the Swap Pool table and the Current Process's Page Table */
    pageTableEntry = &(supportStruct->sup_privatePgTbl[blockID]);
    swapPool[frameNum].sw_asid = asid;
    swapPool[frameNum].sw_pageNo = missingPageNum;
    swapPool[frameNum].sw_pte = pageTableEntry;

    /* Atomically update TLB and page table entry */
    setSTATUS(INTSOFF);

    swapPool[frameNum].sw_pte->pte_entryLO = frameAddress | VALIDON | DIRTYON;
    updateTLB(&(supportStruct->sup_privatePgTbl[blockID]));

    setSTATUS(INTSON);

    /* Release mutual exclusion over the Swap Pool table */
    SYSCALL(VERHOGEN, (int)&swapSemaphore, 0, 0);

    /* Return control to the current process to retry the instruction that caused the page fault */
    returnControlSup(supportStruct, PGFAULTEXCEPT);
}