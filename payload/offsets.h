#ifndef FW
#error "You must define `FW` with the target firmware version!"
#endif

// SceKernelModuleMgr
#if   FW == 318
#define MODULEMGR_BACKTRACE_OFFSET     0x00040 // From .text base
#define MODULEMGR_SYSCALL_STUB_OFFSET  0x0850D // From .text base
#define MODULEMGR_SYSCALL_LO_OFFSET    0x0033C // From .data base
#elif FW >= 330 && FW <= 336 /* 3.30 and 3.35-3.36 are different modules, but the offsets are the same. */
#define MODULEMGR_BACKTRACE_OFFSET     0x00040 // From .text base
#define MODULEMGR_SYSCALL_STUB_OFFSET  0x08579 // From .text base
#define MODULEMGR_SYSCALL_LO_OFFSET    0x00340 // From .data base
#elif FW >= 350 && FW <= 352
#define MODULEMGR_BACKTRACE_OFFSET     0x00040 // From .text base
#define MODULEMGR_SYSCALL_STUB_OFFSET  0x08B15 // From .text base
#define MODULEMGR_SYSCALL_LO_OFFSET    0x0033C // From .data base
#elif FW >= 355 && FW <= 357
#define MODULEMGR_BACKTRACE_OFFSET     0x00040 // From .text base
#define MODULEMGR_SYSCALL_STUB_OFFSET  0x08B15 // From .text base
#define MODULEMGR_SYSCALL_LO_OFFSET    0x00338 // From .data base
#elif FW >= 360 && FW <= 361
#define MODULEMGR_BACKTRACE_OFFSET     0x00040 // From .text base
#define MODULEMGR_SYSCALL_STUB_OFFSET  0x08B45 // From .text base
#define MODULEMGR_SYSCALL_LO_OFFSET    0x00338 // From .data base
#elif FW >= 363 && FW <= 374
#define MODULEMGR_BACKTRACE_OFFSET     0x00040 // From .text base
#define MODULEMGR_SYSCALL_STUB_OFFSET  0x09FC5 // From .text base
#define MODULEMGR_SYSCALL_LO_OFFSET    0x2038C // From .data base
#endif

#ifdef FIX_NETPS_UAF
// SceNetPs
#if   FW >= 330 && FW <= 336 /* 3.30 and 3.35-3.36 are different modules, but the offsets are the same. */
#define NETPS_BNET_MUTEX_UNLOCK_OFFSET 0x29641 // from .text base
#define NETPS_BNET_JUMBO_MUTEX_OFFSET  0x00850 // from .data base
#elif FW == 350 /* unsure... */
#define NETPS_BNET_MUTEX_UNLOCK_OFFSET 0x2A149 // from .text base (different function, but this is what code calls, so...)
#define NETPS_BNET_JUMBO_MUTEX_OFFSET  0x00850 // from .data base
#elif FW >= 351 && FW <= 352
#define NETPS_BNET_MUTEX_UNLOCK_OFFSET 0x2A0A5 // from .text base
#define NETPS_BNET_JUMBO_MUTEX_OFFSET  0x00850 // from .data base
#elif FW >= 355 && FW <= 361 /* 3.61's non-dev module is different, but it's not exploitable, so we ignore it. */
#define NETPS_BNET_MUTEX_UNLOCK_OFFSET 0x2A099 // from .text base
#define NETPS_BNET_JUMBO_MUTEX_OFFSET  0x00850 // from .data base
#endif
#endif

// SceSysmem
#if   FW == 318
#define SYSMEM_PRINTF_OFFSET        0x19CF9 // From .text base
#define SYSMEM_TEXT2DATA_OFFSET     0x25910 // From .text base
#define SYSMEM_DATA2BASE_OFFSET     0x00070 // From .data base
#define SYSMEM_BACKTRACE_PTR_OFFSET 0x0438C // From .data base
#elif FW >= 330 && FW <= 336 /* 3.30 and 3.35-3.36 are different modules, but the offsets are the same. */
#define SYSMEM_PRINTF_OFFSET        0x19891 // From .text base
#define SYSMEM_TEXT2DATA_OFFSET     0x254F0 // From .text base
#define SYSMEM_DATA2BASE_OFFSET     0x00070 // From .data base
#define SYSMEM_BACKTRACE_PTR_OFFSET 0x0438C // From .data base
#elif FW >= 350 && FW <= 361 /* 3.50-3.52 and 3.55-3.61 are different modules, but the offsets are the same. */
#define SYSMEM_PRINTF_OFFSET        0x1A155 // From .text base
#define SYSMEM_TEXT2DATA_OFFSET     0x26A68 // From .text base
#define SYSMEM_DATA2BASE_OFFSET     0x000A0 // From .data base
#define SYSMEM_BACKTRACE_PTR_OFFSET 0x0438C // From .data base
#elif FW >= 363 && FW <= 374
#define SYSMEM_PRINTF_OFFSET        0x1A155 // From .text base
#define SYSMEM_TEXT2DATA_OFFSET     0x26A28 // From .text base
#define SYSMEM_DATA2BASE_OFFSET     0x000A0 // From .data base
#define SYSMEM_BACKTRACE_PTR_OFFSET 0x0438C // From .data base
#endif

// How to find each offset:
// - MODULEMGR_BACKTRACE_OFFSET - location of `SceBacktraceForDriver_888E99B8`.
// - MODULEMGR_SYSCALL_STUB_OFFSET - location of `func_noentry`, |1 for thumb. 
//   This is the only function that returns `0x8002710C`, used in the function that allocates system calls.
// - MODULEMGR_SYSCALL_LO_OFFSET - Variable ANDed with `0xFFF`, in the function that allocates system calls.
//   To find the function that allocates system calls, find the only function that calls `SceIntrmgrForKernel_B60ACF4B / SceIntrmgrForKernel_B5A665C7`.
//
// - SYSMEM_PRINTF_OFFSET - location of `sceKernelPrintf`, |1 for thumb (SceDebugForDriver_391B74B7)
// - SYSMEM_TEXT2DATA_OFFSET - location of `SceSysrootForDriver_4B8C305A` in the export table.
// - SYSMEM_DATA2BASE_OFFSET - location of `SceSysrootForDriver_4B8C305A`
// - SYSMEM_BACKTRACE_PTR_OFFSET - variable used in `SceDebugForKernel_EFF9962B / SceDebugForKernel_91E7D24F`.

// NIDs that were changed in 3.63+
#if   FW <= 361
#define MODULEMGR_REGISTERSYSCALL_NID    0xB427025E // SceModulemgrForKernel
#define MODULEMGR_GETMODULELIST_NID      0x97CF7B4E // SceModulemgrForKernel
#define MODULEMGR_GETMODULEINFO_NID      0xD269F915 // SceModulemgrForKernel
#define SYSMEM_ICACHEINVALIDATERANGE_NID 0x19F17BD0 // SceCpuForKernel
#elif FW >= 363 && FW <= 374
#define MODULEMGR_REGISTERSYSCALL_NID    0x2E4A10A0 // SceModulemgrForKernel
#define MODULEMGR_GETMODULELIST_NID      0xB72C75A4 // SceModulemgrForKernel
#define MODULEMGR_GETMODULEINFO_NID      0xDAA90093 // SceModulemgrForKernel
#define SYSMEM_ICACHEINVALIDATERANGE_NID 0x73E895EA // SceCpuForKernel
#endif
