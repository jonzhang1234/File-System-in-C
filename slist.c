/**
 * @file slist.c
 * @author CS3650 staff
 *
 * A simple linked list of strings.
 *
 * This might be useful for directory listings and for manipulating paths.
 */

#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "slist.h"

slist_t *slist_cons(const char *text, slist_t *rest) {
  slist_t *xs = malloc(sizeof(slist_t));
  xs->data = (char *) malloc(strlen(text) + 1);
  strcpy(xs->data, text);
  xs->data[strlen(text)] = '\0';
  xs->refs = 1;
  xs->next = rest;
  return xs;
}

void slist_free(slist_t *xs) {
  if (xs == 0) {
    return;
  }

  xs->refs -= 1;

  if (xs->refs == 0) {
    slist_free(xs->next);
    free(xs->data);
    free(xs);
  }
}

slist_t *slist_explode(const char *text, char delim) {
  if (*text == 0) {
    return 0;
  }

  int plen = 0;
  while (text[plen] != 0 && text[plen] != delim) {
    plen += 1;
  }

  int skip = 0;
  if (text[plen] == delim) {
    skip = 1;
  }

  slist_t *rest = slist_explode(text + plen + skip, delim);
  char *part = alloca(plen + 2);
  memcpy(part, text, plen);
  part[plen] = 0;

  return slist_cons(part, rest);
}

slist_t *slist_get(slist_t *list, int n) {
    int i = 0;
    for(slist_t *item = list; item != NULL; item = item->next) {
        if (i == n) {
            return item;
        }
        i++;
    }
    return NULL;
}

int slist_len(slist_t *list) {
    int len = 0;
    for(slist_t *item = list; item != NULL; item = item->next) {
        len++;
    }
    return len;
}

void slist_print(slist_t *list) {
    slist_t *curr = list;
    printf("[ ");
    while (curr != NULL) {
        printf("\"%s\" ", curr->data);
        curr = curr->next;
    }
    printf("]\n");
}