#include "ship.h"

typedef GSList* List;

int list_length(GSList *list);

GSList* list_next(GSList *list);

GSList* list_append(GSList *list, Ship *ship);

Ship* list_find(GSList *list, int num);

GSList* list_remove(GSList *list, int num);

GSList* list_free(GSList *list);
