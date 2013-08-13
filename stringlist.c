#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stringlist.h"

StringList *stringlist_new(void) {
  StringList *stringlist = (StringList*) calloc(1, sizeof(StringList));
  stringlist->size = 0;
  stringlist->strings = (char**) NULL;
  return stringlist;
}

void stringlist_append(StringList *self, char *string) {
  self->strings = (char**) realloc(self->strings, (self->size+1) * sizeof(char *));
  self->strings[self->size] = calloc(strlen(string)+1, sizeof(char));
  strncpy(self->strings[self->size], string, strlen(string));
  self->size++;
}

void stringlist_append_tokenized(StringList *self, char* string, char *delim) {
  char *substring;

  if((substring = strtok(string, delim)) != NULL) {
    stringlist_append(self, substring);
  
    while((substring = strtok(NULL, delim)) != NULL) {
      stringlist_append(self, substring);
    } 
  }
}

void stringlist_free(StringList *self) {
  for(int i=0; i<self->size; i++) {
    free(self->strings[i]);
  }
  free(self->strings);
  free(self);
}
