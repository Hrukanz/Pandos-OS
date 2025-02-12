
#include "../h/debug.h"

/* Debugging function can be used with break points to read
   a0, a1, a2, and a3 registers which hold the four parameters 
   to function inputs. These can be used to show states of varibles.
   Variables a, b, c, and d can be used to look at variables in program
   when running. */
void debug (int a, int b, int c, int d)
{
    /* Contents are not important, just here for compiling */
    int i = 42;
    i++;
}