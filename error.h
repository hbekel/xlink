#ifndef XLINK_ERROR_H
#define XLINK_ERROR_H

#include <stdio.h>

#include "xlink.h"
#include "util.h"

#if defined(XLINK_LIBRARY_BUILD)

#define SET_ERROR(c, f, ...) xlink_error->code = c; sprintf(xlink_error->message, f, ##__VA_ARGS__); logger->error(f, ##__VA_ARGS__); 
#define CLEAR_ERROR_IF(r) if (r) { xlink_error->code = XLINK_SUCCESS; sprintf(xlink_error->message, "Success"); }

#else

#define SET_ERROR(c, f, ...)
#define CLEAR_ERROR_IF(r)

#endif

#endif // XLINK_ERROR_H
