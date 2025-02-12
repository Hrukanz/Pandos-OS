/*
 * Date: 26/7/2024
 * Authors: Haruka Yamamoto & James Rushworth
 *
 * Module implements the functions in the external
 * declaration file asl.h.
 *
 * This module is used for the Active Semaphore List
 * 
 * Semaphores are used to protect/coordinate a resource between
 * multiple accessors. For example, protecting a critical region
 * in memory which is being accessed by multiple processes or threads.
 * 
 * This module has lists for semaphores which are currently active and
 * semaphores which are free (unallocated). Semaphores are active when
 * at least one pcb is associated with it.
 * 
 * The lists are modified with functions to insert or remove semaphores
 * from the two lists.
 *
 */

#include "../h/types.h"
#include "../h/asl.h"
#include "../h/pcb.h"

/* Global variables limited to this file.
   Heads to lists containing active semaphore descriptors and free semaphore's */
#define MAXSEMD (MAXPROC + 2)

HIDDEN semd_t semd_table[MAXSEMD];

HIDDEN semd_t *semdFree_h;

HIDDEN semd_t *semd_h;

/* Helper function that returns the previous element
   Allows for operations to be preformed on the element using s_next */
HIDDEN semd_t* findPrevSemd(int *semAdd) {
    semd_t *prev = semd_h;
    while (semAdd > prev->s_next->s_semAdd) {
        prev = prev->s_next;
    }

    return prev;
}

/* Insert the PCB pointed to by p into the process queue of the semaphore with address semAdd.
   If the semaphore is inactive, allocate a new descriptor and initialise it.
   Return TRUE if the semdFree list is empty, otherwise return FALSE. */
int insertBlocked(int *semAdd, pcb_PTR p) {
    semd_t *prev = findPrevSemd(semAdd);
    semd_t *element = prev->s_next;

    if (element->s_semAdd == semAdd) {
        p->p_semAdd = semAdd;
        insertProcQ(&(element->s_procQ), p);
    }
    else {
        if (semdFree_h == NULL){
            return TRUE;
        }

        semd_t *newSemd = semdFree_h;
        semdFree_h = semdFree_h->s_next;

        prev->s_next = newSemd;
        newSemd->s_next = element;
        newSemd->s_semAdd = semAdd;
        newSemd->s_procQ = mkEmptyProcQ();
        p->p_semAdd = semAdd;

        insertProcQ(&(newSemd->s_procQ), p);
    }

    return FALSE;
}


/* Search the ASL for the semaphore with address semAdd.
   Remove and return the first PCB from its process queue.
   If the queue becomes empty, remove the semaphore descriptor from the ASL.
   Return NULL if the semaphore is not found. */
pcb_PTR removeBlocked(int *semAdd) {
    semd_t *prev = findPrevSemd(semAdd);
    semd_t *element = prev->s_next;

    if (element->s_semAdd == semAdd){
        pcb_t *first = removeProcQ(&(element->s_procQ));
        if (emptyProcQ(element->s_procQ)){

            prev->s_next = element->s_next;
            element->s_next = semdFree_h;
            semdFree_h = element;
        }

        return first;
    }

    return NULL;
}

/* Remove the PCB pointed to by p from the process queue of its semaphore.
   Return NULL if the PCB is not found, otherwise return the PCB. */
pcb_PTR outBlocked(pcb_PTR p) {
    if (p == NULL) return NULL;

    semd_t *prev = findPrevSemd(p->p_semAdd);
    semd_t *element = prev->s_next;

    if(element->s_semAdd != p->p_semAdd) return NULL;

    pcb_PTR removed = outProcQ(&(element->s_procQ), p);

    if (emptyProcQ(element->s_procQ)) {
        prev->s_next = element->s_next;
        element->s_next = semdFree_h;
        semdFree_h = element;
    }

    return removed;
}

/* Return the first PCB from the process queue of the semaphore with address semAdd.
   Return NULL if the semaphore is not found or the queue is empty. */
pcb_PTR headBlocked(int *semAdd) {
    semd_t *prev = findPrevSemd(semAdd);
    semd_t *element = prev->s_next;


    if (element->s_semAdd == semAdd){
        return headProcQ(element->s_procQ);
    }
    return NULL;
}

/* Initialise the semdFree list to contain all the elements of semdTable. */
void initASL() {
    semdFree_h = &(semd_table[2]);

    int i;
    for (i = 2; i < MAXSEMD - 1; ++i) {
        semd_table[i].s_next = &(semd_table[i + 1]);
    }
    semd_table[MAXSEMD - 1].s_next = NULL;

    semd_h = &semd_table[0];
    semd_h->s_semAdd = MINPOINT;
    semd_h->s_next = &semd_table[1];
    
    semd_table[1].s_next = NULL;
    semd_table[1].s_semAdd = MAXPOINT;
}
