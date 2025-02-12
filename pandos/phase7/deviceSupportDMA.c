/* 
 * deviceSupportDMA.c
 * Date: 27/9/2024
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 * This module provides support for disk and flash devices, as part of 
 * the μMPS3 operating system. Unlike character-based devices from Level 4/Phase 3,
 * which operate on a character-by-character basis, this phase supports 
 * DMA (Direct Memory Access) for efficient data transfers between the devices and RAM.
 *
 * Key Features:
 * - Each disk and flash device has a dedicated DMA buffer (4KB) in RAM for direct read/write operations.
 * - Disk operations occur on a sector-by-sector basis, while flash operations occur on a block-by-block basis, 
 *   with each sector/block being 4KB in size.
 * - Disk and flash read operations involve reading the requested sector/block into the DMA buffer 
 *   and subsequently copying data into the requesting U-proc’s address space.
 * - Write operations reverse this process, copying data from the U-proc’s address space to the DMA buffer 
 *   and then overwriting the target sector/block.
 * 
 * Important Considerations:
 * - Source (or sink) logical addresses do not need to be page-aligned, which may trigger page faults.
 * - The system supports up to eight disk devices and eight flash devices, requiring sixteen 4KB RAM frames 
 *   allocated for their DMA buffers.
 * - Disk devices are viewed as one-dimensional arrays of sectors for operation purposes, simplifying 
 *   access to disk data.
 * - Error handling includes termination of U-procs when operations involve invalid addresses or sector/block 
 *   numbers.
 *
 * Functions:
 * - flashGet(): Handles reading a block from a specified flash device. 
 *   This function retrieves data from the flash device's DMA buffer and copies it to the U-proc's address space.
 *   If the provided address is invalid, the U-proc is terminated.
 *
 * - flashPut(): Manages writing a block to a specified flash device. 
 *   This function copies data from the U-proc's address space into the flash device's DMA buffer and 
 *   initiates the write operation.
 *
 * - flashOp(): Performs the actual read/write 
 *   operations on the specified flash device. It constructs the necessary command and manages the device's 
 *   status, returning the result of the operation.
 *
 * - diskOp(): Executes operations on the specified disk device. 
 *   It handles seeking the correct track and sector, performing the read/write operation, and managing device status.
 *
 * - diskPut(): Facilitates writing data to a specified disk sector. 
 *   It suspends the U-proc until the write operation is complete, ensuring data integrity.
 *
 * - diskGet(): Manages reading data from a specified disk sector. 
 *   Similar to `diskPut`, it ensures that the U-proc waits until the read operation is fully complete.
 *
 * This module includes functions for synchronous disk operations (diskPut and diskGet) and handles 
 * the complexities of interacting with DMA devices.
 */


#include "../h/types.h"
#include "../h/const.h"

#include <umps3/umps/libumps.h>

#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/deviceSupportDMA.h"

/* Constants */
#define DISKREAD 3 /* Value associated with disk read for command register */
#define DISKWRITE 4 /* Value for disk write in the command register */
#define SEEKCYLINDER 2 /* Seek command register value */

#define DISKPOOLSTART (FRAMEADDRSHIFT + (POOLSIZE * PAGESIZE)) /* Just after swap pool*/
#define FLASHPOOLSTART (DISKPOOLSTART + (DEVICE_INSTANCES * PAGESIZE)) /* Just after Disk Buffers*/

/* 
 * Reads data from a specified flash device and writes it to a 
 * virtual memory address provided in the current_support structure.
 */
