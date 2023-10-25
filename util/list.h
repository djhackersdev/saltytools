#pragma once

#include <stdbool.h>

struct list {
  struct list_node *head;
  struct list_node *tail;
};

struct list_node {
  struct list_node *next;
};

void list_append(struct list *list, struct list_node *node);
void list_remove(struct list *list, struct list_node *node);
