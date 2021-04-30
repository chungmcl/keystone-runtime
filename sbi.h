//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef __SBI_H_
#define __SBI_H_

#include <stdint.h>

#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2

#define SBI_SM_CREATE_ENCLAVE    2001
#define SBI_SM_DESTROY_ENCLAVE   2002
#define SBI_SM_RUN_ENCLAVE       2003
#define SBI_SM_RESUME_ENCLAVE    2005
#define SBI_SM_CLONE_ENCLAVE     2006
#define SBI_SM_RANDOM            3001
#define SBI_SM_ATTEST_ENCLAVE    3002
#define SBI_SM_GET_SEALING_KEY   3003
#define SBI_SM_STOP_ENCLAVE      3004
#define SBI_SM_EXIT_ENCLAVE      3006
#define SBI_SM_SNAPSHOT          3007
#define SBI_SM_FORK              3008
#define SBI_SM_CALL_PLUGIN       4000

/* Plugin IDs and Call IDs */
#define SM_MULTIMEM_PLUGIN_ID   0x01
#define SM_MULTIMEM_CALL_GET_SIZE 0x01
#define SM_MULTIMEM_CALL_GET_ADDR 0x02

#define SBI_STOP_REQ_INTERRUPTED  0
#define SBI_STOP_REQ_EDGE_CALL    1
#define SBI_STOP_REQ_CLONE        2
#define SBI_STOP_REQ_FORK         3 
#define SBI_STOP_REQ_FORK_MORE    4
#define SBI_STOP_REQ_FORK_DONE    5

struct sbi_snapshot_ret {
    uintptr_t utm_paddr;
    uintptr_t utm_size;
    uintptr_t dram_base;
    uintptr_t dram_size;
};

struct sbi_fork_ret {
    uintptr_t eid; //Eid of the child enclave 
    uintptr_t utm_paddr;
    uintptr_t dram_base;
    uintptr_t dram_size;
};

void
sbi_putchar(char c);
void
sbi_set_timer(uint64_t stime_value);
uintptr_t
sbi_stop_enclave(uint64_t request);
void
sbi_exit_enclave(uint64_t retval);
uintptr_t
sbi_random();
uintptr_t
sbi_query_multimem();
uintptr_t
sbi_query_multimem_addr();
uintptr_t
sbi_attest_enclave(void* report, void* buf, uintptr_t len);
uintptr_t
sbi_get_sealing_key(uintptr_t key_struct, uintptr_t key_ident, uintptr_t len);
uintptr_t
sbi_snapshot();
uintptr_t
sbi_fork();

#endif
