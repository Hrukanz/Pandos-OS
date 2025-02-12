#ifndef INITPROC_H
#define INITPROC_H

/*
 * initProc.h
 * Authors: Haruka Yamamoto & James Rushworth
 * 
 */


#include "types.h"

extern int testSem;
extern void test(void);

support_t* allocate_sup();
void initSupport();
void dealocate_sup(support_t *support);
 
#endif