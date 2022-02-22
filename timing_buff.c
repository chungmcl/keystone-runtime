#include "timing_buff.h"

typedef struct {
  void* dest;
  void* data_copy;
  size_t data_size;
} buf_entry;

int timing_buff_count;

bool timing_buff_push(void* dest, void* data, size_t data_size) {
  // buffer start at timing_buffer
  // buffer end at timing_buffer + RISCV_PAGE_SIZE
  // stack top at timing_buffer + sizeof(buf_entry) * timing_buff_count
  // 
  // [[buf_entry][buf_entry][buf_entry]--->        <---[var width data][var width data][var width data]]

  // on each push, add new buf_entry to stack from LHS of timing_buffer
  // push *data to stack from the RHS of the timing_buffer
  // point buf_entry.data_copy to corresponding data

  timing_buff_count += 1;
  
  return true;  // TODO(chungmcl): properly implement me
}

bool timing_buff_flush() {
  // flush as many possible (how do i know when to stop?)
  // flush each buf_entry by writing data at data_copy to dest
  // pop mem from data_copy
  // pop buf_entry
  // decrement timing_buff_count


  return true;  // TODO(chungmcl): properly implement me
}