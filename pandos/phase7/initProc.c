/*
 * initProc.c
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 * This module is responsible for initialising the support level structures and processes
 * necessary for phase 3. This includes setting up device semaphores,
 * initialising the swap pool, and launching the initial user processes (U-procs).
 *
 * The key functions and responsibilities of this module are:
 * 
 * 1. Initialise Support Structures:
 *    - Initialise the Swap Pool table and its associated semaphore.
 *    - Initialise semaphores for I/O devices to ensure mutual exclusion during access.
 * 
 * 2. Launch U-procs:
 *    - Set up the initial processor state for each U-proc.
 *    - Configure exception handling contexts for page faults and general exceptions.
 *    - Set up page tables for each process, managing virtual-to-physical memory mapping.
 *    - Use SYS1 to start U-procs with the specified processor state and support structure.
 * 
 * 3. Terminate or Handle Completion:
 *    - Wait for all U-procs to finish execution using semaphores.
 *    - Terminate after all U-procs complete or handle deadlocks if they occur.
 * 
 * Helper Functions
 * 
 * dealocate_sup()
 * 
 * This function deallocates a given `support_t` structure by adding it 
 * back to the freeSupports stack, making it available for reuse.
 * 
 * allocate_sup()
 * 
 * This function retrieves a `support_t` structure from the freeSupports stack.
 * 
 * initSupport()
 * 
 * This function initialises the stack of support structures by deallocating all
 * available `support_t` structures at the beginning of execution.
 * 
 * test() 
 * 
 * This is the main function responsible for initializing the system's device semaphores
 * and launching the U-procs. It performs the following tasks:
 * 
 * 1. Initialise Semaphores: Initialises the device semaphores and the swap pool.
 * 2. Set Up Processor State: Configures the PC, stack pointer, status register, and ASID for each U-proc.
 * 3. Set Exception Handlers: Sets up exception contexts for page faults and general exceptions.
 * 4. Initialise Page Tables: Sets up the virtual-to-physical page mapping for each U-proc.
 * 5. Launch U-procs: Creates and starts each U-proc using the SYS1 system call.
 * 6. Wait for Completion: Waits for all U-procs to finish using the testSem semaphore.
 * 7. Terminate Process: Terminates the current process after all U-procs have finished.
 */

#include <umps3/umps/libumps.h>
#include "../h/const.h"
#include "../h/types.h"

#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/delayDaemon.h"

/* Global Variables */
int testSem;
support_t support_structs[UPROCMAX];
support_t *freeSupports[UPROCMAX+1];
int stackSupport;

/* Helper function to deallocate support structs to the free list */
void dealocate_sup(support_t *support){
    freeSupports[stackSupport] = support;
    stackSupport++;
}

/* Helper function to remove support struct from free list and return */
support_t* allocate_sup() {
    support_t *tempSupport = NULL;
    
    if (stackSupport != 0){
        stackSupport--;
        tempSupport = freeSupports[stackSupport];
    }
    return tempSupport;
}

/* Load the free list of support structs */
void initSupport() {
    stackSupport = 0;
    int i;
    for (i = 0; i < UPROCMAX; i++){
        dealocate_sup(&support_structs[i]);
    }
}

/**
 * This function is used to initialise the process and device registers.
 * 
 * It initialises the device register semaphores and sets up the processes.
 * For each process, it initialises the process page table and creates the process using the SYSCALL function.
 * After initialising all the processes, it waits for all the processes to finish using the testSem semaphore.
 * Finally, it terminates the current process.
 */
void test() {
    /* Initalise device reg semaphores */
    initSwapPool();
    initSupport();
    initADL();

    state_t initial_state;

    /* Set up initial processor state for each process */
    initial_state.s_pc = UPROCSTARTADDR;
    initial_state.s_t9 = UPROCSTARTADDR;
    initial_state.s_sp = USERSTACKTOP;
    initial_state.s_status = IEPON | IMON | TEBITON | USERPON;

    /*Setup processes*/
    support_t *supportStruct;
    int proc;
    for (proc= 1; proc <= UPROCMAX; proc++) {
        initial_state.s_entryHI = (proc << ASIDSHIFT);

        supportStruct = allocate_sup();
        
        /* Set up exception context for each process */
        supportStruct->sup_privateSem = 0;

        supportStruct->sup_asid = proc;
        supportStruct->sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) &pager;
        supportStruct->sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) &sysSupportGenHandler; 
        
        supportStruct->sup_exceptContext[PGFAULTEXCEPT].c_status = IEPON | IMON | TEBITON;
        supportStruct->sup_exceptContext[GENERALEXCEPT].c_status = IEPON | IMON | TEBITON;

        supportStruct->sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr) &(supportStruct->sup_stackTLB[STACKSIZE]);
        supportStruct->sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr) &(supportStruct->sup_stackGen[STACKSIZE]);
        
        /* Initalise process page table */
        int pgTblSize;
        for(pgTblSize = 0; pgTblSize<USERPGTBLSIZE-1; pgTblSize++) {
            supportStruct->sup_privatePgTbl[pgTblSize].pte_entryHI = VPNBASE + (pgTblSize << VPNSHIFT) + (proc << ASIDSHIFT);
            supportStruct->sup_privatePgTbl[pgTblSize].pte_entryLO = DIRTYON;
        }
        
        /* Set last entry in page table to the stack*/
        supportStruct->sup_privatePgTbl[USERPGTBLSIZE-1].pte_entryHI = UPROCSTACKPG + (proc << ASIDSHIFT);
        supportStruct->sup_privatePgTbl[USERPGTBLSIZE-1].pte_entryLO = DIRTYON;

         /* Create the process using SYSCALL function */
        SYSCALL(CREATEPROCESS, (memaddr) &initial_state, (memaddr)supportStruct, 0);
    }
    
    /* Wait for all processes to finish using testSem semaphore */
    int i;
    for (i = 0; i < UPROCMAX; i++){
        SYSCALL(PASSEREN, (memaddr) &testSem, 0, 0);
    }

    /* Terminate the current process */
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}
