# Copyright (c) 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Authors:
# - Paul Scheffler <paulsc@iis.ee.ethz.ch>
# - Philippe Sauter <phsauter@iis.ee.ethz.ch>

.globl _start
.section .text._start
_start:
  # Global pointer
  .option push
  .option norelax
  la      x3, __global_pointer$
  .option pop
  # Stack pointer
  la      x2, __stack_pointer$
  # Reset vector
  li      x1, 0
  li      x4, 0
  li      x5, 0
  li      x6, 0
  li      x7, 0
  li      x8, 0
  li      x9, 0
  li      x10, 0
  li      x11, 0
  li      x12, 0
  li      x13, 0
  li      x14, 0
  li      x15, 0
  # Setup interrupt handlers
  la      t0, __interupt_table
  csrw    mtvec, t0  # Set mtvec to point to our vector table
  # Call Main Function
  call main
  j _eoc

.globl _eoc
_eoc:
  la      t0, status
  sw      a0, 0(t0)
  wfi

# mtvec will point to the beginning of this table
# when an interrupt or exception occurs, it will jump to mtvec + exception id
# exception IDs can be found in cve2_pkg.sv
# Uses up some space and only fast interrupts and timer interrupts can be enabled
# consider pointing mtvec to to the table address - 15, so that only fast interrupts have an interrupt manager, but risky
.section .text.__interupt_table
.globl __interupt_table
.align 8
__interupt_table:
  .set i, 0
  .rept 32
    .if i == 19
      j handle_irq_19
    .else
      j _eoc
    .endif
    .set i, i + 1
  .endr


.section .text.handle_irq_19
.globl handle_irq_19
handle_irq_19:

  # Save caller-saved registers (optional depending on what you touch)
  addi sp, sp, -16
  sw ra, 0(sp)
  sw a5, 4(sp)

  # Read from DMA_REG(DMA_INTERRUPT_REG_OFFSET) to clear interrupt
  lui     a5,0x50000
  lw      a5,16(a5)
  # j dma_irq_handler # software version

  lw ra, 0(sp)
  lw a5, 4(sp)
  addi sp, sp, 16
  nop
  nop
  nop
  mret