void flashGet(support_t *current_support) {
    int flashNum, sector, device_status;   /* Flash device number, sector number, and device status */
    memaddr *buffer;                        /* Pointer for buffer in memory */
    memaddr *virtualAddr;                   /* Pointer for the virtual address to write data */

    /* Get parameters from the exception state */
    virtualAddr = (memaddr *) current_support->sup_exceptState[GENERALEXCEPT].s_a1;
    flashNum = current_support->sup_exceptState[GENERALEXCEPT].s_a2;
    sector = current_support->sup_exceptState[GENERALEXCEPT].s_a3;

    /* Check if virtual address is valid (greater than KUSEG) */
    if ((int) virtualAddr < KUSEG) {
        terminate(NULL);  /* Terminate if the address is invalid */
    }

    /* Mutex device */
    SYSCALL(PASSEREN, (memaddr)&support_device_sems[1][flashNum], 0, 0);

    /* Set up the buffer location based on the flash device number */
    buffer = (memaddr *) FLASHPOOLSTART + (flashNum * PAGESIZE);

    /* Perform the flash operation (read) */
    device_status = flashOp(flashNum, sector, (memaddr) buffer, FLASHREAD);

    /* If the device is ready, copy data from the buffer to the virtual address */
    if (device_status == READY) {
        int i;
        for (i = 0; i < PAGESIZE / WORDLEN; i++) {
            *virtualAddr++ = *buffer++;  /* Copy data word by word */
        }
    }

    /* Release semaphore for the flash device */
    SYSCALL(VERHOGEN, (memaddr)&support_device_sems[1][flashNum], 0, 0);

    /* Set the return value in the exception state */
    current_support->sup_exceptState[GENERALEXCEPT].s_v0 = device_status;
}

/* 
 * Writes data from a virtual memory address to a specified 
 * flash device.
 */
void flashPut(support_t *current_support) {
    int flashNum, sector, device_status;  /* Flash device number, sector number, and device status */
    memaddr *buffer;                       /* Pointer for buffer in memory */
    memaddr *virtualAddr;                  /* Pointer for the virtual address to read data */

    /* Get parameters from the exception state */
    virtualAddr = (memaddr *) current_support->sup_exceptState[GENERALEXCEPT].s_a1;
    flashNum = current_support->sup_exceptState[GENERALEXCEPT].s_a2;
    sector = current_support->sup_exceptState[GENERALEXCEPT].s_a3;

    /* Check if virtual address is valid */
    if ((int) virtualAddr < KUSEG) {
        terminate(NULL);  /* Terminate if the address is invalid */
    }

    /* Acquire mutex with semaphore for the flash device */
    SYSCALL(PASSEREN, (memaddr)&support_device_sems[1][flashNum], 0, 0);

    /* Set up the buffer location based on the flash device number */
    buffer = (memaddr *) FLASHPOOLSTART + (flashNum * PAGESIZE);

    /* Copy data from the virtual address to the buffer */
    int i;
    for (i = 0; i < PAGESIZE / WORDLEN; i++) {
        *buffer++ = *virtualAddr++;  /* Copy data word by word */
    }

    /* Perform the flash operation (write) */
    device_status = flashOp(flashNum, sector, (memaddr)buffer, FLASHWRITE);

    /* Release semaphore for the flash device */
    SYSCALL(VERHOGEN, (memaddr)&support_device_sems[1][flashNum], 0, 0);

    /* Set the return value in the exception state */
    current_support->sup_exceptState[GENERALEXCEPT].s_v0 = device_status;
}

/* 
 * Executes a read or write operation on a specified flash device.
 * 
 * Returns:
 *   Device status indicating success or failure.
 */
int flashOp(int flashNum, int sector, int buffer, int operation) {
    unsigned int command;   /* Command to send to the device */
    device_t *flashDevice;  /* Pointer to the flash device registers */
    unsigned int maxBlock;  /* Maximum block for the flash device */

    /* Get the pointer to the flash device based on its number */
    flashDevice = (device_t *) (DEVICEREGSTART + ((FLASHINT - DISKINT) * (DEVICE_INSTANCES * DEVREGSIZE)) + (flashNum * DEVREGSIZE));
    maxBlock = flashDevice->d_data1;  /* Get the max block number from the device */

    /* Check if the requested sector is valid */
    if (sector >= maxBlock) {
        terminate(NULL);  /* Terminate if the sector is out of range */
    }

    /* Prepare the command based on the operation type */
    if (operation == FLASHREAD) {
        command = FLASHREAD | (sector << FLASHADDRSHIFT);  /* Set read command */
    } else if (operation == FLASHWRITE) {
        command = FLASHWRITE | (sector << FLASHADDRSHIFT);  /* Set write command */
    } else {
        return -1;  /* Invalid operation */
    }

    /* Set the buffer address in the device register */
    flashDevice->d_data0 = buffer;

    /* Disable interrupts for the operation */
    /* Atomically set the command and call wait IO */
    setSTATUS(INTSOFF);
    flashDevice->d_command = command;  /* Send the command to the device */
    
    /* Wait for the I/O operation to complete */
    unsigned int deviceStatus = SYSCALL(WAITIO, FLASHINT, flashNum, 0);
    
    /* Re-enable interrupts */
    setSTATUS(INTSON);

    return deviceStatus;  /* Return the device status */
}

