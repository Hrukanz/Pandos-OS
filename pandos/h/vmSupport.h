#ifndef VMSUPPORT_H
#define VMSUPPORT_H

/*
 * vmSupport.h
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 * This header file provides the declarations for the virtual memory support functions
 * and structures used in vmSupport.c.
 */

#include "const.h"
#include "types.h"

extern swap_t swapPool[POOLSIZE];

void updateTLB(pteEntry_t *updatedEntry);
void initSwapPool();
void pager();

#endif