#ifndef SHIP_STRUCT
#define SHIP_STRUCT

typedef struct Ship
{
   int num;
   int timestamp;
   int dmg;
   struct Ship* next;
}Ship;

#endif
