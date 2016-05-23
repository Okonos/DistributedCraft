#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h> // EBUSY
#include <mpi.h>
#include <pthread.h>
#include <glib.h>
#include "list.h"
#include "utilities.h"
// #include "ship.h"

#define MAX_DATA 2
#define SLEEP_INTERVAL 1000

int myNumber;
int SHIPS;
int DOCKS;
int SCVs;
List lowerPriorityShips = NULL;
List higherPriorityShips = NULL;
List neutralShips = NULL;
int _time = 0;
int timestamp;

typedef enum {REQUEST, ACK, TERMINATE} msg_type;

pthread_mutex_t quitMutex = PTHREAD_MUTEX_INITIALIZER; // used as quit flag

bool isMyPriorityHigher(Ship *s)
{
   if(timestamp == s->timestamp)
      return myNumber < s->number;
   return timestamp < s->timestamp;
}

void agreeToPendingRequests()
{
   List node = lowerPriorityShips;
   while(node != NULL)
   {
      _time++;
      Ship *s = ((Ship *) node->data);
      int data[2] = {s->timestamp, s->dmg};
      MPI_Send(data, 2, MPI_INT, s->number, ACK, MPI_COMM_WORLD);
      higherPriorityShips = list_append(higherPriorityShips, s);
      node = list_next(node);
   }
   lowerPriorityShips = list_free(lowerPriorityShips);
}

void checkPermission(Ship *ship)
{
   if(!isMyPriorityHigher(ship))
   {
      _time++;
      int data[2] = {ship->timestamp, ship->dmg};
      MPI_Send(data, 2, MPI_INT, ship->number, ACK, MPI_COMM_WORLD);
      higherPriorityShips = list_append(higherPriorityShips, ship);
   }
   else
   {
      neutralShips = list_remove_soft(neutralShips, ship->number);
      lowerPriorityShips = list_append(lowerPriorityShips, ship);
   }
}

/* Returns 1 (true) if the mutex is unlocked, which is the
 * thread's signal to terminate.
 */
bool needQuit()
{
  switch(pthread_mutex_trylock(&quitMutex))
  {
    case 0: /* if we got the lock, unlock and return true */
      pthread_mutex_unlock(&quitMutex);
      return true;
    case EBUSY: /* return false if the mutex was locked */
      return false;
  }
  return true;
}

int countDamage()
{
   List node = higherPriorityShips;
   int totalDmg = 0;
   while(node != NULL)
   {
      totalDmg += ((Ship *) node->data)->dmg;
      node = list_next(node);
   }
   return totalDmg;
}

void cleanup()
{
   printf("Cleaning\n");
   neutralShips = list_free(neutralShips);
   lowerPriorityShips = list_free(lowerPriorityShips);
   higherPriorityShips = list_free(higherPriorityShips);
}

void *terminationListener()
{
   char ch;
   printf("Type 'q' to terminate\n");
   while(true)
   {
      ch = getchar();
      if(ch == 'q')
      {
         pthread_mutex_unlock(&quitMutex);
         printf("Terminating program\n");
         return NULL;
      }
   }
}

