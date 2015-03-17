#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "range.h"

Range* range_new(int start, int end) {

  Range* self = (Range*) calloc(1, sizeof(Range));
  self->min = 0;
  self->max = 0x10000;
  self->start = start;
  self->end = end;
  return self;
}

Range* range_parse(char *str) {

  Range* self = range_new(0, 0x10000);

  if(strlen(str) == 0) {
    return self;
  }
  
  char *start = str;
  char *end = strstr(str, "-");

  if(end != NULL) {
    end++;
  }
  
  while(isspace(start[0])) start++;
  
  if(strlen(start) > 0 && start[0] != '-') {
    self->start = (int) strtol(start, NULL, 0);
  }

  if(end == NULL) {
    self->end = -1;
  }
  else {
    while(isspace(end[0])) end++;
    
    if(strlen(end) > 0) {
      self->end = (int) strtol(end, NULL, 0);
    }
  }
  return self;
}

bool range_valid(Range* self) {
  return
    self->start >= self->min &&
    self->start <= self->max &&
    self->end >= self->min &&
    self->end <= self->max &&
    self->end >= self->start;
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

bool range_ends(Range* self) {
  return self->end != -1;
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
    (self->end >= range->start && self->end <= range->end) ||  
    (range->start >= self->start && range->start <= self->end) ||
    (range->end >= self->start && range->end <= self->end);  
}

void range_free(Range* self) {
  free(self);
}
