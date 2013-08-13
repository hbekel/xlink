#ifndef STRINGLIST_H
#define STRINGLIST_H

typedef struct {
  int size;
  char **strings;
} StringList;

StringList *stringlist_new(void);
void stringlist_append(StringList *self, char *string);
void stringlist_append_tokenized(StringList *self, char* string, char *delim);
void stringlist_free(StringList *self);

#endif // STRINGLIST_H
