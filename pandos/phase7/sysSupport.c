/*
 * sysSupport,c
 * Date: 26/8/2024
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 * Support-level Exception and System Call Handlers
 * 
 * This file implements the support-level handling of general exceptions and
 * specific system calls for user processes. It deals with SYSCALL and Program Trap
 * exceptions, providing services like I/O operations, process termination, and 
 * resource management.
 * 
 * The following handlers are implemented:
 * 
 * 1. General Exception Handler:
 *    - Handles all non-TLB exceptions passed up from user mode.
 *    - Specifically processes SYSCALL exceptions (SYS9 to SYS13) and Program Trap exceptions.
 *    - Directs SYSCALL exceptions to the appropriate handler based on the exception code.
 *    - Terminates the process for unrecognized exceptions.
 * 
 * 2. SYSCALL Handler:
 *    - Processes SYSCALL exceptions (SYS9 to SYS13).
 *    - Supports system calls like terminating processes, getting time of day, writing to printer, writing to/read from terminal.
 *    - Advances the program counter after handling the exception.
 * 
 * 3. Helper Functions:
 *    - `terminate()`: Terminates the current process, deallocates resources, and handles mutex semaphores.
 *    - `getTOD()`: Returns the number of microseconds since the last system reset/reboot.
 *    - `writeToPrinter()`: Suspends the process until output is transmitted to the printer.
 *    - `writeToTerminal()`: Suspends the process until output is transmitted to the terminal.
 *    - `readTerminal()`: Suspends the process until input is received from the terminal.
 * 
 * Trap Handling:
 * - Unhandled exceptions lead to the termination of the process via `trapExcHandler()`.
 */

#include <umps3/umps/libumps.h>
#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/deviceSupportDMA.h"
#include "../h/delayDaemon.h"


/* 2D Array of support level device semaphores */
int support_device_sems[DEVICE_TYPES][DEVICE_INSTANCES];


/* Helper function to LDST using exception state */
void returnControl()
{

    /* Return control and load the exception state */
    LDST(EXCSTATE);

}

/* Helper function to LDST using support struct. Helps debug and minimise code */
void returnControlSup(support_t *support, int exc_code)
{
    /* Return control and load the exception state */
    LDST(&(support->sup_exceptState[exc_code]));

}

/* Support level program trap exception handler */
void trapExcHandler(support_t *support_struct)
{
    /* Terminates using the support level implementation of SYS2 */
    terminate(support_struct);
}

/* 
 * Support level general exception handler
 * The Support Level general exception handler will process all passed up non-TLB exceptions:
 * - All SYSCALL (SYSCALL) exceptions numbered 9 and above.
 * - All Program Trap exceptions; all exception causes exclusive of those for 
 *   SYSCALL exceptions and those related to TLB exceptions.
 */
/* Support level general exception handler */
void sysSupportGenHandler() {

    /* Retrieve the support structure using SYS8 */
    support_t *support_struct = (support_t *) SYSCALL(GETSUPPORTPTR, 0, 0, 0);

    /* Extract the Cause Register value */
    int cause = support_struct->sup_exceptState[GENERALEXCEPT].s_cause;

    /* Extract the exception code from the Cause Register */
    int exc_code = EXCCODE(cause);

    /* Check if the exception is a SYSCALL */
    if (exc_code == 8) {
        /* Extract the syscall number from a0 */
        int syscall_num = support_struct->sup_exceptState[GENERALEXCEPT].s_a0;

        /* Handle the syscall */
        supportSyscallHandler(syscall_num, support_struct);
    }
    else {
        /* Handle as a Program Trap exception */
        trapExcHandler(support_struct);
    }
}

/*
 * Support Syscall exception handler
 * Handles all SYS9 to SYS13 syscalls.
 */
