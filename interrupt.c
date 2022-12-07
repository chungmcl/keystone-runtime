//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "regs.h"
#include "sbi.h"
#include "timex.h"
#include "interrupt.h"
#include "printf.h"
#include <asm/csr.h>
#include "fuzzy_buff.h"

#define DEFAULT_CLOCK_DELAY 10000
#include "syscall.h" // for debugging w/ print_strace() calls

void init_timer(void)
{
  sbi_set_timer(get_cycles64() + DEFAULT_CLOCK_DELAY);
  csr_set(sstatus, SR_SPIE);
  csr_set(sie, SIE_STIE | SIE_SSIE);
}

void handle_timer_interrupt()
{
  sbi_stop_enclave(0);
  unsigned long next_cycle = get_cycles64() + DEFAULT_CLOCK_DELAY;
  sbi_set_timer(next_cycle);
  csr_set(sstatus, SR_SPIE);
  return;
}

void handle_interrupts(struct encl_ctx* regs)
{
  print_strace("\thandle_interrupts called!\n");
  unsigned long cause = regs->scause;

  switch(cause) {
    case INTERRUPT_CAUSE_TIMER:
      handle_timer_interrupt();
      break;
    /* ignore other interrupts */
    case INTERRUPT_CAUSE_SOFTWARE:
      ipi_handle();
      break;
    case INTERRUPT_CAUSE_EXTERNAL:
    default:
      sbi_stop_enclave(0);
      return;
  }
}