int main(int argc, char* argv[])
{
   int i;
   int provided;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   // printf("%d\n", provided);

   // TODO: broadcast do innych?(chyba nie trzeba)
   DOCKS = atoi(argv[1]);
   SCVs = atoi(argv[2]);

   MPI_Comm_rank (MPI_COMM_WORLD, &myNumber);        /* get current process id */
   MPI_Comm_size (MPI_COMM_WORLD, &SHIPS);        /* get number of processes */
   printf( "Battlecruiser number %d of %d reporting\n", myNumber, SHIPS );

   pthread_mutex_lock(&quitMutex);
   if(myNumber == 0)
   {
      pthread_t pth; // wątek do terminacji programu, zbędny w procesach innych niż root
      pthread_create(&pth, NULL, &terminationListener, NULL);
   }

   srand(time(NULL));
   bool requesting = false;
   int fightTime = 0;
   MPI_Request request = MPI_REQUEST_NULL;
   MPI_Status status;
   int flag = -1;
   int data[MAX_DATA];
   int dmgRcvd = 0;

   while( !needQuit() )
   {
      if(fightTime == 0 && !requesting)
         fightTime = ((rand() % 10) + 1) * 1000000; // microseconds

      if(fightTime > 0)
      {
         fightSpaceBears(SLEEP_INTERVAL);
         fightTime -= SLEEP_INTERVAL;
      }

      if(fightTime == 0 && !requesting)
      {
         dmgRcvd = (rand() % SCVs) + 1;
         printf("%d: Received %d dmg\n", myNumber, dmgRcvd);
         _time++;
         timestamp = _time;
         for(i=0; i<SHIPS; i++)
            if(i != myNumber)
            {
               int data[2] = {timestamp, dmgRcvd};
               // TODO: POWINIEN BYĆ CHYBA NIEBLOKUJĄCY
               MPI_Send(data, 2, MPI_INT, i, REQUEST, MPI_COMM_WORLD);
            }

         printf("%d: Awaiting dock and %d repairs\n", myNumber, dmgRcvd);
         requesting = true;
      }


      // RECEIVE
      if(flag != 0)
      {
         MPI_Irecv(data, MAX_DATA, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
         flag = 0;
      }
      MPI_Test(&request, &flag, &status);
      if(flag != 0)
      {
         // zawsze wysyłany timestamp i dmgRcvd
         int senderNumber = status.MPI_SOURCE;
         int senderTimestamp = data[0];
         int senderDmgRcvd = data[1];
         switch (status.MPI_TAG)
         {
            case REQUEST:
            {
               Ship *s = malloc(sizeof(Ship));
               s->timestamp = senderTimestamp;
               s->number = senderNumber;
               s->dmg = senderDmgRcvd;
               checkPermission(s);
               _time = max(_time, senderTimestamp) + 1;
               break;
            }

            case ACK:
            {
               // DONE: przesyłanie dmgRcvd
               // (dodajemy do neutrali, później może trafić do higherPriorityShips)
               higherPriorityShips = list_remove_hard(higherPriorityShips, senderNumber);
               if(senderTimestamp != timestamp)
                  break; // old, outdated ACK

               if( list_find(neutralShips, senderNumber) == NULL \
               && list_find(lowerPriorityShips, senderNumber) == NULL )
               {
                  Ship *s = malloc(sizeof(Ship));
                  s->number = senderNumber;
                  s->timestamp = senderTimestamp;
                  s->dmg = senderDmgRcvd;
                  list_append(neutralShips, s);
               }
               _time = max(_time, senderTimestamp) + 1;
               break;
            }

            case TERMINATE:
            {
               // 'terminate' broadcast
               for(i=0; i<SHIPS; i++)
                  if(i != myNumber)
                     MPI_Send(0, 0, MPI_INT, i, TERMINATE, MPI_COMM_WORLD);

               pthread_mutex_unlock(&quitMutex); // alternatywa -- goto
               requesting = false; // coby pominać poniższe
               break;
            }
         }
      }


      // CRITICAL SECTION ENTRY
      if(requesting)
      {
         int hpShips = list_length(higherPriorityShips);
         if( (SHIPS - 1) == list_length(neutralShips) + list_length(lowerPriorityShips) + hpShips )
            if( hpShips <= (DOCKS - 1) && countDamage() <= (SCVs - dmgRcvd) )
            {
               // CRITICAL SECTION: dock and repair
               agreeToPendingRequests();
               neutralShips = list_free(neutralShips);
               requesting = false; // gdzieś tutaj na końcu
               printf("%d: Battlecruiser operational\n", myNumber);
            }
      }
   }

   cleanup();
   printf("Thread %d terminated\n", myNumber);

   MPI_Finalize();
   return 0;
}
