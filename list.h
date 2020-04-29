
#if !defined(__LIST_H__)

#define __LIST_H__

#include <stdbool.h>

#include "mutex.h"

namespace panglos {

struct ListItem;

typedef struct ListItem *pList;

typedef pList* (*pnext)(pList item);

void list_push(pList *head, pList w, pnext next_fn, Mutex *mutex);
void list_append(pList *head, pList w, pnext next_fn, Mutex *mutex);
bool list_remove(pList *head, pList w, pnext next_fn, Mutex *mutex);
int list_size(pList *head, pnext next_fn, Mutex *mutex);

pList list_pop(pList *head, pnext next_fn, Mutex *mutex);

typedef int (*cmp_fn)(const pList w1, const pList w2);

void list_add_sorted(pList *head, pList w, pnext next_fn, cmp_fn cmp, Mutex *mutex);

typedef int (*visitor)(pList w, void *arg);

pList  list_find(pList *head, pnext next_fn, visitor fn, void *arg, Mutex *mutex);
void list_visit(pList *head, pnext next_fn, visitor fn, void *arg, Mutex *mutex);

}   //  namespace

#endif // __LIST_H__

//  FIN
