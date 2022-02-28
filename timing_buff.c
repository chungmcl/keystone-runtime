#include "timing_buff.h"

uint8_t* timing_buff;
uint8_t* timing_buff_end;
int timing_buff_size;
int timing_buff_count;

buf_entry* head;
buf_entry* tail;

typedef struct {
  buf_entry* next;
  uint8_t* dest;
  size_t data_size;
  uint8_t data_copy[];  // flexible array member
} buf_entry;

bool timing_buff_init() {
  /* initialize timing buffer memory */
  // size of page defined by RISCV_PAGE_SIZE in vm_defs.h
  // PTE flags should be: PTE_R | PTE_W | PTE_D | PTE_A
  // look at linux_wrap.c mmap for example

  // TODO(chungmcl): which one do I use? what's the difference?
  uintptr_t starting_vpn = vpn(EYRIE_UNTRUSTED_START);
  //uintptr_t starting_vpn = vpn(EYRIE_ANON_REGION_START);

  // TODO(chungmcl): find better way to do this
  int req_pages = 1;

  // TODO(chungmcl): get rid of PTE_U and write a alloc_pages
  // that doesn't crash without PTE_U
  int pte_flags = PTE_R | PTE_W | PTE_D | PTE_A | PTE_U;
  uintptr_t valid_pages;
  while((starting_vpn + req_pages) <= EYRIE_ANON_REGION_END){
    valid_pages = test_va_range(starting_vpn, req_pages);

    if(req_pages <= valid_pages){
      uintptr_t alloc_result = alloc_page(starting_vpn, pte_flags);
      // if alloc_page fails
      if (alloc_result == 0) {
        // TODO(chungmcl): what do we do here?
      }
      timing_buff = alloc_result;
      break;
    }
    else
      starting_vpn += valid_pages + 1;
  }

  timing_buff_size = RISCV_PAGE_SIZE;
  timing_buff_end = timing_buff + timing_buff_size;
  timing_buff_count = 0;
  head = timing_buff;
  tail = timing_buff;
  return true;
}

void copy(uint8_t* from, uint8_t* to, size_t size) {
  for (uint8_t* ptr = to; ptr < (ptr + size); ptr++) {
    *ptr = *from;
    from++;
  }
}

bool timing_buff_push(void* dest, void* data, size_t data_size) {
  // calculate size of metadata (buf_entry struct) 
  // + size of data (buf_entry flexible array member)
  size_t total_size = sizeof(buf_entry) + data_size;
  buf_entry* entry_ptr;

  if (tail > head) {
    if (timing_buff_end - tail >= total_size) {
      entry_ptr = tail;
    } else if (head - timing_buff >= total_size) {
      entry_ptr = timing_buff;
    } else return false;
  } else {
    if (head - tail >= total_size) {
      entry_ptr = tail;
    } else return false;
  }

  entry_ptr->next = NULL;
  entry_ptr->data_size = data_size;
  entry_ptr->dest = dest;
  copy(data, entry_ptr->dest, data_size);

  timing_buff_count += 1;
  tail->next = entry_ptr;
  tail = entry_ptr + total_size;
  return true;
}

bool timing_buff_flush() {
  // flush as many possible (how do i know when to stop?)
  // flush each buf_entry by writing data at data_copy to dest
  // pop mem from data_copy
  // pop buf_entry
  // decrement timing_buff_count


  return true;  // TODO(chungmcl): properly implement me
}

bool timing_buff_remove(void* out) {
  copy(head->data_copy, out, head->size);
  head = head->next;
  return true;
}