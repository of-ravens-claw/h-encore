#ifndef FW
#error "You must define `FW` with the target firmware version!"
#endif

// SceKernelModuleMgr
#if   FW >= 350 && FW <= 352
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

// SceSysmem
#if   FW >= 350 && FW <= 361 /* 3.50-3.52 and 3.55-3.61 are different modules, but the offsets are the same. */
#define SYSMEM_PRINTF_OFFSET                   0x1A155 // From .text base
#define SYSMEM_AUTOCLOCKDOWN_CTX_EXPORT_OFFSET 0x26A68 // From .text base
#define SYSMEM_AUTOCLOCKDOWN_CTX_OFFSET        0x000A0 // From .data base
#define SYSMEM_BACKTRACE_PTR_OFFSET            0x0438C // From .data base
#elif FW >= 363 && FW <= 374
#define SYSMEM_PRINTF_OFFSET                   0x1A155 // From .text base
#define SYSMEM_AUTOCLOCKDOWN_CTX_EXPORT_OFFSET 0x26A28 // From .text base
#define SYSMEM_AUTOCLOCKDOWN_CTX_OFFSET        0x000A0 // From .data base
#define SYSMEM_BACKTRACE_PTR_OFFSET            0x0438C // From .data base
#endif

// How to find each offset:
// - MODULEMGR_BACKTRACE_OFFSET - location of `SceBacktraceForDriver_888E99B8`.
// - MODULEMGR_SYSCALL_STUB_OFFSET - location of `func_noentry`, |1 for thumb. 
//   This is the only function that returns `0x8002710C`, used in the function that allocates system calls.
// - MODULEMGR_SYSCALL_LO_OFFSET - Variable ANDed with `0xFFF`, in the function that allocates system calls.
//   To find the function that allocates system calls, find the only function that calls `SceIntrmgrForKernel_B60ACF4B / SceIntrmgrForKernel_B5A665C7`.
//
// - SYSMEM_PRINTF_OFFSET - location of `sceKernelPrintf`, |1 for thumb (SceDebugForDriver_391B74B7)
// - SYSMEM_AUTOCLOCKDOWN_CTX_EXPORT_OFFSET - location of `SceSysrootForDriver_4B8C305A` in the export table.
// - SYSMEM_AUTOCLOCKDOWN_CTX_OFFSET - location of `SceSysrootForDriver_4B8C305A`
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
