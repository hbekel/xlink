#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------

#define lo(x) x & 0xff
#define hi(x) x >> 8

//------------------------------------------------------------------------------
// StringList
//------------------------------------------------------------------------------

typedef struct {
  int size;
  char **strings;
} StringList;

StringList *stringlist_new(void);
void stringlist_append(StringList *self, char *string);
void stringlist_append_tokenized(StringList *self, char* string, char *delim);
char* stringlist_last(StringList *self);
void stringlist_remove_last(StringList *self);
void stringlist_free(StringList *self);

//------------------------------------------------------------------------------
// Logger
//------------------------------------------------------------------------------

#define LOGLEVEL_NONE  0
#define LOGLEVEL_ERROR 1
#define LOGLEVEL_WARN  2
#define LOGLEVEL_INFO  3
#define LOGLEVEL_DEBUG 4
#define LOGLEVEL_TRACE 5
#define LOGLEVEL_ALL   6

typedef struct {
  int level;
  StringList *context;
  bool enabled;
  void (*set) (char *level);  
  void (*enter) (char *context);
  void (*leave) (void);
  void (*suspend) (void);
  void (*resume) (void);
  void (*log) (int level, char *format, va_list ap);
  void (*warn) (char *format, ...);
  void (*error) (char *format, ...);
  void (*info) (char *format, ...);
  void (*debug) (char *format, ...);
  void (*trace) (char *format, ...);
  void (*free) (void);
} Logger;

extern Logger* logger;

//------------------------------------------------------------------------------
// Watch
//------------------------------------------------------------------------------

typedef struct {
  struct timeval start;
} Watch;

Watch* watch_new(void);
void watch_start(Watch*);
double watch_elapsed(Watch*);
void watch_free(Watch*);

#endif // UTIL_H
