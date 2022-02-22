#ifndef __EPOCH_BUFF_H__
#define __EPOCH_BUFF_H__

#include "vm.h"

bool timing_buff_push(void* dest, void* data, size_t data_len);

// TODO(chungmcl): this should be privatized and
// flushing should be made such that it only occurs
// during an epoch
bool timing_buff_flush();

#endif  // __EPOCH_BUFF_H__