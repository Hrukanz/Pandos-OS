#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/*
 * Date: 31/7/2024
 * Authors: Haruka Yamamoto & James Rushworth
 */

#define GETIP   0x0000FE00
#define IPSHIFT 8
#define TERMSTATUSMASK 0x000000FF

#define DEVREGADDR ((devregarea_t *)RAMBASEADDR)

/* 
 * Device/Timer Interrupt Exception Handler
 * Handles all device and timer interrupts, converting them into V operations
 * on the appropriate semaphores.
 */
void intExceptionHandler(state_t *exceptionState);

#endif