/* 
 * Executes a read or write operation on a specified disk device.
 * Returns:
 *   Device status indicating success or failure.
 */
int diskOp(int diskNum, int track, int head, int sector, int buffer, int operation) {
    unsigned int command;             /* Command to send to the device */
    int device_status;                /* Status of the device operation */
    devregarea_t *diskDevice;        /* Pointer to the disk device registers */

    /* Get pointer to the disk device registers */
    diskDevice = (devregarea_t *) RAMBASEADDR; 

    /* Disable interrupts for the operation (Atomic) */
    setSTATUS(INTSOFF);
    
    /* Prepare command for seeking cylinder */
    command = (track << 8) | SEEKCYLINDER; 
    diskDevice->devreg[0][diskNum].d_command = command;  /* Send command to the device */
    
    /* Wait for the I/O operation to complete */
    device_status = SYSCALL(WAITIO, DISKINT, diskNum, 0);
    setSTATUS(INTSON);  /* Re-enable interrupts */

    /* Check if the device is ready after seeking */
    if(device_status != READY) {
        device_status = -(device_status);  /* Invert status for error indication */
    } else {
        /* Proceed with the read/write operation */
        setSTATUS(INTSOFF);
        diskDevice->devreg[0][diskNum].d_data0  = buffer;  /* Set the buffer address */
        
        /* Prepare command for the actual read/write operation */
        command = (head << 16) | (sector << 8) | operation; 
        diskDevice->devreg[0][diskNum].d_command = command;  /* Send command to the device */
        
        /* Wait for the I/O operation to complete */
        device_status = SYSCALL(WAITIO, DISKINT, diskNum, 0);
        setSTATUS(INTSON);  /* Re-enable interrupts */

        /* Check if the device is ready after operation */
        if(device_status != READY) {
            device_status = -(device_status);  /* Invert status for error indication */
        }
    }

    return device_status;  /* Return the device status */
}

/* 
 * Writes data from a virtual memory address to a specified 
 * disk device.
 */
void diskPut(support_t *current_support) {
    int maxPlatter, maxSector, maxCylinder, diskPhysicalGeometry, maxCount;  /* Disk geometry parameters */
    int seekCylinder, platterNum, device_status; /* Parameters for seeking and status */
    int diskNum, sectorNum;                       /* Disk device number and sector number */
    memaddr *buffer;                              /* Pointer for buffer in memory */
    memaddr *virtualAddr;                         /* Pointer for the virtual address to read data */
    devregarea_t *devReg;                        /* Pointer to device register area */

    /* Get pointer to the device registers */
    devReg = (devregarea_t *) RAMBASEADDR; 

    /* Get parameters from the exception state */
    virtualAddr = (memaddr *) current_support->sup_exceptState[GENERALEXCEPT].s_a1;
    diskNum = current_support->sup_exceptState[GENERALEXCEPT].s_a2;
    sectorNum = current_support->sup_exceptState[GENERALEXCEPT].s_a3;

    /* Get the physical geometry of the disk */
    diskPhysicalGeometry = devReg->devreg[0][diskNum].d_data1;

    /* Calculate maximum counts for the geometry */
    maxCylinder = (diskPhysicalGeometry >> 16);
    maxPlatter = (diskPhysicalGeometry & 0x0000FF00) >> 8;
    maxSector = (diskPhysicalGeometry & 0x000000FF);
    maxCount = maxCylinder * maxPlatter * maxSector;

    /* Check if the virtual address and sector number are valid */
    if(((int) virtualAddr < KUSEG) || (sectorNum > maxCount)) {
        terminate(NULL);  /* Terminate if the address or sector is invalid */
    }

    /* Calculate the seek parameters based on the sector number */
    seekCylinder = sectorNum / (maxPlatter * maxSector);
    sectorNum = sectorNum % (maxPlatter * maxSector);
    platterNum = sectorNum / maxSector;
    sectorNum = sectorNum % maxSector;

    /* Acquire semaphore for the disk device */
    SYSCALL(PASSEREN, (memaddr)&support_device_sems[0][diskNum], 0, 0);
    
    /* Set up the buffer location based on the disk device number */
    buffer = (memaddr *)(DISKPOOLSTART + (diskNum * PAGESIZE));
    memaddr *originBuff = (DISKPOOLSTART + (diskNum * PAGESIZE));
    
    /* Copy data from the virtual address to the buffer */
    int i;
    for (i = 0; i < PAGESIZE / WORDLEN; i++) {
        *buffer++ = *virtualAddr++;  /* Copy data word by word */
    }

    /* Perform the disk operation (write) */
    device_status = diskOp(diskNum, seekCylinder, platterNum, sectorNum, originBuff, DISKWRITE);

    /* Release semaphore for the disk device */
    SYSCALL(VERHOGEN, (memaddr)&support_device_sems[0][diskNum], 0, 0);

    /* Set the return value in the exception state */
    current_support->sup_exceptState[GENERALEXCEPT].s_v0 = device_status;
}

