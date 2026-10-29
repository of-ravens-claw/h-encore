/* offsets.S -- user offsets
 *
 * Copyright (C) 2026 of-ravens-claw
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef FW
#error "You must define `FW` with the target firmware version!"
#endif

// SceLibKernel offsets
#if   FW >= 310 && FW <= 336
.equ SCE_LIB_KERNEL_OFFSET,  -0x975F
.equ GET_MODULE_LIST_OFFSET, 0x5B3C
.equ GET_MODULE_INFO_OFFSET, 0x5B4C
.equ GET_THREAD_INFO_OFFSET, 0x9A39
.equ DEVCTL_OFFSET,          0x9805
#elif FW >= 350 && FW <= 374
.equ SCE_LIB_KERNEL_OFFSET,  -0xA4B7
.equ GET_MODULE_LIST_OFFSET, 0x675C
.equ GET_MODULE_INFO_OFFSET, 0x676C
.equ GET_THREAD_INFO_OFFSET, 0xA791
.equ DEVCTL_OFFSET,          0xA55D
#endif

// SceNgsUser offsets
#if   FW >= 310 && FW <= 336
.equ SYSTEM_GET_SIZE_OFFSET, 0x54D
.equ SYSTEM_INIT_OFFSET,     0x57D
.equ RACK_GET_SIZE_OFFSET,   0x8D1
.equ RACK_INIT_OFFSET,       0x90D
.equ RACK_RELEASE_OFFSET,    0xA25
#elif FW >= 350 && FW <= 352
.equ SYSTEM_GET_SIZE_OFFSET, 0x54D
.equ SYSTEM_INIT_OFFSET,     0x57D
.equ RACK_GET_SIZE_OFFSET,   0x8E5
.equ RACK_INIT_OFFSET,       0x921
.equ RACK_RELEASE_OFFSET,    0xA39
#elif FW >= 355 && FW <= 374
.equ SYSTEM_GET_SIZE_OFFSET, 0x54D
.equ SYSTEM_INIT_OFFSET,     0x57D
.equ RACK_GET_SIZE_OFFSET,   0xB29
.equ RACK_INIT_OFFSET,       0xB65
.equ RACK_RELEASE_OFFSET,    0xDA1
#endif

// How to find each offset:
// - SCE_LIB_KERNEL_OFFSET - location of `add sp, #0xC` in `SceLibKernel_6C60AC61`, after call to `SceIofilemgr_CC67B6FD`
// - GET_MODULE_LIST_OFFSET - location of `SceModulemgr_2EF2581F` (import)
// - GET_MODULE_INFO_OFFSET - location of `SceModulemgr_36585DAF` (import)
// - GET_THREAD_INFO_OFFSET - location of `SceLibKernel_8D9C5461`
// - DEVCTL_OFFSET - location of `SceLibKernel_04B30CB2`
//
// - SYSTEM_GET_SIZE_OFFSET - location of `SceNgs_6CE8B36F`
// - SYSTEM_INIT_OFFSET - location of `SceNgs_ED14CF4A`
// - RACK_GET_SIZE_OFFSET - location of `SceNgs_477318C0`
// - RACK_INIT_OFFSET - location of `SceNgs_0A92E4EC`
// - RACK_RELEASE_OFFSET - location of `SceNgs_DD5CA10B`
