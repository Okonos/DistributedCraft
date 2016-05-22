#include <glib.h>
#include <stdlib.h>
#include "list.h"

// #include <stdio.h> // USUNAC

int list_length(GSList *list)
{
   return g_slist_length(list);
}

GSList* list_next(GSList *list)
{
   return g_slist_next(list);
}

GSList* list_append(GSList *list, Ship *ship)
{
   return g_slist_append(list, ship);
}

Ship* list_find(GSList *list, int num)
{
   GSList *node = list;
   while(node != NULL)
   {
      if( ((Ship *) node->data)->number == num )
      {
         Ship *s = ((Ship *) node->data);
         return s;
      }
      node = g_slist_next(node);
   }

   return NULL;
}

/*
nie zwalnia pamięci usuwanego z listy elementu(tak wynika z testów)
 */
GSList* list_remove_soft(GSList *list, int num)
{
   Ship *s = list_find(list, num);
   if(s != NULL)
      list = g_slist_remove(list, s);

   return list;
}

/*
zwalnia pamięć usuwanego z listy elementu
 */
GSList* list_remove_hard(GSList *list, int num)
{
   Ship *s = list_find(list, num);
   if(s != NULL)
   {
      list = g_slist_remove(list, s);
      free(s);
   }

   return list;
}

GSList* list_free(GSList *list)
{
   // g_list_foreach(list, );
   // g_slist_free(list);
   g_slist_free_full(list, g_free);
   list = NULL;
   return list;
}

// int main()
// {
//    GSList *list = NULL;
//    Ship *s = malloc(sizeof(Ship));
//    s->number = 1;
//    s->timestamp = 10;
//    s->dmg = 3;
//    // Ship s;
//    // s.number = 1;
//    // s.timestamp = 10;
//    // s.dmg = 3;
//    list = list_append(list, s);
//
//    printf("dmg:%d\n", ((Ship *) list->data)->dmg);
//
//    s = malloc(sizeof(Ship));
//    s->number = 2;
//    s->timestamp = 11;
//    s->dmg = 5;
//    list = list_append(list, s);
//
//    // Ship *s2 = malloc(sizeof(Ship));
//    // s2->number = 2;
//    // s2->timestamp = 11;
//    // s2->dmg = 5;
//    // list = list_append(list, s2);
//
//    printf("dmg:%d\nlen:%d\n", ((Ship *) list->data)->dmg, list_length(list));
//
//    list = list_remove(list, 1);
//    printf("dmg:%d\nlen:%d\n", ((Ship *) list->data)->dmg, list_length(list));
//    // printf("dmg:%d\n", ((Ship *) list->data)->dmg);
//
//    list = list_free(list);
//    // list = list_clear(list);
//    if(list == NULL)
//       printf("List is null\n");
//    // free(s);
//    // free(s2);
//    // printf("%d\n", s->dmg);
//
//    printf("len:%d\n", list_length(list));
//    // printf("dmg:%d\n", ((Ship *) list->data)->dmg);
//
//    // s = malloc(sizeof(Ship));
//    // s->number = 2;
//    // s->timestamp = 11;
//    // s->dmg = 5;
//    // list = list_append(list, s);
//    // printf("%d\n", list_length(list));
//
//    return 0;
// }
