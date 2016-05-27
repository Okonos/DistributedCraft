#include "ship.h"


int list_length(Ship* head);

Ship* list_next(Ship* ship);

void list_add(Ship** head, Ship* ship);

Ship* list_find(Ship* head, int num);

Ship* list_findship(Ship* head, Ship* ship);

int list_remove(Ship** head, int num);

int list_removeship(Ship** head, Ship* ship);

void list_free(Ship** head);

void list_cut_paste(Ship** head1, Ship** head2);

Ship* make_ship(int num, int time, int dmg);
