#ifndef __TIMING_BUFF_H__
#define __TIMING_BUFF_H__

#include "vm.h"
#include "mm.h"

bool timing_buff_init();

bool timing_buff_push(void* dest, void* data, size_t data_size);

// TODO(chungmcl): this should be privatized and
// flushing should be made such that it only occurs
// during an epoch
bool timing_buff_flush();

bool timing_buff_remove(void* out);

#endif  // __TIMING_BUFF_H__