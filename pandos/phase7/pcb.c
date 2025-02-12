/*
 * Date: 18/7/2024
 * Authors: Haruka Yamamoto & James Rushworth
 *
 * Module implements the functions in the external
 * declaration file pcb.h.
 *
 * This module is used for the Process Control Block
 * 
 * The Process Control Block (PCB) is a struct defined in
 * the included types.h file. This includes variables for
 * next and previous PCBs and child PCBs.
 * 
 * The PCBs are arranged and altered by functions which
 * remove, insert, and initialise the various PCB data structures.
 * 
 * This module utilises a double linked stack for the PCB free list.
 * The process queue is a double circularly linked list.
 * The process tree for child PCBs is linearly linked and NULL terminated.
 *  
 */


#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/const.h"

/* Global variable of free list head (limited to this file)*/
HIDDEN pcb_t pcbFree_table[MAXPROC];

HIDDEN pcb_PTR pcbFree_h;

/* Initialises the MAXPROC PCBs */
void initPcbs() {
    pcbFree_h = &(pcbFree_table[0]); /* Array with maximum number of processes */

    /* Adds each PCB to free PCB list (stack) */
    int i;
    for (i = 0; i < MAXPROC - 1; ++i) {
        pcbFree_table[i].p_next = &(pcbFree_table[i + 1]);
    }
    /* Initially had the tail and head connected in a circular buffer
       But a stack is all that is needed as order of access is not important */
    pcbFree_table[MAXPROC - 1].p_next = NULL;

}

/* Puts PCB back into free list */
void freePcb(pcb_PTR p) {
   p->p_next = pcbFree_h;
   pcbFree_h = p; 
}

/* Removes PCB from free list for allocation */
pcb_PTR allocPcb() {
    pcb_PTR head = NULL;
    if (pcbFree_h != NULL){
        head = pcbFree_h;
        pcbFree_h = head->p_next;
        
        head->p_next = NULL;
        head->p_prev = NULL;
        head->p_prnt = NULL;
        head->p_child = NULL;
        head->p_next_sib = NULL;
        head->p_prev_sib = NULL;
        head->p_semAdd = NULL;
        head->p_time = 0;
        
        head->p_s.s_cause = 0;
        head->p_s.s_entryHI = 0;
        int i;
        for (i = 0; i < STATEREGNUM; ++i) {
            head->p_s.s_reg[i] = 0;
        }
        head->p_s.s_entryHI = 0;
        head->p_s.s_pc = 0;
        head->p_s.s_status = 0;

        head->p_supportStruct = NULL;
    }
    return head;
}

/* This method is used to initialise a variable to be tail pointer to a
   process queue.
   Return a pointer to the tail of an empty process queue; i.e. NULL. */
pcb_PTR mkEmptyProcQ() {
    return NULL;
}

/* Return TRUE if the queue whose tail is pointed to by tp is empty.
   Return FALSE otherwise. */
int emptyProcQ(pcb_PTR tp) {
    return tp == NULL;
}

/* Return a pointer to the first pcb from the process queue whose tail
   is pointed to by tp. Do not remove this pcbfrom the process queue.
   Return NULL if the process queue is empty. */
pcb_PTR headProcQ(pcb_PTR tp){
    if (tp == NULL) return NULL;

    return tp->p_prev;
}

/* Remove the pcb pointed to by p from the process queue whose tail-
   pointer is pointed to by tp. Update the process queue’s tail pointer if
   necessary. If the desired entry is not in the indicated queue (an error
   condition), return NULL; otherwise, return p. Note that p can point
   to any element of the process queue. */
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){
    if (*tp == NULL) return NULL;

    pcb_PTR toRemove = *tp;
    int found = FALSE;
    do {
        if (toRemove == p) {
            found = TRUE;
            break;
        }

        toRemove = toRemove->p_next;

    } while (toRemove != *tp);

    if (!found) return NULL;

    if (toRemove->p_next == toRemove) *tp = NULL;
    else {

        if (toRemove == *tp) *tp = (*tp)->p_next;

        toRemove->p_prev->p_next = toRemove->p_next;
        toRemove->p_next->p_prev = toRemove->p_prev;
    }

    toRemove->p_next = NULL;
    toRemove->p_prev = NULL;

    return toRemove;
}

/* Insert the pcb pointed to by p into the process queue whose tail-
   pointer is pointed to by tp. Note the double indirection through tp
   to allow for the possible updating of the tail pointer as well. */
void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
    if (*tp == NULL) {
        *tp = p;
        p->p_prev = p;
        p->p_next = p;
        return;
    }

    p->p_next = *tp;
    p->p_prev = (*tp)->p_prev;
    p->p_prev->p_next = p;
    (*tp)->p_prev = p;
    *tp = p;
}

/* Remove the first (i.e. head) element from the process queue whose
   tail-pointer is pointed to by tp. Return NULL if the process queue
   was initially empty; otherwise return the pointer to the removed ele-
   ment. Update the process queue’s tail pointer if necessary. */
pcb_PTR removeProcQ(pcb_PTR *tp) {
    if(*tp == NULL) return NULL;

    pcb_PTR toRemove = (*tp)->p_prev;

    if (toRemove->p_prev == toRemove) *tp = NULL;
    else {
        (*tp)->p_prev = toRemove->p_prev;
        toRemove->p_prev->p_next = *tp;
    }

    toRemove->p_next = NULL;
    toRemove->p_prev = NULL;

    return toRemove;
}

/* Return TRUE if the pcb pointed to by p has no children. Return
   FALSE otherwise. */
int emptyChild(pcb_PTR p) {
    return p->p_child == NULL;
}

/* Make the pcb pointed to by p a child of the pcb pointed to by prnt */
void insertChild(pcb_PTR prnt, pcb_PTR p) {

    if (prnt->p_child != NULL) {
        prnt->p_child->p_prev_sib = p;
    }

    p->p_prnt = prnt;
    p->p_next_sib = prnt->p_child;
    p->p_prev_sib = NULL;
    prnt->p_child = p;
}

/* Make the first child of the pcb pointed to by p no longer a child of
   p. Return NULL if initially there were no children of p. Otherwise,
   return a pointer to this removed first child pcb. */
pcb_PTR removeChild(pcb_PTR p) {
    if (p->p_child == NULL) {
        return NULL;
    }
    else {
        pcb_PTR child = p->p_child;
        p->p_child = child->p_next_sib;

        if (child->p_next_sib != NULL){
            child->p_next_sib->p_prev_sib = NULL;
        }

        child->p_prnt = NULL;
        child->p_next_sib = NULL;

        return child;
    }
}

/* Make the pcb pointed to by p no longer the child of its parent. If
   the pcb pointed to by p has no parent, return NULL; otherwise, return
   p. Note that the element pointed to by p need not be the first child of
   its parent. */
pcb_PTR outChild(pcb_PTR p) {
    if (p->p_prnt == NULL) {
        return NULL;
    }

    if (p->p_prev_sib != NULL) {
        p->p_prev_sib->p_next_sib = p->p_next_sib;
    }
    
    if (p->p_next_sib != NULL) {
        p->p_next_sib->p_prev_sib = p->p_prev_sib;
    }

    if (p->p_prnt->p_child == p) {
        p->p_prnt->p_child = p->p_next_sib;
    }

    p->p_prev_sib = NULL;
    p->p_next_sib = NULL;
    p->p_prnt = NULL;

    return p;
}