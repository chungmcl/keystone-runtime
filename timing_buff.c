#include "timing_buff.h"
#include "string.h"  // for memcpy()

uintptr_t timing_buff;
uintptr_t timing_buff_end;
int timing_buff_size;
int timing_buff_count;

buff_entry* head;
buff_entry* tail;

bool timing_buff_init() {
  /* initialize timing buffer memory */
  // size of page defined by RISCV_PAGE_SIZE in vm_defs.h

  uintptr_t starting_vpn = vpn(EYRIE_ANON_REGION_START);

  // TODO(chungmcl): get rid of PTE_U and write a alloc_pages
  // that doesn't crash without PTE_U
  // PTE flags should be: PTE_R | PTE_W | PTE_D | PTE_A
  int pte_flags = PTE_R | PTE_W | PTE_D | PTE_A | PTE_U;
  uintptr_t valid_pages;
  int req_pages = 1;
  while ((starting_vpn + req_pages) <= EYRIE_ANON_REGION_END) {
    valid_pages = test_va_range(starting_vpn, req_pages);

    if (req_pages <= valid_pages) {
      uintptr_t alloc_result = alloc_page(starting_vpn, pte_flags);
      // if alloc_page fails
      if (alloc_result == 0) {
        return false;
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
  head = NULL;
  tail = NULL;
  return true;
}

bool timing_buff_push(void* dest, void* data, size_t data_size) {
  // TODO(chungmcl):
  // - If you run out of space, just ask SM to wait until 
  // next interval, then dequeue the thing (note that
  // this makes it so that a buffer of even size zero
  // should still work with timing stuff!)
  size_t total_size = sizeof(buff_entry) + data_size;
  buff_entry* entry_ptr;

  if (timing_buff_count == 0) {
    entry_ptr = (buff_entry*)timing_buff;
    head = entry_ptr;
    tail = entry_ptr;
  } else if (tail > head) {
    if ((buff_entry*)timing_buff_end - (tail + sizeof(buff_entry) + tail->data_size) >= total_size) {
      entry_ptr = (tail + sizeof(buff_entry) + tail->data_size);
    } else if (head - (buff_entry*)timing_buff >= total_size) {
      entry_ptr = (buff_entry*)timing_buff;
    } else return false;
  } else {
    if (head - (tail + sizeof(buff_entry) + tail->data_size) >= total_size) {
      entry_ptr = (tail + sizeof(buff_entry) + tail->data_size);
    } else return false;
  }

  entry_ptr->next = head;
  entry_ptr->data_size = data_size;
  entry_ptr->write_time = sbi_get_time() + 2 * sbi_get_interval_len();
  entry_ptr->dest = dest;
  memcpy(entry_ptr->data_copy, data, data_size);

  timing_buff_count += 1;
  tail->next = entry_ptr;
  tail = entry_ptr;
  return true;
}

int timing_buff_flush() {
  unsigned long time = sbi_get_time();
  buff_entry* curr = head;
  int count = 0;
  while (curr != NULL) {
    if (time >= curr->write_time) {
      if (timing_buff_remove()) {
        count += 1;
      } else return -1;
    } else break;
    curr = curr->next;
  }
  return count;
}

bool timing_buff_remove() {
  // TODO(chungmcl): get the result of memcpy()?
  memcpy(head->dest, head->data_copy, head->data_size);
  head = head->next;
  timing_buff_count -= 1;
  return true;
}