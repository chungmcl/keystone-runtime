#ifndef __TIMING_BUFF_H__
#define __TIMING_BUFF_H__

#include "vm.h"

typedef struct buff_entry {
  struct buff_entry* next;
  uint8_t* dest;
  unsigned long write_time;
  size_t data_size;
  uint8_t data_copy[];  // flexible array member
} buff_entry;

// Initialize the timing buff
bool timing_buff_init();

// Push a new element to the timing buff
bool timing_buff_push(void* dest, void* data, size_t data_size);

// Flushes all buf_entries that are up for removal.
// Returns # of successful flushes.
// Returns -1 on failure.
int timing_buff_flush_due_items(unsigned long time);
int timing_buff_flush();

// Flush the head of the timing buff
bool timing_buff_remove();

int timing_buff_get_count();

void debug_timing_buff();

#endif  // __TIMING_BUFF_H__