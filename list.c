#include <stdlib.h>
#include <stdio.h>
#include "list.h"

// #include <stdio.h> // USUNAC

void print_ship(Ship* ship){
   printf("Ship data: id:%d timestamp:%d dmg:%d\n",ship->num,ship->timestamp,ship->dmg);
}

void print_list(Ship* head){
   while(head!=NULL){
      print_ship(head);
      head=head->next;
   }
}

Ship* make_ship(int num, int time, int dmg){
   Ship* ship = (Ship*) malloc(sizeof(Ship));
   ship->num=num;
   ship->timestamp=time;
   ship->dmg=dmg;
   ship->next=NULL;
   return ship;
}


int list_length(Ship* head)
{
   int len = 0;
   while (head!=NULL){
      head=head->next;
   }
   return len;
}

Ship* list_next(Ship* ship)
{
   return ship->next;
}

void list_add(Ship** head, Ship* ship)
{

   ship -> next= *head;
   *head=ship;
   //TODO: remove the rest when everything will be working in our magnificent project
   if (list_find((*head)->next,(*head)->num)!=NULL) printf("The added ship is already on list, we're all doomed!\n");
}

Ship* list_find(Ship* head, int num)
{
   while(head != NULL)
   {
      if(head->num == num )
      {
         return head;
      }
      head = head->next;
   }

   return NULL;
}

Ship* list_findship(Ship* head, Ship* ship)
{
   return list_find(head, ship->num);
}


int list_remove(Ship** head, int num)
{
   if (*head==NULL) return 0;
   Ship* current = *head;
   if (current->num == num){
      *head = (*head)->next;
      free(current);
   }
   while (current->next != NULL ){
      if (current->next->num != num){
         current=current->next;
      }
      else{
         Ship* temp = current->next;
         current->next=current->next->next;
         free(temp);
         return 1;
      }
   }
   //nothing was removed
   return 0;
}

int list_removeship(Ship** head, Ship* ship){
   return list_remove(head,ship->num);
}


void list_free(Ship** head)
{
   while (*head!=NULL){
      Ship* temp = *head;
      *head = (*head)->next;
      free(temp);
   }
}

void list_cut_paste(Ship** head1, Ship** head2){
   while (*head1!=NULL){
      list_add(head2, *head1);
      *head1=(*head1)->next;
   }
   list_free(head1);
}

/*
int main(){
   Ship* head = NULL;
   list_remove(&head,20);
   list_add(&head,make_ship(1,1,1));
   list_add(&head,make_ship(2,2,2));
   list_add(&head,make_ship(3,3,3));
   list_remove(&head,20);
   list_add(&head,make_ship(1,1,1));
   list_free(&head);
   list_add(&head,make_ship(1,1,1));
   print_list(head);
   return 0;
}*/

