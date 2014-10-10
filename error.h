#ifndef XLINK_ERROR_H
#define XLINK_ERROR_H

#include <stdio.h>

#include "xlink.h"
#include "util.h"

#if defined(XLINK_LIBRARY_BUILD)

#define FAILURE(c, f, ...) xlink_error->code = c; sprintf(xlink_error->message, f, ##__VA_ARGS__); logger->error(f, ##__VA_ARGS__); 
#define SUCCESS_IF(r) if (r) { xlink_error->code = XLINK_SUCCESS; sprintf(xlink_error->message, "Success"); }

#else

#define FAILURE
#define SUCCESS_IF(r)

#endif

#endif // XLINK_ERROR_H
