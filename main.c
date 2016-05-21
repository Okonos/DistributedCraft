#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h> // sleep
#include <errno.h> // EBUSY
#include <mpi.h>
#include <pthread.h>
#include <glib.h>
#include "list.h"
#include "utilities.h"
// #include "ship.h"

#define MAX_DATA 2

int myNumber;
int SHIPS;
int DOCKS;
int SCVs;
List lowerPriorityShips = NULL;
List higherPriorityShips = NULL;
List neutralShips = NULL;
int _time = 0;
int timestamp;

typedef enum {REQUEST, ACK} msg_type;

pthread_mutex_t quitMutex = PTHREAD_MUTEX_INITIALIZER; // used as quit flag
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
      int data[2] = {s->timestamp, s->dmgRcvd};
      MPI_Send(data, 2, MPI_INT, s->number, ACK, MPI_COMM_WORLD);

      // można też kolejno usuwać, ale funkcja i tak jest w muteksie,
      // więc to raczej nie ma większego znaczenia
      // lowerPriorityShips = list_remove(lowerPriorityShips, s);

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
      int data[2] = {ship->timestamp, ship->dmgRcvd};
      MPI_Send(data, 2, MPI_INT, ship->number, ACK, MPI_COMM_WORLD);
      higherPriorityShips = list_append(higherPriorityShips, ship);
   }
   else
   {
      lowerPriorityShips = list_append(lowerPriorityShips, ship);
      neutralShips = list_remove(neutralShips, ship->number);
   }
}

/* Returns 1 (true) if the mutex is unlocked, which is the
 * thread's signal to terminate.
 */
bool needQuit()
{
  switch(pthread_mutex_trylock(&quitMutex))
  {
    case false: /* false = 0; if we got the lock, unlock and return true */
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

void *mainThread()
{
   srand(time(NULL));
   while( !needQuit() )
   {
      fightSpaceBears( (rand() % 10) + 1 );
      int dmgRcvd = (rand() % SCVs) + 1;

      pthread_mutex_lock(&mutex);
      _time++;
      timestamp = _time;
      pthread_mutex_unlock(&mutex);
      int i;
      for(i=0; i<SHIPS; i++)
         if(i != myNumber)
         {
            int data[2] = {timestamp, dmgRcvd};
            // TODO: POWINIEN BYĆ NIEBLOKUJĄCY, na razie chyba nie jest
            MPI_Send(data, 2, MPI_INT, i, REQUEST, MPI_COMM_WORLD);
         }

      printf("%d: Awaiting dock and %d repairs\n", myNumber, dmgRcvd);
      while( !needQuit() )
      {
         int hpShips = list_length(higherPriorityShips);
         if( (SHIPS - 1) == list_length(neutralShips) + list_length(lowerPriorityShips) + hpShips )
            if( hpShips <= (DOCKS - 1) && countDamage() <= (SCVs - dmgRcvd) )
               break;
      }

      // CRITICAL SECTION: dock and repair
      pthread_mutex_lock(&mutex);
      agreeToPendingRequests();
      neutralShips = list_free(neutralShips);
      pthread_mutex_unlock(&mutex);
      printf("%d: Battlecruiser operational\n", myNumber);
   }

   return NULL;
}

void *communicationThread()
{
   while( !needQuit() )
   {
      MPI_Status status;
      int data[MAX_DATA];
      MPI_Recv(data, MAX_DATA, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
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
            pthread_mutex_lock(&mutex);
            checkPermission(s);
            _time = max(_time, senderTimestamp) + 1;
            pthread_mutex_unlock(&mutex);
            break;
         }

         case ACK:
         {
            pthread_mutex_lock(&mutex);
            // DONE: przesyłanie dmgRcvd
            // (dodajemy do neutrali, później może trafić do higherPriorityShips)
            higherPriorityShips = list_remove(higherPriorityShips, senderNumber);
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
            pthread_mutex_unlock(&mutex);
            break;
         }
      }
   }

   return NULL;
}

void cleanup()
{
   neutralShips = list_free(neutralShips);
   lowerPriorityShips = list_free(lowerPriorityShips);
   higherPriorityShips = list_free(higherPriorityShips);
}

int main(int argc, char* argv[])
{
   int provided;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   // printf("%d\n", provided);
   MPI_Comm_rank (MPI_COMM_WORLD, &myNumber);        /* get current process id */
   MPI_Comm_size (MPI_COMM_WORLD, &SHIPS);        /* get number of processes */
   printf( "Battlecruiser number %d of %d reporting\n", myNumber, SHIPS );

   pthread_mutex_lock(&quitMutex);
   pthread_t pth, pth2;
   pthread_create(&pth, NULL, &mainThread, NULL);
   pthread_create(&pth2, NULL, &communicationThread, NULL);

   char ch;
   printf("Type 'q' to terminate\n");
   while(true)
   {
      ch = getchar();
      if(ch == 'q')
         break;
   }

   pthread_mutex_unlock(&quitMutex);

   // oczekiwanie na zakończenie wątków
   printf("Terminating...\n");
   pthread_join(pth, NULL);
   pthread_join(pth2, NULL);
   cleanup();
   printf("Terminated\n");

   MPI_Finalize();
   return 0;
}
