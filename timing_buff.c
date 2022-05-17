#include "timing_buff.h"
#include "string.h"  // for memcpy()
#include "vm.h"
#include "mm.h"

// TODO(chungmcl): REMOVE ME! For debugging
#include "syscall.h" // (print_strace() calls)

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
  size_t add_size = sizeof(buff_entry) + data_size;

  buff_entry* entry_ptr;

  if (timing_buff_count == 0) {
    entry_ptr = (buff_entry*)timing_buff;
    head = entry_ptr;
    tail = entry_ptr;
  } else {
    uintptr_t head_raw = (uintptr_t)head;
    uintptr_t next_raw = (uintptr_t)tail + sizeof(buff_entry) + tail->data_size;
    if (tail > head) {
      if ((size_t)(timing_buff_end - next_raw) >= add_size) {
        //        head <------- [USED SPACE] -------> tail <---- [FREE SPACE] ---->
        // start ~~^                                                         end ~~^
        entry_ptr = (buff_entry*)next_raw;
      } else if ((size_t)(head_raw - timing_buff) >= add_size) {
        //          <------- [FREE SPACE] -------> head <------ [USED SPACE] ------> tail 
        // start ~~^                                                               end ~~^
        entry_ptr = (buff_entry*)timing_buff;
      } else {
        return false;
      }
    } else {
      if ((size_t)(head_raw - next_raw) >= add_size) {
        //          |---- [USED SPACE] ----> tail <---- [FREE SPACE] ----> head <---- [USED SPACE] ----|
        // start ~~^                                                                              end ~~^
        entry_ptr = (buff_entry*)next_raw;
      } else {
        return false;
      }
    }
  }
  
  unsigned long time = sbi_get_time();
  entry_ptr->next = head;
  entry_ptr->data_size = data_size;
  entry_ptr->write_time = time + 2 * sbi_get_interval_len();

  entry_ptr->dest = dest;
  memcpy(entry_ptr->data_copy, data, data_size);

  timing_buff_count += 1;
  tail->next = entry_ptr;
  tail = entry_ptr;
  if (timing_buff_count == 0) {
    head = tail;
  }

  return true;
}

int timing_buff_flush_due_items(unsigned long curr_time) {
  buff_entry* curr = head;
  int count = 0;
  int timing_buff_count_copy = timing_buff_count;
  for (int i = 0; i < timing_buff_count_copy; i++) {
    if (curr_time >= curr->write_time) {
      print_strace("timing_buff_flush_due_items() time: %lu\n", curr_time);
      print_strace("timing_buff_flush_due_items() write_time: %lu\n", curr->write_time);
      if (timing_buff_remove()) {
        count += 1;
      } else return -1;
      print_strace("\n");
    } else return count;
    curr = curr->next;
  }
  return count;
}

int timing_buff_flush() {
  int count = 0;
  while (timing_buff_count != 0) {
    int items_flushed = timing_buff_flush_due_items(sbi_pause());
    if (items_flushed != -1) {
      count += items_flushed;
    } else return -1;
  }
  return count;
}

bool timing_buff_remove() {
  if (timing_buff_count > 0) {
    memcpy(head->dest, head->data_copy, head->data_size);
    print_strace("timing_buff_remove() wrote val: %lu\n", *head->dest);
    head = head->next;
    timing_buff_count -= 1;
    return true;
  } else return false;
}

int timing_buff_get_count() {
  return timing_buff_count;
}

// for print_strace() to work, make sure to change the line:
//  set(eyrie_plugins "freemem")
// to:
//  set(eyrie_plugins "freemem strace_debug")
// in the CMakeLists.txt of whatever app you're
// building
void debug_timing_buff() {
  buff_entry* curr = head;
  print_strace("-------------------------------------------------------------------\n");
  print_strace("timing_buff: %p\n", (void*)timing_buff);
  print_strace("Head: %p\n", (void*)head);
  print_strace("Tail: %p\n\n", (void*)tail);
  for (int i = 0; i < timing_buff_count; i++) {
    print_strace("Element #: %i\n", i);
    print_strace("curr->next: %p\n", (void*)curr->next);
    print_strace("curr->dest: %p\n", (void*)curr->dest);
    print_strace("curr->write_time: %lu\n", curr->write_time);
    print_strace("curr->data_size: %zu\n", curr->data_size);
    print_strace("curr->data_copy: %lu\n", *curr->data_copy);
    print_strace("\n");
    curr = curr->next;
  }
}