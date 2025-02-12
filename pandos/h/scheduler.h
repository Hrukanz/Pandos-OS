#ifndef SCHEDULER_H
#define SCHEDULER_H

extern volatile cpu_t timeSlice;   

void scheduler();

#endif