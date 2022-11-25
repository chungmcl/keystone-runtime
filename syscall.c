//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
#include "syscall.h"
#include "string.h"
#include "edge_call.h"
#include "uaccess.h"
#include "mm.h"
#include "rt_util.h"
#include "fuzzy_buff.h"

#include "syscall_nums.h"

#ifdef IO_SYSCALL_WRAPPING
#include "io_wrap.h"
#endif /* IO_SYSCALL_WRAPPING */

#ifdef LINUX_SYSCALL_WRAPPING
#include "linux_wrap.h"
#endif /* LINUX_SYSCALL_WRAPPING */

extern void exit_enclave(uintptr_t arg0);

uintptr_t dispatch_edgecall_syscall(struct edge_syscall* syscall_data_ptr, size_t data_len, bool pause){
  int ret;

  if (pause) {
    sbi_pause();
  }

  // Syscall data should already be at the edge_call_data section
  /* For now we assume by convention that the start of the buffer is
   * the right place to put calls */
  struct edge_call* edge_call = (struct edge_call*)shared_buffer;

  edge_call->call_id = EDGECALL_SYSCALL;


  if(edge_call_setup_call(edge_call, (void*)syscall_data_ptr, data_len) != 0){
    return -1;
  }

  ret = sbi_stop_enclave(1);

  if (ret != 0) {
    return -1;
  }

  if(edge_call->return_data.call_status != CALL_STATUS_OK){
    return -1;
  }

  uintptr_t return_ptr;
  size_t return_len;
  if(edge_call_ret_ptr(edge_call, &return_ptr, &return_len) != 0){
    return -1;
  }

  if(return_len < sizeof(uintptr_t)){
    return -1;
  }

  return *(uintptr_t*)return_ptr;
}

uintptr_t dispatch_edgecall_ocall( unsigned long call_id,
				   void* data, size_t data_len,
				   void* return_buffer, size_t return_len){

  uintptr_t ret;
  /* For now we assume by convention that the start of the buffer is
   * the right place to put calls */
  struct edge_call* edge_call = (struct edge_call*)shared_buffer;

  /* We encode the call id, copy the argument data into the shared
   * region, calculate the offsets to the argument data, and then
   * dispatch the ocall to host */
#if FUZZ
  fuzzy_buff_push((void*)&edge_call->call_id, &call_id, sizeof(call_id));
#else
  edge_call->call_id = call_id;
#endif
  

  uintptr_t buffer_data_start = edge_call_data_ptr();

  if(data_len > (shared_buffer_size - (buffer_data_start - shared_buffer))){
    goto ocall_error;
  }
  //TODO safety check on source
  copy_from_user((void*)buffer_data_start, (void*)data, data_len);

  if(edge_call_setup_call(edge_call, (void*)buffer_data_start, data_len) != 0){
    goto ocall_error;
  }

  ret = sbi_stop_enclave(1);

  if (ret != 0) {
    goto ocall_error;
  }

  if(edge_call->return_data.call_status != CALL_STATUS_OK){
    goto ocall_error;
  }

  if( return_len == 0 ){
    /* Done, no return */
    return (uintptr_t)NULL;
  }

  uintptr_t return_ptr;
  size_t ret_len_untrusted;
  if(edge_call_ret_ptr(edge_call, &return_ptr, &ret_len_untrusted) != 0){
    goto ocall_error;
  }

  /* Done, there was a return value to copy out of shared mem */
  /* TODO This is currently assuming return_len is the length, not the
     value passed in the edge_call return data. We need to somehow
     validate these. The size in the edge_call return data is larger
     almost certainly.*/
  copy_to_user(return_buffer, (void*)return_ptr, return_len);

  return 0;

 ocall_error:
  /* TODO In the future, this should fault */
  return 1;
}

uintptr_t handle_copy_from_shared(void* dst, uintptr_t offset, size_t size){

  /* This is where we would handle cache side channels for a given
     platform */

  /* The only safety check we do is to confirm all data comes from the
   * shared region. */
  uintptr_t src_ptr;
  if(edge_call_get_ptr_from_offset(offset, size,
				   &src_ptr) != 0){
    return 1;
  }

  return copy_to_user(dst, (void*)src_ptr, size);
}

int* __debug_get_page() {
  // size of page defined by RISCV_PAGE_SIZE in vm_defs.h
  print_strace("getting page with %d bytes\n", RISCV_PAGE_SIZE);

  uintptr_t starting_vpn = vpn(EYRIE_ANON_REGION_START);
  int pte_flags = PTE_R | PTE_W | PTE_D | PTE_A;
  uintptr_t valid_pages;
  int req_pages = 1;
  while ((starting_vpn + req_pages) <= EYRIE_ANON_REGION_END) {
    valid_pages = test_va_range(starting_vpn, req_pages);

    if (req_pages <= valid_pages) {
      uintptr_t alloc_result = alloc_page(starting_vpn, pte_flags, false);
      // if alloc_page fails
      if (alloc_result == 0) {
        return false;
      }
      return (int*)alloc_result;
      break;
    }
    else
      starting_vpn += valid_pages + 1;
  }

  return 0;
}

