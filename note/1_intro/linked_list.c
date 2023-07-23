#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct sNode {
  struct sNode *prev;
  struct sNode *next;
  char *string;
} Node;

// insert new node after prev or at the head if new list
// this just uses a prev pointer and the find function to get to the
// point at which the new node should be inserted
// hence, the iteration to find the location is not done in insert but in find
void insert(Node **list, Node *prev, const char *string) {
  // create the node
  Node *node = malloc(sizeof(Node));
  // copy string to the heap
  node->string = malloc(strlen(string) + 1);
  strcpy(node->string, string);

  if (prev == NULL) {
    // if the list is not empty, but prev is null, then the new node is entered
    // at the head
    if (*list != NULL) {
      (*list)->prev = node;
    }
    node->prev = NULL;
    node->next = *list;
    *list = node; // point the head to the new node
  } else {
    node->next = prev->next;
    if (node->next != NULL) {
      node->next->prev = node;
    }
    prev->next = node;
    node->prev = prev;
  }
}

Node *find(Node *cur, const char *string) {
  while (cur != NULL) {
    if (strcmp(string, cur->string) == 0) {
      return cur;
    }
    cur = cur->next;
  }
  return NULL;
}

void delete(Node **list, Node *node) {

  // unlink the node
  if (node->prev != NULL) {
    node->prev->next = node->next;
  }

  if (node->next != NULL) {
    node->next->prev = node->prev;
  }

  // if head (*list is head) is being deleted, update it
  // pointer to the head of the node
  if (*list == node) {
    *list = node->next;
  }

  // cleanup
  free(node->string);
  free(node);
}

void print_list(Node *list) {
  while (list != NULL) {
    printf("%p [prev %p next %p] %s\n", list, list->prev, list->next,
           list->string);
    list = list->next;
  }
}

void cleanup(Node *head) {
  if (head) {
    Node *prev = head;
    Node *cur = prev->next;
    while (cur != NULL) {
      // unlink
      prev->next = NULL;
      cur->prev = NULL;

      free(prev->string);
      free(prev);

      prev = cur;
      cur = cur->next;
    }
    // after hitting the end, prev is still there. Need to delete
    free(prev->string);
    free(prev);
  }
}

int main(int argc, const char *argv[]) {
  printf("Hello\n");

  Node *list = NULL;
  insert(&list, NULL, "four");
  insert(&list, NULL, "one");
  insert(&list, find(list, "one"), "two");
  insert(&list, find(list, "two"), "three");

  print_list(list);

  printf("--delete three --\n");
  delete (&list, find(list, "three"));
  print_list(list);

  printf("--delete one --\n");
  delete (&list, find(list, "one"));
  print_list(list);

  cleanup(list);

  return 0;
}
