#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "util/list.h"

void list_append(struct list *list, struct list_node *node) {
  assert(list != NULL);
  assert(node != NULL);

  node->next = NULL;

  if (list->tail != NULL) {
    list->tail->next = node;
  } else {
    list->head = node;
  }

  list->tail = node;
}

void list_remove(struct list *list, struct list_node *node) {
  struct list_node *pos;
  struct list_node *prev;

  assert(list != NULL);

  for (prev = NULL, pos = list->head; pos != NULL;
       prev = pos, pos = pos->next) {
    if (pos == node) {
      if (prev != NULL) {
        prev->next = node->next;
      } else {
        list->head = node->next;
      }

      if (node->next == NULL) {
        list->tail = prev;
      }

      node->next = NULL;

      return;
    }
  }
}
