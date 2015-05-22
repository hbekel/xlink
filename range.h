#ifndef RANGE_H
#define RANGE_H

typedef struct {
  int start;
  int end;
  int min;
  int max;
} Range;

Range* range_new(int start, int end);
Range* range_new_from_int(int range); 
Range* range_parse(char *str);
bool range_equals(Range* self, Range* range);
bool range_valid(Range* self);
int range_size(Range* self);
bool range_ends(Range *self);
void range_move(Range* self, int amount);
bool range_inside(Range* self, Range* range);
bool range_outside(Range* self, Range* range);
bool range_overlaps(Range* self, Range* range);
void range_print(Range* self);
void range_free(Range* self);

#endif // RANGE_H
