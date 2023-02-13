#ifndef __FUZZY_BUFF_H__
#define __FUZZY_BUFF_H__

#include "vm.h"

// Essentially enable or disable the timing buff at compile time --
// If set to 0 (disabled), timing buff may still be used
// but fuzzing properties are effectively disabled.
#define FUZZ 1

extern bool use_fuzzy_buff;

typedef struct buff_entry {
  struct buff_entry* next;
  uint8_t* dest;
  unsigned long write_time;
  size_t data_size;
  uint8_t data_copy[];  // flexible array member
} buff_entry;

void fuzzy_buff_ipi_handle();

// Initialize the timing buff
bool fuzzy_buff_init();

// Push a new element to the timing buff
bool fuzzy_buff_push(void* dest, void* data, size_t data_size);

// Flushes all buf_entries that are up for removal,
// given the current time.
// Returns # of successful flushes.
// Returns -1 on failure.
int fuzzy_buff_flush_due_items(unsigned long curr_time);

// Flushes all items, regardless of their due time.
// Returns -1 on failure.
int fuzzy_buff_flush();

// Flush the head of the timing buff.
bool fuzzy_buff_remove();

// Get # of items in timing buff.
int fuzzy_buff_get_count();

// void debug_fuzzy_buff();

#endif  // __FUZZY_BUFF_H__