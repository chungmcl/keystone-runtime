//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "printf.h"
#include "regs.h"
#include "edge_syscall.h"
#include "vm.h"

#define RUNTIME_SYSCALL_UNKNOWN             1000
#define RUNTIME_SYSCALL_OCALL               1001
#define RUNTIME_SYSCALL_SHAREDCOPY          1002
#define RUNTIME_SYSCALL_ATTEST_ENCLAVE      1003
#define RUNTIME_SYSCALL_GET_SEALING_KEY     1004

#define RUNTIME_SYSCALL_SHARED_WRITE        1005
#define RUNTIME_SYSCALL_PAUSE_MS            1006
#define RUNTIME_SYSCALL_REG_CLOCK_IPI       1007
#define RUNTIME_SYSCALL_PRINT_TIME          1008

#define RUNTIME_SYSCALL_EXIT                1101

void handle_syscall(struct encl_ctx* ctx); // (3), (4) [dispatch_edgecall_ocall() above handle_syscall() @ :64]
void init_edge_internals(void);
uintptr_t dispatch_edgecall_syscall(struct edge_syscall* syscall_data_ptr,
                                    size_t data_len, bool pause);

// Define this to enable printing of a large amount of syscall information
//#define INTERNAL_STRACE 1

#ifdef INTERNAL_STRACE
#define print_strace printf
#else
#define print_strace(...)
#endif

#endif /* syscall.h */

