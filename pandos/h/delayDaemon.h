#ifndef DELAYDAEMON_H
#define DELAYDAEMON_H

#include "../h/types.h"
#include "../h/const.h"

extern int delaySemaphore;
extern delay_PTR delaydFree;
extern delay_PTR delaydFree_h;

void initADL();
void delayCurrentProc(support_t *current_support);
int insertDelayNode(support_t *current_support, int sleepTime);
void delayDaemon();
delay_PTR activateASL();
void freeNode(delay_PTR delayEvent);

#endif /* DELAYDAEMON_H */