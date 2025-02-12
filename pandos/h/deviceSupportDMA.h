#ifndef DEVICESUPPORTDMA_H
#define DEVICESUPPORTDMA_H

#include "../h/types.h"
#include "../h/const.h"

/* Function prototypes */
void flashGet(support_t *current_support);
void flashPut(support_t *current_support);
int flashOp(int flashNum, int sector, int buffer, int operation);
int diskOp(int diskNum, int track, int head, int sector, int buffer, int operation);
void diskPut(support_t *current_support);
void diskGet(support_t *current_support);

#endif /* DEVICESUPPORTDMA_H */