void supportSyscallHandler(int exc_code, support_t *support_struct)
{

    /* Validate syscall number */
    if (exc_code < TERMINATE || exc_code > DELAY) {
        /* Invalid syscall number, treat as Program Trap */
        trapExcHandler(support_struct);
        return;
    }

    /* Get a1 (arg1) and a2 (arg2) from sup_exceptState to pass to syscalls */
    int arg1 = support_struct->sup_exceptState[GENERALEXCEPT].s_a1;
    int arg2 = support_struct->sup_exceptState[GENERALEXCEPT].s_a2;

    /* Return any values to v0 and load state the sup_exceptState */
    switch(exc_code) {
        case TERMINATE:
            terminate(support_struct);
            break;

        case GET_TOD:
            getTOD(support_struct);
            break;

        case WRITEPRINTER:
            writeToPrinter((char *) arg1, arg2, support_struct);
            break;

        case WRITETERMINAL:
            writeToTerminal((char *) arg1, arg2, support_struct);  
            break;

        case READTERMINAL:
            readTerminal((char *) arg1, support_struct);
            break;

        case FLASH_PUT:
            flashPut(support_struct);
            break;
        
        case FLASH_GET:
            flashGet(support_struct);
            break;

        case DISK_PUT:
            diskPut(support_struct);
            break;
        
        case DISK_GET:
            diskGet(support_struct);
            break;

        case DELAY:
            delayCurrentProc(support_struct);
            break;

        default:
            trapExcHandler(support_struct);
            break;
    }

    /* Increment pc by word size (4) (assuming, the sup_exceptState pc count) */
    /* Ensure the state is updated and return to the caller */
    support_struct->sup_exceptState[GENERALEXCEPT].s_pc += WORDLEN;
        /* Return control to the current process to retry the instruction that caused the page fault */
    returnControlSup(support_struct, GENERALEXCEPT);

}

