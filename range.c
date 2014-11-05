#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "range.h"

Range* range_new(int start, int end) {

  Range* self = (Range*) calloc(1, sizeof(Range));
  self->min = 0;
  self->max = 0x10000;
  self->start = start;
  self->end = end;
  return self;
}

void range_print(Range* self) {
  printf("$%04X-$%04X\n", self->start, self->end);
}

int range_size(Range* self) {
  return self->end - self->start;
}

bool range_equals(Range* self, Range* range) {
  return self->start == range->start && self->end == range->end;
}

void range_move(Range* self, int amount) {

  if(amount == 0) return;
  
  if(amount > 0) {
    if(self->end + amount > self->max) {
      amount -= (self->end + amount) - self->max;
    }
  }

  if(amount < 0) {
    if(self->start + amount < self->min) {
      amount -= (self->start + amount);
    }
  }
  
  self->start += amount;
  self->end += amount;
}

bool range_inside(Range* self, Range* range) {
  if(range_equals(self, range)) {
    return true;    
  }
  return self->start >= range->start && self->end <= range->end;
}

bool range_outside(Range* self, Range* range) {
  return !range_inside(self, range);
}

bool range_overlaps(Range* self, Range* range) {

  if(range_equals(self, range)) {
    return true;
  }

  return
    (self->start >= range->start && self->start <= range->end) ||
    (self->end >= range->start && self->end <= range->end);  
}

void range_free(Range* self) {
  free(self);
}
