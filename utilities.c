#include <unistd.h>
#include "utilities.h"

int max(int a, int b)
{
   return a > b ? a : b;
}

void fightSpaceBears(useconds_t microseconds)
{
   usleep(microseconds);
}
