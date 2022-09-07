#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>

#include "util.h"

//------------------------------------------------------------------------------
// StringList
//------------------------------------------------------------------------------

StringList *stringlist_new(void) {
  StringList *stringlist = (StringList*) calloc(1, sizeof(StringList));
  stringlist->size = 0;
  stringlist->strings = (char**) NULL;
  return stringlist;
}

void stringlist_append(StringList *self, char *string) {
  size_t len = strlen(string);
  self->strings = (char**) realloc(self->strings, (self->size+1) * sizeof(char *));
  self->strings[self->size] = calloc(len+1, sizeof(char));
  strncpy(self->strings[self->size], string, len+1);
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

char* stringlist_last(StringList *self) {
  return self->strings[self->size-1];
}

void stringlist_remove_last(StringList *self) {
  free(self->strings[--self->size]);
}

void stringlist_free(StringList *self) {
  for(int i=0; i<self->size; i++) {
    free(self->strings[i]);
  }
  free(self->strings);
  free(self);
}

//------------------------------------------------------------------------------
// Logger
//------------------------------------------------------------------------------

#define VA_START va_list ap; va_start(ap, fmt)
#define VA_END va_end(ap)

static void _logger_print_context() {
  if(logger->context != NULL && logger->context->size > 0) {
    printf("%s: ", stringlist_last(logger->context)); 
  }
}

static void _logger_print_level(int level) {
  switch(level) {

  case LOGLEVEL_ERROR:
    printf("error: ");
    break;
    
  case LOGLEVEL_WARN:
    printf("warning: ");
    break;
  }
}

static void _logger_log(int level, char *fmt, va_list ap) {

  char format[256];
  int count = 0;
  int i, j;
  char c;
  double d;
  unsigned u;
  char *s;
  void *v;

  if(level > logger->level || !logger->enabled) return;

  _logger_print_context();
  _logger_print_level(level);
  
  while (*fmt) {
    for (j = 0; fmt[j] && fmt[j] != '%'; j++)
      format[j] = fmt[j];                   
    if (j) {
      format[j] = '\0';
      count += printf(format);
      fmt += j;
    } else {
      for (j = 0; !isalpha((int)fmt[j]); j++) {
        format[j] = fmt[j];
        if (j && fmt[j] == '%')
          break;
      }
      format[j] = fmt[j];
      format[j + 1] = '\0';
      fmt += j + 1;
      
      switch (format[j]) {
      case 'd':
      case 'i':
        i = va_arg(ap, int);
        count += printf(format, i);
        break;
      case 'o':
      case 'x':
      case 'X':
      case 'u':
        u = va_arg(ap, unsigned);
        count += printf(format, u);
        break;
      case 'c':
        c = (char) va_arg(ap, int);
        count += printf(format, c);
        break;
      case 's':
        s = va_arg(ap, char *);
        count += printf(format, s);
        break;
      case 'f':
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        d = va_arg(ap, double);
        count += printf(format, d);
        break;
      case 'p':
        v = va_arg(ap, void *);
        count += printf(format, v);
        break;
      case 'n':
        count += printf("%d", count);
        break;
      case '%':
        count += printf("%%");
        break;
      default:
        printf("Invalid format specifier '%s'", &(format[j]));
      }
    }
  }
  printf("\n");
  fflush(stdout);
}

static void _logger_set(char* level) {
  
  int current_level = logger->level;

  if(strncasecmp(level, "NONE", 4) == 0) logger->level = LOGLEVEL_NONE; else  
  if(strncasecmp(level, "ERROR", 5) == 0) logger->level = LOGLEVEL_ERROR; else
  if(strncasecmp(level, "WARN", 4) == 0) logger->level = LOGLEVEL_WARN; else
  if(strncasecmp(level, "INFO", 4) == 0) logger->level = LOGLEVEL_INFO; else
  if(strncasecmp(level, "DEBUG", 5) == 0) logger->level = LOGLEVEL_DEBUG; else
  if(strncasecmp(level, "TRACE", 5) == 0) logger->level = LOGLEVEL_TRACE; else
  if(strncasecmp(level, "ALL", 3) == 0) logger->level = LOGLEVEL_ALL; 
  else {  
    logger->level = LOGLEVEL_WARN;
    logger->warn("unkown loglevel \"%s\"", level);
    logger->level = current_level;
  }
}

static void _logger_inc() {
  if(logger->level < LOGLEVEL_ALL) {
    logger->level++;
  }
}

static void _logger_dec() {
  if(logger->level > LOGLEVEL_NONE) {
    logger->level--;
  }
}

static void _logger_enter(char *context) {

  if(logger->context == NULL) {
    logger->context = stringlist_new();
  }
  stringlist_append(logger->context, context);
};

static void _logger_leave() { 
  stringlist_remove_last(logger->context);
};

static void _logger_suspend() {
  logger->enabled = false;
}

static void _logger_resume() {
  logger->enabled = true;
}

static void _logger_error(char *fmt, ...) { VA_START; logger->log(LOGLEVEL_ERROR, fmt, ap); VA_END; }
static void _logger_warn(char *fmt, ...)  { VA_START; logger->log(LOGLEVEL_WARN,  fmt, ap); VA_END; }
static void _logger_info(char *fmt, ...)  { VA_START; logger->log(LOGLEVEL_INFO,  fmt, ap); VA_END; }
static void _logger_debug(char *fmt, ...) { VA_START; logger->log(LOGLEVEL_DEBUG, fmt, ap); VA_END; }
static void _logger_trace(char *fmt, ...) { VA_START; logger->log(LOGLEVEL_TRACE, fmt, ap); VA_END; }

static void _logger_free() {
  if(logger->context != NULL) 
    stringlist_free(logger->context);
}

Logger _logger = { 
  .level   = LOGLEVEL_INFO, 
  .context = NULL,
  .enabled = true,
  .set     = &_logger_set,
  .inc     = &_logger_inc,
  .dec     = &_logger_dec,
  .enter   = &_logger_enter,
  .leave   = &_logger_leave,
  .suspend = &_logger_suspend,
  .resume  = &_logger_resume,
  .log     = &_logger_log,
  .error   = &_logger_error,
  .warn    = &_logger_warn,
  .info    = &_logger_info,
  .debug   = &_logger_debug,
  .trace   = &_logger_trace,
  .free    = &_logger_free
};

Logger *logger = &_logger;

//------------------------------------------------------------------------------
// Watch
//------------------------------------------------------------------------------

Watch* watch_new() {

  Watch *watch = (Watch*) calloc(1, sizeof(Watch));
  watch_start(watch);
  return watch;
}

void watch_start(Watch* self) {
  gettimeofday(&self->start, NULL);
}

double watch_elapsed(Watch* self) {
  struct timeval now;
  double elapsed;
  gettimeofday(&now, NULL);

  elapsed = (now.tv_sec - self->start.tv_sec) * 1000.0;    
  elapsed += (now.tv_usec - self->start.tv_usec) / 1000.0; 

  return elapsed;
}

void watch_free(Watch* self) {
  free(self);
}

//------------------------------------------------------------------------------
// Chunked processing
//------------------------------------------------------------------------------

bool chunked(bool (*callback) (unsigned short chunk, void *context), void* context, unsigned short chunk, int size) {

  bool result = true;

  do {
    if(size > chunk) {
      size -= chunk;
    }
    else {
      chunk = size;
      size = 0;
    }
    
    if(!(result = (*callback)(chunk, context))) {
      break;
    }    
  } while(size > 0);
  return result;
}

//------------------------------------------------------------------------------