/* User mode instance of the SYS2 terminateProc syscall */
void terminate(support_t *support_struct)
{
    int dev_num = support_struct->sup_asid - 1;

    /* Check if the process holds a mutex semaphore */
    int i;
    for (i = 0; i < DEVICE_TYPES; i++) {
        if (support_device_sems[i][dev_num] == 0) {
            SYSCALL(VERHOGEN, (memaddr) &support_device_sems[i][dev_num], 0, 0);
        }
    }

    /* Mark all pages as unused */
    for (i = 0; i < MAXPAGES; i++) {
        if(support_struct->sup_privatePgTbl[i].pte_entryLO & VALIDON){
            setSTATUS(INTSOFF);

            /* This could be changed according to mikey*/
            support_struct->sup_privatePgTbl[i].pte_entryLO &= ~VALIDON;
            updateTLB(&(support_struct->sup_privatePgTbl[i]));

            setSTATUS(INTSON);
        }
    }
    SYSCALL(VERHOGEN, (memaddr) &testSem, 0, 0);
    dealocate_sup(support_struct);
    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/* Returns number of microseconds since last reboot/reset to v0 */
void getTOD(support_t *support_struct)
{
    STCK(support_struct->sup_exceptState[GENERALEXCEPT].s_v0);
}

/* Suspends process until output has been transmitted to printer */
void writeToPrinter(char *virtualAddr, int len, support_t *support_struct) {
    int device_instance = support_struct->sup_asid - 1;
    int charCount = 0;

    /* Mutex on the device semaphore */
    SYSCALL(PASSEREN, (memaddr) &support_device_sems[PRINTSEM][device_instance], 0, 0);

    /* Calculate device instance address and store as device_t pointer */
    device_t *device_int = (device_t *)(DEVICEREGSTART + ((PRNTINT - DISKINT) * (DEVICE_INSTANCES * DEVREGSIZE)) + (device_instance * DEVREGSIZE));
    
    int i;
    for (i = 0; i < len; i++) {
        if(device_int->d_status == READY) {

            /* Atomically set the data0 and command fields */
            setSTATUS(INTSOFF);

            device_int->d_data0 = ((int) *(virtualAddr + i));
            device_int->d_command = PRINTCHR;

            /* WaitIO call to block device until the command is completed */
            SYSCALL(WAITIO, PRNTINT, device_instance, 0);
            setSTATUS(INTSON);

            charCount++;
        }
        else {
            /* If device is not ready, return the negative of device status */
            charCount = -(device_int->d_status);

            /* End the loop by setting the index out of bounds */
            i = len;
        }
    }

    /* Load the number of chars transmitted to v0 register */
    support_struct->sup_exceptState[GENERALEXCEPT].s_v0 = charCount;

    /* Release mutex on device semaphore */
    SYSCALL(VERHOGEN, (memaddr) &support_device_sems[PRINTSEM][device_instance], 0, 0);  

}

/* Suspends process until output has been transmitted to terminal */
void writeToTerminal(char *virtualAddr, int len, support_t *support_struct) {
    int device_instance = support_struct->sup_asid - 1;
    unsigned int charCount = 0;
    
    /* Mutex on device semaphore */
    SYSCALL(PASSEREN, (memaddr) &support_device_sems[TERMWRSEM][device_instance], 0, 0);

    /* Adress of device instance */
    device_t *device_int = (device_t *) (DEVICEREGSTART + ((TERMINT - DISKINT) * (DEVICE_INSTANCES * DEVREGSIZE)) + (device_instance * DEVREGSIZE));
    int status = OKCHARTRANS;


    int i;
    for (i = 0; i < len; i++) {
        /* If the terminal is ok to be transmitted to, write to device registers */
        if(((device_int->d_data0 & TERMSTATUSMASK) == READY) && ((status & TERMSTATUSMASK) == OKCHARTRANS)) {

            /* Atomically set the status and data1 fields of the device */
            setSTATUS(INTSOFF);

            device_int->d_data1 = (((int) *(virtualAddr + i)) << TERMTRANSHIFT) | TRANSMITCHAR;
            status = SYSCALL(WAITIO, TERMINT, device_instance, 0);

            setSTATUS(INTSON);
            charCount++;
        }
        else{
            /* If the device is not ready, return the negative of the status*/
            charCount = -(status);
            i = len;
        }
    }

    /* Load the number of char transmissions in v0 */
    support_struct->sup_exceptState[GENERALEXCEPT].s_v0 = charCount;

    /* Release device from mutex */
    SYSCALL(VERHOGEN, (memaddr) &support_device_sems[TERMWRSEM][device_instance], 0, 0);  

}

/* Suspends process until input has been transmitted from the terminal */
void readTerminal(char *virtualAddr, support_t *support_struct){
    int device_instance = support_struct->sup_asid - 1;
    int charCount = 0;
    int status;
    char string = ' ';

    /* Device mutex */
    SYSCALL(PASSEREN, (memaddr) &support_device_sems[TERMSEM][device_instance], 0, 0);

    device_t* device_int = (device_t *)(DEVICEREGSTART + ((TERMINT - DISKINT) * (DEVICE_INSTANCES * DEVREGSIZE)) + (device_instance * DEVREGSIZE));

    /* Loop until the device status is not ready or reaches end of string */
    while(((device_int->d_status & TERMSTATUSMASK) == READY) && (string != EOS)) {
        
        /* Atomically set device command and status */
        setSTATUS(INTSOFF);

        device_int->d_command = TRANSMITCHAR;

        status = SYSCALL(WAITIO, TERMINT, device_instance, TRUE);

        setSTATUS(INTSON);

        /* Check if the terminal can be read */
        if((status & TERMSTATUSMASK) == OKCHARTRANS) {
            
            string = (status >> DEVICE_INSTANCES);

            if(string != EOS) {
                *virtualAddr = string;
                virtualAddr++;
                charCount++;
            }
        }
        else {
            /* End loop if fails */
            string = EOS;
        }
    }

    /* If the device was not ready, return the negative of status */
    if((device_int->d_status & TERMSTATUSMASK) != READY || (status & TERMSTATUSMASK) != OKCHARTRANS) {
        charCount = -(status);
    }

    /* Release device from mutex*/
    SYSCALL(VERHOGEN, (memaddr) &support_device_sems[TERMSEM][device_instance], 0, 0);

    /* Load number of chars transmitted */
    support_struct->sup_exceptState[GENERALEXCEPT].s_v0 = charCount;

}




