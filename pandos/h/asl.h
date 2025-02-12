#ifndef ASL_H
#define ASL_H

/************************** ASL.H ******************************
*
*  The externals declaration file for the Active Semaphore List
*    Module.
*
*  Written by Mikeyg
*/

#include "types.h"

int insertBlocked(int *semAdd, pcb_PTR p);
pcb_PTR removeBlocked(int *semAdd);
pcb_PTR outBlocked(pcb_PTR p);
pcb_PTR headBlocked(int *semAdd);
void initASL();

#endif