void handle_print_time() {
  print_strace("handle_print_time start\n");
  int LOOPS = 100000;
  int PAGE_COUNT = (LOOPS * sizeof(int) / RISCV_PAGE_SIZE) + 1;

  int* pages[PAGE_COUNT];
  for (int page_idx = 0; page_idx < PAGE_COUNT; page_idx++) {
    pages[page_idx] = __debug_get_page();
    if (pages[page_idx] == 0) {
      print_strace("get page failed!\n");
      while (1) {}
    }
    print_strace("got page %d with %d bytes\n", page_idx, RISCV_PAGE_SIZE);
  }

  int i = 0;
  while (i < LOOPS) {
    int page_idx = (i * sizeof(int)) / RISCV_PAGE_SIZE;
    int* page = pages[page_idx];
    int rel_offset = i - page_idx * RISCV_PAGE_SIZE / sizeof(int);
    print_strace("page_idx: %d && page: %p && rel_offset: %d\n", page_idx, page, rel_offset);
    page[rel_offset] = sbi_get_time();
    i += 1;
  }

  for (i = 0; i < LOOPS; i++) {
    int page_idx = (i * sizeof(int)) / RISCV_PAGE_SIZE;
    int* page = pages[page_idx];
    int rel_offset = i - page_idx * RISCV_PAGE_SIZE / sizeof(int);
    print_strace("page_idx: %d && page: %p && rel_offset: %d\n", page_idx, page, rel_offset);
    print_strace("%lu\n", page[rel_offset]);
  }
}

// TODO(chungmcl): syscall to copy/write to shared memory
// consider making it a build option?
bool handle_write_to_shared(void* src, uintptr_t offset, size_t size) {
  uintptr_t dst_ptr;
  if (edge_call_get_ptr_from_offset(offset, size,
  			   &dst_ptr) != 0){
    return false;
  }

#if FUZZ
  copy_from_user((void*)rt_copy_buffer_2, src, size);

  if (!fuzzy_buff_push((void*)dst_ptr, rt_copy_buffer_2, size)) {
    print_strace("write_to_shared push failed.\n");
  }
#else
  copy_from_user((void*)(dst_ptr), src, size);
#endif


  return true;
}

bool handle_pause_ms(unsigned long ms) {
  sbi_pause_ms(ms);
  return true;
}

void init_edge_internals(){
  edge_call_init_internals(shared_buffer, shared_buffer_size);
}

