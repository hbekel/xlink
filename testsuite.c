#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "range.h"
#include "target.h"

void check(bool condition, const char* message) {
  if(!condition) {
    fprintf(stderr, message);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
  }
}

void test_target() {

#if linux
  printf("Target is linux\n");
#endif

#if windows
  printf("Target is windows\n");
#endif

#if mac
  printf("Target is mac\n");
#endif

#if posix
  printf("Target is posix\n");
#endif  
}

void test_parse(char *str, int start, int end) {

  Range* range = range_new(start, end);
  Range* parsed = range_parse(str);

  printf("Parsing \"%s\", expecting ", str);
  range_print(range);
  
  printf(range_equals(range, parsed) ? "OK: " : "FAIL: ");
  range_print(parsed);

  check(range_equals(range, parsed), "Parsing failed...");
	 
  free(range);
  free(parsed);
}

void test_range() {
  
  Range* range = range_new(0x0801, 0x3000);
  Range* other = range_new(0x0801, 0x3000);
  
  check(range->start == 0x0801 && range->end == 0x3000, "Range not $0801-$3000");

  check(range_size(range) == 0x27ff, "Range size is not $27ff");

  check(range_equals(range, other), "Ranges are not equal");
  
  range_move(range, 0x1000);
  check(range->start == 0x1801 && range->end == 0x4000, "Range not moved to $1801-$4000");

  range_move(range, -0x1000);
  check(range->start == 0x0801 && range->end == 0x3000, "Range not moved back to $0801-$3000");

  range_move(range, -0x1000);
  check(range->start == 0x0000 && range->end == 0x27ff, "Range not moved to $0000-$27ff");

  range_move(range, 0x80000);
  check(range->start == 0xd801 && range->end == 0x10000, "Range not moved to $d801-$10000");  
  
  free(range);
  free(other);

  range = range_new_from_int(0xc000d000);
  other = range_new(0xc000, 0xd000);

  check(range_equals(range, other), "Range from int 0xc000d000 does not equal range $c000-$d000");

  free(range);
  free(other);

  range = range_new(0x1000, 0x2000);
  other = range_new(0x1800, 0x2800);

  check(range_overlaps(range, other), "Ranges don't overlap");
  check(range_overlaps(other, range), "Ranges don't overlap");

  range_move(other, 0x4000);
  check(!range_overlaps(range, other), "Ranges overlap");
  check(!range_overlaps(other, range), "Ranges overlap");

  free(range);
  free(other);

  range = range_new(0x1000, 0x2000);
  other = range_new(0x0800, 0x2800);

  check(range_inside(range, other), "Range $1000-$2000 not within Range $0800-$2800");

  other->start = 0x1000;
  check(range_inside(range, other), "Range $1000-$2000 not within Range $1000-$2800");

  other->start = 0x0800;
  other->end = 0x2000;

  check(range_inside(range, other), "Range $1000-$2000 not within Range $0800-$2000");

  free(range);
  free(other);

  range = range_new(-23, 34);
  check(!range_valid(range), "Range -23-34 is falsely considered valid");

  range->start = 0;
  check(range_valid(range), "Range 0-34 is falsely considered invalid");

  free(range);

  range = range_new(0x080e, 0x0af7);
  other = range_new(0x0801, 0x2b77);

  check(range_overlaps(range, other), "Ranges $080e-$0af7 and $0801-$2b77 don't overlap");
  check(range_overlaps(other, range), "Ranges $0801-$2b77 and $080e-$0af7 don't overlap");

  free(range);
  free(other);

  test_parse("1-5", 1, 5);
  test_parse(" 1 - 5", 1, 5);
  test_parse("", 0, 0x10000);
  test_parse("5-", 5, 0x10000);
  test_parse("5 -  ", 5, 0x10000);
  test_parse("-200", 0, 200);
  test_parse(" -  200", 0, 200);
  test_parse("0xd000-0x10000", 0xd000, 0x10000);
  test_parse("0xa000", 0xa000, -1);
  
  printf("passed range tests\n");
}

int main(int argc, char** argv) {
  test_target();
  test_range();

  exit(EXIT_SUCCESS);
}
