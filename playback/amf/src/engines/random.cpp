#include <stdlib.h>
#include <sys/time.h>
#include "random.h"

//TODO, improve random implementation
void our_srandom(AM_U32 x)
{
    srand(x);
}

AM_U32 our_random()
{
    AM_INT random1 = rand();
    AM_INT random2 = rand();
    AM_U32 myrand = (random1 & 0xffff);
    return ((myrand << 16) & 0xffff0000) + (random2 & 0xffff);
}

AM_U32 our_random32() 
{
    // Return a 32-bit random number.
    // Because "our_random()" returns a 31-bit random number, we call it a second
    // time, to generate the high bit:
    AM_U32 random1 = our_random();
    AM_U32 random2 = our_random();
    return (AM_U32)((random2<<31) | random1);
}