/* 
 * Reads data from a specified disk device and writes it to a 
 * virtual memory address.
 */
void diskGet(support_t *current_support) {
    int maxPlatter, maxSector, maxCylinder, diskPhysicalGeometry, maxCount;  /* Disk geometry parameters */
    int seekCylinder, platterNum, device_status; /* Parameters for seeking and status */
    int diskNum, sectorNum;                       /* Disk device number and sector number */
    memaddr *buffer;                              /* Pointer for buffer in memory */
    memaddr *virtualAddr;                         /* Pointer for the virtual address to read data */
    devregarea_t *devReg;                        /* Pointer to device register area */

    /* Get pointer to the device registers */
    devReg = (devregarea_t *) RAMBASEADDR; 

    /* Get parameters from the exception state */
    virtualAddr = (memaddr *) current_support->sup_exceptState[GENERALEXCEPT].s_a1;
    diskNum = current_support->sup_exceptState[GENERALEXCEPT].s_a2;
    sectorNum = current_support->sup_exceptState[GENERALEXCEPT].s_a3;
    
    /* Get the physical geometry of the disk */
    diskPhysicalGeometry = devReg->devreg[0][diskNum].d_data1;

    /* Calculate maximum counts for the geometry */
    maxCylinder = (diskPhysicalGeometry >> 16);
    maxPlatter = (diskPhysicalGeometry & 0x0000FF00) >> 8;
    maxSector = (diskPhysicalGeometry & 0x000000FF);
    maxCount = maxCylinder * maxPlatter * maxSector;

    /* Check if the virtual address and sector number are valid */
    if(((int) virtualAddr < KUSEG) || (sectorNum > maxCount)) {
        terminate(NULL);  /* Terminate if the address or sector is invalid */
    }

    /* Calculate the seek parameters based on the sector number */
    seekCylinder = sectorNum / (maxPlatter * maxSector);
    sectorNum = sectorNum % (maxPlatter * maxSector);
    platterNum = sectorNum / maxSector;
    sectorNum = sectorNum % maxSector;

    /* Acquire semaphore for the disk device */
    SYSCALL(PASSEREN, (memaddr)&support_device_sems[0][diskNum], 0, 0);

    /* Set up the buffer location based on the disk device number */
    buffer = (memaddr *)(DISKPOOLSTART + (diskNum * PAGESIZE));
    memaddr *originBuff = (DISKPOOLSTART + (diskNum * PAGESIZE));

    /* Perform the disk operation (read) */
    device_status = diskOp(diskNum, seekCylinder, platterNum, sectorNum, originBuff, DISKREAD);

    /* If the operation is ready, copy data from the buffer to the virtual address */
    if(device_status == READY) {
        int i;
        for (i = 0; i < PAGESIZE / WORDLEN; i++) {
            *virtualAddr++ = *buffer++;  /* Copy data word by word */
        }
    }

    /* Release semaphore for the disk device */
    SYSCALL(VERHOGEN, (memaddr)&support_device_sems[0][diskNum], 0, 0);

    /* Set the return value in the exception state */
    current_support->sup_exceptState[GENERALEXCEPT].s_v0 = device_status;
}
