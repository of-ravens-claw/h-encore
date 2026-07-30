/* koffsets.S -- kernel offsets
 *
 * Copyright (C) 2026 of-ravens-claw
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef FW
#error "You must define `FW` with the target firmware version!"
#endif

#if   FW >= 350 && FW <= 352
// Kernel offsets
.equ KSTACK_SIZE,                                  0x1000
.equ KSTACK_DEVCTL_INDATA_OFFSET,                  0x6F8
.equ KSTACK_SYSMEM_OFFSET,                         0x89C
.equ SCE_SYSMEM_OFFSET,                            -0x3E67

// call
.equ SceSysmem_blx_r0,                             0x1FB3F
.equ SceSysmem_blx_r4_pop_r4_pc,                   0x19FB9

// load
.equ SceSysmem_pop_pc,                             0x347
.equ SceSysmem_pop_r0_r1_pc,                       0x853
.equ SceSysmem_pop_r0_r1_r2_r3_r4_pc,              0x258FD
.equ SceSysmem_pop_r1_r2_r4_r6_pc,                 0x28E0B
.equ SceSysmem_pop_r3_pc,                          0x1605
.equ SceSysmem_pop_r3_r4_r5_pc,                    0x144B
.equ SceSysmem_pop_r3_r4_r5_r6_r7_pc,              0x102D
.equ SceSysmem_ldr_r0_r0_blx_r3,                   0xD51B
.equ SceSysmem_ldr_r2_r5_14_blx_r3,                0x1F697

// store
.equ SceSysmem_str_r0_r4_movs_r0_0_pop_r4_pc,      0x163D

// move
.equ SceSysmem_mov_sp_r1_blx_r2,                   0x67

// arithmetic
.equ SceSysmem_adds_r0_1_bx_lr,                    0x1A07
.equ SceSysmem_add_r1_sp_bc_mov_r2_r6_blx_r3,      0x1D97D
.equ SceSysmem_sub_r1_r1_r3_bx_lr,                 0x86D

// functions
.equ SceSysmem_sceKernelGetMemBlockBase,           0x1F15
.equ SceSysmem_sceKernelAllocMemBlockForKernel,    0xA521
.equ SceSysmem_sceKernelRemapMemBlock,             0xA74D
.equ SceSysmem_sceKernelFreeMemBlock,              0xAA41
.equ SceSysmem_sceDeflateDecompress,               0x1C3BB
.equ SceSysmem_sceKernelCopyFromUser,              0x825D
.equ SceSysmem_sceKernelDcacheCleanRangeForL1WBWA, 0x22FCD

// misc
.equ SceSysmem_empty_string,                       0x19
#elif FW >= 355 && FW <= 361
// Kernel offsets
.equ KSTACK_SIZE,                                  0x1000
.equ KSTACK_DEVCTL_INDATA_OFFSET,                  0x6F8
.equ KSTACK_SYSMEM_OFFSET,                         0x89C
.equ SCE_SYSMEM_OFFSET,                            -0x3E67

// call
.equ SceSysmem_blx_r0,                             0x1FB3F
.equ SceSysmem_blx_r4_pop_r4_pc,                   0x19FB9

// load
.equ SceSysmem_pop_pc,                             0x347
.equ SceSysmem_pop_r0_r1_pc,                       0x853
.equ SceSysmem_pop_r0_r1_r2_r3_r4_pc,              0x258FD
.equ SceSysmem_pop_r1_r2_r4_r6_pc,                 0x28DF3
.equ SceSysmem_pop_r3_pc,                          0x1B4D
.equ SceSysmem_pop_r3_r4_r5_pc,                    0x144B
.equ SceSysmem_pop_r3_r4_r5_r6_r7_pc,              0x103F
.equ SceSysmem_ldr_r0_r0_blx_r3,                   0xD51B
.equ SceSysmem_ldr_r2_r5_14_blx_r3,                0x1F697

// store
.equ SceSysmem_str_r0_r4_movs_r0_0_pop_r4_pc,      0x163D

// move
.equ SceSysmem_mov_sp_r1_blx_r2,                   0x67

// arithmetic
.equ SceSysmem_adds_r0_1_bx_lr,                    0x1A07
.equ SceSysmem_add_r1_sp_bc_mov_r2_r6_blx_r3,      0x1D97D
.equ SceSysmem_sub_r1_r1_r3_bx_lr,                 0x86D

// functions
.equ SceSysmem_sceKernelGetMemBlockBase,           0x1F15
.equ SceSysmem_sceKernelAllocMemBlockForKernel,    0xA521
.equ SceSysmem_sceKernelRemapMemBlock,             0xA74D
.equ SceSysmem_sceKernelFreeMemBlock,              0xAA41
.equ SceSysmem_sceDeflateDecompress,               0x1C3BB
.equ SceSysmem_sceKernelCopyFromUser,              0x825D
.equ SceSysmem_sceKernelDcacheCleanRangeForL1WBWA, 0x22FCD

// misc
.equ SceSysmem_empty_string,                       0x19
#elif FW >= 363 && FW <= 374
// Kernel offsets
.equ KSTACK_SIZE,                                  0x1000
.equ KSTACK_DEVCTL_INDATA_OFFSET,                  0x6F8
.equ KSTACK_SYSMEM_OFFSET,                         0x89C
.equ SCE_SYSMEM_OFFSET,                            -0x1697

// call
.equ SceSysmem_blx_r0,                             0x1FB3F
.equ SceSysmem_blx_r4_pop_r4_pc,                   0x19FB9

// load
.equ SceSysmem_pop_pc,                             0x347
.equ SceSysmem_pop_r0_r1_pc,                       0x853
.equ SceSysmem_pop_r0_r1_r2_r3_r4_pc,              0x258BD
.equ SceSysmem_pop_r1_r2_r4_r6_pc,                 0x288FF
.equ SceSysmem_pop_r3_pc,                          0x1D8F
.equ SceSysmem_pop_r3_r4_r5_pc,                    0x1377
.equ SceSysmem_pop_r3_r4_r5_r6_r7_pc,              0xF59
.equ SceSysmem_ldr_r0_r0_blx_r3,                   0x11337
.equ SceSysmem_ldr_r2_r5_14_blx_r3,                0x1F697

// store
.equ SceSysmem_str_r0_r4_movs_r0_0_pop_r4_pc,      0x4F71

// move
.equ SceSysmem_mov_sp_r1_blx_r2,                   0x67

// arithmetic
.equ SceSysmem_adds_r0_1_bx_lr,                    0x533B
.equ SceSysmem_add_r1_sp_bc_mov_r2_r6_blx_r3,      0x1D97D
.equ SceSysmem_sub_r1_r1_r3_bx_lr,                 0x86D

// functions
.equ SceSysmem_sceKernelGetMemBlockBase,           0x5849
.equ SceSysmem_sceKernelAllocMemBlockForKernel,    0x7D3D
.equ SceSysmem_sceKernelRemapMemBlock,             0x7F69
.equ SceSysmem_sceKernelFreeMemBlock,              0x825D
.equ SceSysmem_sceDeflateDecompress,               0x1C3BB
.equ SceSysmem_sceKernelCopyFromUser,              0x6289
.equ SceSysmem_sceKernelDcacheCleanRangeForL1WBWA, 0x22FCD

// misc
.equ SceSysmem_empty_string,                       0x19
#endif
