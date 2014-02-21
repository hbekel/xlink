#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
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

#define LOGLEVEL_NONE  0
#define LOGLEVEL_ERROR 1
#define LOGLEVEL_WARN  2
#define LOGLEVEL_INFO  3
#define LOGLEVEL_DEBUG 4
#define LOGLEVEL_TRACE 5
#define LOGLEVEL_ALL   6

//------------------------------------------------------------------------------
// Logger
//------------------------------------------------------------------------------

typedef struct {
  int level;
  StringList *context;
  void (*set) (char *level);  
  void (*enter) (char *context);
  void (*leave) (void);
  void (*log) (int level, char *format, va_list ap);
  void (*warn) (char *format, ...);
  void (*error) (char *format, ...);
  void (*info) (char *format, ...);
  void (*debug) (char *format, ...);
  void (*trace) (char *format, ...);
} Logger;

void _logger_set(char *level);
void _logger_enter(char *context);
void _logger_leave(void);
void _logger_log(int level, char *fmt, va_list ap);
void _logger_error(char *fmt, ...);
void _logger_warn(char *fmt, ...);
void _logger_info(char *fmt, ...);
void _logger_debug(char *fmt, ...);
void _logger_trace(char *fmt, ...);

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