void handle_syscall(struct encl_ctx* ctx)
{
  uintptr_t n = ctx->regs.a7;
  uintptr_t arg0 = ctx->regs.a0;
  uintptr_t arg1 = ctx->regs.a1;
  uintptr_t arg2 = ctx->regs.a2;
  uintptr_t arg3 = ctx->regs.a3;
  uintptr_t arg4 = ctx->regs.a4;

#if FUZZ
  if (n != RUNTIME_SYSCALL_EXIT && n != RUNTIME_SYSCALL_OCALL) {
    fuzzy_buff_flush_due_items(sbi_get_time());
  }
#endif

  // We only use arg5 in these for now, keep warnings happy.
#if defined(LINUX_SYSCALL_WRAPPING) || defined(IO_SYSCALL_WRAPPING)
  uintptr_t arg5 = ctx->regs.a5;
#endif /* IO_SYSCALL_WRAPPING */
  uintptr_t ret = 0;

  ctx->regs.sepc += 4;

  switch (n) {

  case(RUNTIME_SYSCALL_EXIT):
    sbi_exit_enclave(arg0);
    break;
  case(RUNTIME_SYSCALL_OCALL):
    ret = dispatch_edgecall_ocall(arg0, (void*)arg1, arg2, (void*)arg3, arg4);
    break;
  case(RUNTIME_SYSCALL_SHAREDCOPY):
    ret = handle_copy_from_shared((void*)arg0, arg1, arg2);
    break;
  // chungmcl
  case(RUNTIME_SYSCALL_PRINT_TIME):
    handle_print_time();
    break;
  case(RUNTIME_SYSCALL_SHAREDWRITE):
    ret = handle_write_to_shared((void*)arg0, arg1, arg2);
    break;
  case(RUNTIME_SYSCALL_PAUSE_MS):
    ret = handle_pause_ms((unsigned long)arg0);
    break;
  // chungmcl
  case(RUNTIME_SYSCALL_ATTEST_ENCLAVE):;
    copy_from_user((void*)rt_copy_buffer_2, (void*)arg1, arg2);

    ret = sbi_attest_enclave(rt_copy_buffer_1, rt_copy_buffer_2, arg2);

    /* TODO we consistently don't have report size when we need it */
    copy_to_user((void*)arg0, (void*)rt_copy_buffer_1, 2048);
    //print_strace("[ATTEST] p1 0x%p->0x%p p2 0x%p->0x%p sz %lx = %lu\r\n",arg0,arg0_trans,arg1,arg1_trans,arg2,ret);
    break;
  case(RUNTIME_SYSCALL_GET_SEALING_KEY):;
    /* Stores the key receive structure */
    uintptr_t buffer_1_pa = kernel_va_to_pa(rt_copy_buffer_1);

    /* Stores the key identifier */
    uintptr_t buffer_2_pa = kernel_va_to_pa(rt_copy_buffer_2);

    if (arg1 > sizeof(rt_copy_buffer_1) ||
        arg3 > sizeof(rt_copy_buffer_2)) {
      ret = -1;
      break;
    }

    copy_from_user(rt_copy_buffer_2, (void *)arg2, arg3);

    ret = sbi_get_sealing_key(buffer_1_pa, buffer_2_pa, arg3);

    if (!ret) {
      copy_to_user((void *)arg0, (void *)rt_copy_buffer_1, arg1);
    }

    /* Delete key from copy buffer */
    memset(rt_copy_buffer_1, 0x00, sizeof(rt_copy_buffer_1));

    break;


#ifdef LINUX_SYSCALL_WRAPPING
  case(SYS_clock_gettime):
    ret = linux_clock_gettime((__clockid_t)arg0, (struct timespec*)arg1);
    break;

  case(SYS_getrandom):
    ret = linux_getrandom((void*)arg0, (size_t)arg1, (unsigned int)arg2);
    break;

  case(SYS_rt_sigprocmask):
    ret = linux_rt_sigprocmask((int)arg0, (const sigset_t*)arg1, (sigset_t*)arg2);
    break;

  case(SYS_getpid):
    ret = linux_getpid();
    break;

  case(SYS_uname):
    ret = linux_uname((void*) arg0);
    break;

  case(SYS_rt_sigaction):
    ret = linux_RET_ZERO_wrap(n);
    break;

  case(SYS_set_tid_address):
    ret = linux_set_tid_address((int*) arg0);
    break;

  case(SYS_brk):
    ret = syscall_brk((void*) arg0);
    break;

  case(SYS_mmap):
    ret = syscall_mmap((void*) arg0, (size_t)arg1, (int)arg2,
                       (int)arg3, (int)arg4, (__off_t)arg5);
    break;

  case(SYS_munmap):
    ret = syscall_munmap((void*) arg0, (size_t)arg1);
    break;

  case(SYS_exit):
  case(SYS_exit_group):
    print_strace("[runtime] exit or exit_group (%lu)\r\n",n);
    sbi_exit_enclave(arg0);
    break;
#endif /* LINUX_SYSCALL_WRAPPING */

#ifdef IO_SYSCALL_WRAPPING
  case(SYS_read):
    ret = io_syscall_read((int)arg0, (void*)arg1, (size_t)arg2);
    break;
  case(SYS_write):
    ret = io_syscall_write((int)arg0, (void*)arg1, (size_t)arg2);
    break;
  case(SYS_writev):
    ret = io_syscall_writev((int)arg0, (const struct iovec*)arg1, (int)arg2);
    break;
  case(SYS_readv):
    ret = io_syscall_readv((int)arg0, (const struct iovec*)arg1, (int)arg2);
    break;
  case(SYS_openat):
    ret = io_syscall_openat((int)arg0, (char*)arg1, (int)arg2, (mode_t)arg3);
    break;
  case(SYS_unlinkat):
    ret = io_syscall_unlinkat((int)arg0, (char*)arg1, (int)arg2);
    break;
  case(SYS_fstatat):
    ret = io_syscall_fstatat((int)arg0, (char*)arg1, (struct stat*)arg2, (int)arg3);
    break;
  case(SYS_lseek):
    ret = io_syscall_lseek((int)arg0, (off_t)arg1, (int)arg2);
    break;
  case(SYS_ftruncate):
    ret = io_syscall_ftruncate((int)arg0, (off_t)arg1);
    break;
  case(SYS_sync):
    ret = io_syscall_sync();
    break;
  case(SYS_fsync):
    ret = io_syscall_fsync((int)arg0);
    break;
  case(SYS_close):
    ret = io_syscall_close((int)arg0);
    break;
#endif /* IO_SYSCALL_WRAPPING */


  case(RUNTIME_SYSCALL_UNKNOWN):
  default:
    print_strace("[runtime] syscall %ld not implemented\r\n", (unsigned long) n);
    ret = -1;
    break;
  }

  /* store the result in the stack */
  ctx->regs.a0 = ret;
  return;
}
