#include <stddef.h>
#include <stdint.h>

#include "bootstrap.h"

#include "defines.h"

// During development it may be useful to run the payload multiple times without rebooting.
// Or, to run the payload with hooks already in place so debugging is possible.
// Or, to be able to run it without removing ensō first.
// It is probably a good idea to comment this for releases.
//#define DONT_LOAD_TAIHEN_TWICE

// Comment this out for releases!
// TODO: For some reason, this always freezes my system.
// Is it because CatLog conflicts? Would it work better over UART?
//#define DEBUG

// Define this if you'd like to log to a file, as opposed to logging to UART.
// NOTE: This will only work after the kernel thread has been started.
#define FILE_LOGGING

// Define this if you're using the SceNgs exploit (used by h-encore)
#define FIX_NGS

// Define this if you're using the SceNetPs use-after-free exploit (used by HENkaku)
//#define FIX_NETPS_UAF

// Define this if you're running from a syscall context (required for h-encore)
#define RUNNING_IN_SYSCALL_CONTEXT

// Define this if you'd like to modify the framebuffer address passed in r2.
// NOTE: your kernel rop chain _must_ pass a valid framebuffer address in r2 for this to work.
#define MODIFY_FRAMEBUFFER

// For HENkaku, define:
// - FIX_NETPS_UAF

// For h-encore, define:
// - FIX_NGS
// - RUNNING_IN_SYSCALL_CONTEXT
// - MODIFY_FRAMEBUFFER (optional)

#include "offsets.h"

#ifdef DEBUG
#define LOG(fmt, ...) sceKernelPrintf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

#define DACR_OFF(stmt)                                                        \
do {                                                                          \
	uint32_t prev_dacr;                                                   \
	__asm__ volatile("mrc p15, 0, %0, c3, c0, 0 \n" : "=r"(prev_dacr));   \
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0 \n" : : "r"(0xFFFF0000)); \
	stmt;                                                                 \
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0 \n" : : "r"(prev_dacr));  \
} while (0)

// Encodes the following instruction sequence, in ARM mode:
// - LDR PC, [PC, #0]
// - NOP              @ This instruction will never be executed.
// - DCD addr         @ 32-bit address to jump to.
// Corrupts 12 bytes.
#define INSTALL_HOOK(func, addr)              \
do {                                          \
	uint32_t* target = (uint32_t*)(addr); \
	target[0] = 0xE59FF000;               \
	target[1] = 0xE320F000;               \
	target[2] = (uint32_t)(func);         \
} while (0)

#define ENTER_SYSCALL(state)                                                            \
do {                                                                                    \
	__asm__ volatile("mrc p15, 0, %0, c13, c0, 3" : "=r"(state));                   \
	__asm__ volatile("mcr p15, 0, %0, c13, c0, 3" : : "r"(state << 16) : "memory"); \
} while (0)

#define EXIT_SYSCALL(state)                                                       \
do {                                                                              \
	__asm__ volatile("mcr p15, 0, %0, c13, c0, 3" : : "r"(state) : "memory"); \
} while (0)

typedef struct segment_info
{
	int  size;   // this structure size (0x18)
	int  perms;  // probably rwx in low bits
	void *vaddr; // address in memory
	int  memsz;  // size in memory
	int  flags;  // meanig unknown
	int  res;    // unused?
} segment_info_t;

typedef struct SceKernelModuleInfo
{
	int            size;            // 0x00
	int            UID;             // 0x04
	int            mod_attr;        // 0x08
	char           name[0x1C];      // 0x0C
	uint32_t       unk0;            // 0x28
	void           *module_start;   // 0x2C addr0
	void           *module_init;    // 0x30 addr1
	void           *module_stop;    // 0x34 addr2
	void           *exidx_start;    // 0x38 addr3
	void           *exidx_end;      // 0x3C addr4
	void           *addr5;          // 0x40 addr5
	void           *addr6;          // 0x44 addr6
	void           *module_top;     // 0x48 addr7
	void           *addr8;          // 0x4C addr8
	void           *addr9;          // 0x50 addr9
	char           filepath[0x100]; // 0x54
	segment_info_t segments[4];     // 0x58
	uint32_t          unk2;            // 0x1B4
} SceKernelModuleInfo; // 0x1B8

typedef struct module_imports_2
{
	uint16_t size; // 0x24
	uint16_t version;
	uint16_t flags;
	uint16_t num_functions;
	uint32_t reserved1;
	uint32_t lib_nid;
	char  *lib_name;
	uint32_t *func_nid_table;
	void  **func_entry_table;
	uint32_t unk1;
	uint32_t unk2;
} module_imports_2_t;

typedef struct module_exports // thanks roxfan
{
	uint16_t size;           // size of this structure; 0x20 for Vita 1.x
	uint8_t  lib_version[2]; //
	uint16_t attribute;      // ?
	uint16_t num_functions;  // number of exported functions
	uint32_t num_vars;       // number of exported variables
	uint32_t num_tls_vars;   // number of exported TLS variables?  <-- pretty sure wrong // yifanlu
	uint32_t module_nid;     // NID of this specific export list; one PRX can export several names
	char  *lib_name;      // name of the export module
	uint32_t *nid_table;     // array of 32-bit NIDs for the exports, first functions then vars
	void  **entry_table;  // array of pointers to exported functions and then variables
} module_exports_t;

typedef struct module_info // thanks roxfan
{
	uint16_t modattribute;  // ??
	uint16_t modversion;    // always 1,1?
	char  modname[27];   ///< Name of the module
	uint8_t  type;          // 6 = user-mode prx?
	void  *gp_value;     // always 0 on ARM
	uint32_t ent_top;       // beginning of the export list (sceModuleExports array)
	uint32_t ent_end;       // end of same
	uint32_t stub_top;      // beginning of the import list (sceModuleStubInfo array)
	uint32_t stub_end;      // end of same
	uint32_t module_nid;    // ID of the PRX? seems to be unused
	uint32_t field_38;      // unused in samples
	uint32_t field_3C;      // I suspect these may contain TLS info
	uint32_t field_40;      //
	uint32_t mod_start;     // 44 module start function; can be 0 or -1; also present in exports
	uint32_t mod_stop;      // 48 module stop function
	uint32_t exidx_start;   // 4c ARM EABI style exception tables
	uint32_t exidx_end;     // 50
	uint32_t extab_start;   // 54
	uint32_t extab_end;     // 58
} module_info_t; // 5c?

int strcmp(const char* s1, const char* s2)
{
	while (*s1 == *s2++)
	{
		if (*s1++ == 0)
			return 0;
	}

	return *(unsigned char*)s1 - *(unsigned char*)--s2;
}

void* memcpy(void* dst, const void* src, size_t len)
{
	char* d = (char*)dst;
	const char* s = (const char*)src;
	while (len-- > 0)
		*d++ = *s++;

	return dst;
}

void* memset(void* dst, int val, size_t len)
{
	char* p = (char*)dst;
	while (len-- > 0)
		*p++ = val;

	return dst;
}

module_info_t* find_modinfo(SceUIntPtr start_addr, const char* needle)
{
	start_addr &= ~0xF;
	start_addr += 4;

	while (1)
	{
		if (strcmp((const char*)start_addr, needle) == 0)
			 return (module_info_t*)(start_addr - 0x4);

		start_addr += 0x10;
	}

	return NULL;
}

void* find_export(module_info_t* mod, uint32_t nid)
{
	if (!mod->ent_top)
		return NULL;

	for (unsigned ent = mod->ent_top; ent < mod->ent_end; )
	{
		module_exports_t* exports = (module_exports_t*)((unsigned int)mod + 0x5C + ent - mod->ent_top);

		for (int j = 0; j < exports->num_functions; j++)
		{
			if (exports->nid_table[j] == nid)
				return (void*)((uint32_t*)exports->entry_table)[j];
		}
		ent += exports->size;
	}

	return NULL;
}

void* find_import(module_info_t* mod, uint32_t lnid, uint32_t fnid)
{
	if (!mod->stub_top)
		return NULL;

	for (unsigned stub = mod->stub_top; stub < mod->stub_end;)
	{
		// Older fw versions use a different import format
		// TODO: Support both.
		module_imports_2_t* imports = (module_imports_2_t*)((unsigned int)mod + 0x5C + stub - mod->stub_top);

		if (imports->size == sizeof(module_imports_2_t) && imports->lib_nid == lnid)
		{
			for (int j = 0; j < imports->num_functions; ++j)
			{
				if (imports->func_nid_table[j] == fnid)
					return (void*)((uint32_t*)imports->func_entry_table)[j];
			}
		}

		stub += imports->size;
	}

	return NULL;
}

#ifdef DEBUG
// SceSysmem
static int (*sceKernelPrintf)(const char* fmt, ...) = NULL;
#endif

static unsigned (*orig_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp)(unsigned a1, unsigned a2, unsigned a3, unsigned a4) = NULL;
static unsigned (*orig_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp)(unsigned a1, unsigned a2) = NULL;
static unsigned (*orig_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp)(unsigned a1, unsigned a2, unsigned a3) = NULL;

// SceKernelModulemgr
static int (*sceKernelGetModuleList)(SceUID pid, int flags1, int flags2, SceUID* modids, SceSize* num) = NULL; // guessed name.
static int (*sceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo* info) = NULL; // guessed name.
static SceUID (*sceKernelLoadModule)(const char* moduleFileName, SceUInt32 flags, const SceKernelLoadModuleOpt* pOpt) = NULL;
static int (*sceKernelStartModule)(SceUID uid, SceSize args, const void* argp, SceUInt32 flags, const SceKernelStartModuleOpt* pOpt, int *pRes) = NULL;
static void (*sceKernelRegisterSyscall)(SceUInt32 index, void* function) = NULL; // guessed name.

// SceSysmem
static int (*sceKernelIcacheInvalidateRange)(void* start, SceSize size) = NULL;
static int (*sceKernelDcacheCleanRangeForL1WBWA)(void* start, SceSize size) = NULL;
static int (*sceKernelFreeMemBlock)(SceUID uid) = NULL;
static SceUID (*sceKernelFindMemBlockByAddr)(void* pBase, SceSize size) = NULL;
static int (*sceKernelGetMemBlockBase)(SceUID uid, void** ppBase) = NULL;
static SceUID (*sceKernelSysrootGetShellPid)(void) = NULL;

// SceIofilemgr
static SceUID (*sceIoOpen)(const char* filename, int flags, int mode) = NULL;
static SceSSize (*sceIoWrite)(SceUID fd, const void* data, SceSize size) = NULL;
static int (*sceIoClose)(SceUID fd) = NULL;

// SceAppMgr
static int (*sceAppMgrLaunchAppByPath)(const char* path, const char* args, SceSize arg_size, unsigned int type, SceAppMgrLaunchParam* pParam, void*) = NULL;

// SceKernelThreadmgr
static SceUID (*sceKernelCreateThread)(const char* pName, SceKernelThreadEntry entry, SceInt32 initPriority, SceSize stackSize, SceUInt32 attr, SceInt32 cpuAffinityMask, const SceKernelThreadOptParam* pOptParam) = NULL;
static SceInt32 (*sceKernelStartThread)(SceUID threadId, SceSize argSize, const void* pArgBlock) = NULL;
static SceInt32 (*sceKernelExitDeleteThread)(SceInt32 exitStatus) = NULL;

// SceProcessmgr
static int (*sceKernelGetProcessInfo)(SceUID pid, SceKernelProcessInfo* pInfo) = NULL;
static int (*sceKernelGetRemoteKPLS)(SceUID pid, SceUInt32 key, void** kpls_addr, SceBool force_create) = NULL;

// SceSblACMgr
static SceBool (*sceSblACMgrIsDevelopmentMode)(void) = NULL;

// context for the hooks
static SceBool g_homebrew_decrypt = SCE_FALSE;
static module_info_t* modulemgr_info = NULL;
static module_info_t* scenpdrm_info = NULL;
static module_info_t* appmgr_info = NULL;
static SceUIntPtr appmgr_code = 0;
static SceUIntPtr appmgr_size = 0;
static SceUIntPtr appmgr_data = 0;

#ifdef FIX_NGS
static SceUIntPtr ngs_data = 0;
#endif

#ifdef FIX_NETPS_UAF
static SceUIntPtr netps_code = 0;
static SceUIntPtr netps_data = 0;
#endif

// save the block id of our own memory
static int g_rx_block = 0;
static int* syscall_lo = NULL;
static void* syscall_stub = NULL;
static int syscall_id = 0;

#ifdef DONT_LOAD_TAIHEN_TWICE
static SceBool has_taihen = SCE_FALSE;
#endif

#ifdef FILE_LOGGING

#define LOG_LOC "ux0:payload.log"

size_t my_strlen(const char* str)
{
	const char* s = str;
	while (*s != '\0')
		s++;

	return s - str;
}

static int (*ptr_snprintf)(char* str, size_t size, const char* format, ...) = NULL;

#define FLOG(fmt, ...) \
do { \
	char buffer[256]; \
	buffer[0] = '\0'; \
	ptr_snprintf(buffer, sizeof(buffer), "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
	logg(buffer, my_strlen(buffer), LOG_LOC, 2); \
} while (0)

#define FLOG_START(fmt, ...) \
do { \
	char buffer[256]; \
	buffer[0] = '\0'; \
	ptr_snprintf(buffer, sizeof(buffer), "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
	logg(buffer, my_strlen(buffer), LOG_LOC, 1); \
} while (0)

typedef enum bare_SceIoMode
{
	SCE_O_WRONLY = 0x0002, //!< Write-only
	SCE_O_APPEND = 0x0100, //!< Append
	SCE_O_CREAT  = 0x0200, //!< Create
	SCE_O_TRUNC  = 0x0400, //!< Truncate
} bare_SceIoMode;

static int logg(const void* buffer, int length, const char* logloc, int create)
{
	if (!sceIoOpen || !sceIoWrite || !sceIoClose)
		return 0;

	int fd;

	if (create == 0)
		fd = sceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND, 6);
	else if (create == 1)
		fd = sceIoOpen(logloc, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	else if (create == 2)
		fd = sceIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 6);
	else
		fd = -1;
	
	if (fd < 0)
		return 0;

	sceIoWrite(fd, buffer, length);
	sceIoClose(fd);
	return 1;
}
#else
#define FLOG(...)
#define FLOG_START(...)
#endif

int hook_sceSblAIMgrIsCEX_SceNpDrm_imp(void)
{
	return 1;
}

int hook_sceSblAIMgrIsDEX_SceNpDrm_imp(void)
{
	return 1;
}

int hook_SceSysrootForDriver_421EFC96_SceAppMgr_imp(void)
{
	return 0;
}

int hook_sceSblAIMgrIsCEX_SceAppMgr_imp(void)
{
	return 0;
}

int hook_sceSblACMgrIsDevelopmentMode_SceAppMgr_imp(void)
{
	return 1;
}

int hook_SceProcessmgrForDriver_C715591F_SceAppMgr_imp(void)
{
	return 0;
}

// setup file decryption
unsigned hook_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp(unsigned a1, unsigned a2, unsigned a3, unsigned a4)
{
	LOG("sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp(0x%x, 0x%x, 0x%x, 0x%x)", a1, a2, a3, a4);
	FLOG("sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp(0x%08X, 0x%08X, 0x%08X, 0x%08X)", a1, a2, a3, a4);

	unsigned res = orig_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp(a1, a2, a3, a4);
	LOG("sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp: %x", res);
	FLOG("sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp: 0x%08X", res);
	unsigned* somebuf = (unsigned*)a4;

	if (res == 0x800F0624 || res == 0x800F0616 || res == 0x800F0024 || (res >= 0x800F0B30 && res <= 0x800F0B3F))
	{
		DACR_OFF(g_homebrew_decrypt = SCE_TRUE);
		somebuf[42] = 0x40; // Known to work on 1.60-3.74, probably never changed...?
		return 0;
	}
	else
	{
		DACR_OFF(g_homebrew_decrypt = SCE_FALSE);
	}
	return res;
}

// setup output buffer
unsigned hook_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp(unsigned a1, unsigned a2)
{
	LOG("sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp(0x%x, 0x%x) hb=0x%x", a1, a2, g_homebrew_decrypt);
	FLOG("sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp(0x%08X, 0x%08X) 0x%08X", a1, a2, g_homebrew_decrypt);
	if (g_homebrew_decrypt == SCE_TRUE)
		return 2; // always compressed!
	return orig_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp(a1, a2);
}

// decrypt
unsigned hook_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp(unsigned a1, unsigned a2, unsigned a3)
{
	LOG("sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp(0x%x, 0x%x, 0x%x) hb=0x%x", a1, a2, a3, g_homebrew_decrypt);
	FLOG("sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp(0x%08X, 0x%08X, 0x%08X) hb=0x%08X", a1, a2, a3, g_homebrew_decrypt);
	if (g_homebrew_decrypt == SCE_TRUE)
		return 0;
	return orig_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp(a1, a2, a3);
}

static const char ur0_temp_path[] = "ur0:temp";

static SceBool has_sigpatches = SCE_FALSE;
static char    old_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp[16] = {0};
static char    old_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp[16] = {0};
static char    old_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp[16] = {0};
static char    old_sceSblAIMgrIsCEX_SceNpDrm_imp[16] = {0};
static char    old_sceSblAIMgrIsDEX_SceNpDrm_imp[16] = {0};
static char    old_SceSysrootForDriver_421EFC96_SceAppMgr_imp[16] = {0};
static char    old_sceSblAIMgrIsCEX_SceAppMgr_imp[16] = {0};
static char    old_sceSblACMgrIsDevelopmentMode_SceAppMgr_imp[16] = {0};
static char    old_SceProcessmgrForDriver_C715591F_SceAppMgr_imp[16] = {0};
static char    old_ux0_data_path[9] = {0};
static void*   ux0_data_path_addr = NULL;

void temp_pkgpatches(void)
{
	void* addr = NULL;
	LOG("inserting temporary package patches");

	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xD78B04A2);
	LOG("sceSblAIMgrIsCEX_SceNpDrm_imp stub: %p", addr);
	DACR_OFF(
		memcpy(old_sceSblAIMgrIsCEX_SceNpDrm_imp, addr, 16);
		INSTALL_HOOK(hook_sceSblAIMgrIsCEX_SceNpDrm_imp, addr);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("hooked sceSblAIMgrIsCEX_SceNpDrm_imp");

	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xF4B98F66);
	LOG("sceSblAIMgrIsDEX_SceNpDrm_imp stub: %p", addr);
	DACR_OFF(
		memcpy(old_sceSblAIMgrIsDEX_SceNpDrm_imp, addr, 16);
		INSTALL_HOOK(hook_sceSblAIMgrIsDEX_SceNpDrm_imp, addr);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("hooked sceSblAIMgrIsDEX_SceNpDrm_imp");

	__asm__ volatile("isb" : : : "memory");
}

void remove_pkgpatches(void)
{
	void* addr = NULL;
	LOG("removing temporary package patches");

	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xD78B04A2);
	LOG("sceSblAIMgrIsCEX_SceNpDrm_imp stub: %p", addr);
	DACR_OFF(
		memcpy(addr, old_sceSblAIMgrIsCEX_SceNpDrm_imp, 16);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("unhooked sceSblAIMgrIsCEX_SceNpDrm_imp");

	addr = find_import(scenpdrm_info, 0xFD00C69A, 0xF4B98F66);
	LOG("sceSblAIMgrIsDEX_SceNpDrm_imp stub: %p", addr);
	DACR_OFF(
		memcpy(addr, old_sceSblAIMgrIsDEX_SceNpDrm_imp, 16);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("unhooked sceSblAIMgrIsDEX_SceNpDrm_imp");

	__asm__ volatile("isb" : : : "memory");
}

void temp_sigpatches(void)
{
	void* addr = NULL;
	LOG("inserting temporary signature patches");

	addr = find_import(modulemgr_info, 0x7ABF5135, 0xF3411881);
	LOG("sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp stub: %p\n", addr);
	DACR_OFF(
		memcpy(old_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp, addr, 16);
		INSTALL_HOOK(hook_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp, addr);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("hooked sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp");

	addr = find_import(modulemgr_info, 0x7ABF5135, 0x89CCDA2C);
	LOG("sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp stub: %p", addr);
	DACR_OFF(memcpy(old_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp, addr, 16));
	DACR_OFF(
		memcpy(old_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp, addr, 16);
		INSTALL_HOOK(hook_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp, addr);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("hooked sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp");

	addr = find_import(modulemgr_info, 0x7ABF5135, 0xBC422443);
	LOG("sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp stub: %p", addr);
	DACR_OFF(
		memcpy(old_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp, addr, 16);
		INSTALL_HOOK(hook_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp, addr);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("hooked sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp");

	addr = find_import(appmgr_info, 0x2ED7F97A, 0x421EFC96);
	LOG("SceSysrootForDriver_421EFC96_SceAppMgr_imp stub: %p", addr);
	DACR_OFF(
		memcpy(old_SceSysrootForDriver_421EFC96_SceAppMgr_imp, addr, 16);
		INSTALL_HOOK(hook_SceSysrootForDriver_421EFC96_SceAppMgr_imp, addr);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("hooked SceSysrootForDriver_421EFC96_SceAppMgr_imp");

	// To bypass 0x8080004C error on FW 3.60+
	addr = find_import(appmgr_info, 0xFD00C69A, 0xD78B04A2);
	LOG("sceSblAIMgrIsCEX_SceAppMgr_imp stub: %p", addr);
	if (addr)
	{
		DACR_OFF(
			memcpy(old_sceSblAIMgrIsCEX_SceAppMgr_imp, addr, 16);
			INSTALL_HOOK(hook_sceSblAIMgrIsCEX_SceAppMgr_imp, addr);
			sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
			sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		);
		LOG("hooked sceSblAIMgrIsCEX_SceAppMgr_imp");
	}

	// To bypass 0x80020012 error on FW 3.70+
	addr = find_import(appmgr_info, 0x9AD8E213, 0xE87D1777);
	LOG("sceSblACMgrIsDevelopmentMode_SceAppMgr_imp stub: %p", addr);
	if (addr)
	{
		DACR_OFF(
			memcpy(old_sceSblACMgrIsDevelopmentMode_SceAppMgr_imp, addr, 16);
			INSTALL_HOOK(hook_sceSblACMgrIsDevelopmentMode_SceAppMgr_imp, addr);
			sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
			sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		);
		LOG("hooked sceSblACMgrIsDevelopmentMode_SceAppMgr_imp");
	}

	// To bypass 0x80802025 error on FW 3.50
	// TODO: I really don't like this, is there no better way..?
	addr = find_import(appmgr_info, 0x746EC971, 0xC715591F);
	LOG("SceProcessmgrForDriver_C715591F_SceAppMgr_imp stub: %p", addr);
	if (addr)
	{
		DACR_OFF(
			memcpy(old_SceProcessmgrForDriver_C715591F_SceAppMgr_imp, addr, 16);
			INSTALL_HOOK(hook_SceProcessmgrForDriver_C715591F_SceAppMgr_imp, addr);
			sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
			sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		);
		LOG("hooked SceProcessmgrForDriver_C715591F_SceAppMgr_imp");
	}

	// TODO: What is the purpose of this patch?
	// Seems to be inherited from HENkaku, to allow launching the bootstrap from ur0.
	// ...but we don't need this, since we target a save game exploit, which requires a memory card.
	// TODO: Remove this and see if anything breaks.
	// TODO: Also remove the path to load the bootstrap from ur0 (that code is located elsewhere.)
	DACR_OFF(
		memcpy(old_ux0_data_path, ux0_data_path_addr, sizeof(old_ux0_data_path));
		memcpy(ux0_data_path_addr, ur0_temp_path, sizeof(ur0_temp_path));
	);
	LOG("hooked ux0:data path");

	DACR_OFF(has_sigpatches = SCE_TRUE);

	__asm__ volatile("isb" : : : "memory");
}

void remove_sigpatches(void)
{
	if (!has_sigpatches)
		return;

	void* addr = NULL;
	LOG("removing temporary signature patches");

	addr = find_import(modulemgr_info, 0x7ABF5135, 0xF3411881);
	LOG("sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp stub: %p", addr);
	DACR_OFF(
		memcpy(addr, old_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp, 16);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("unhooked sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp");

	addr = find_import(modulemgr_info, 0x7ABF5135, 0x89CCDA2C);
	LOG("sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp stub: %p", addr);
	DACR_OFF(
		memcpy(addr, old_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp, 16);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("unhooked sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp");

	addr = find_import(modulemgr_info, 0x7ABF5135, 0xBC422443);
	LOG("sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp stub: %p", addr);
	DACR_OFF(
		memcpy(addr, old_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp, 16);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("unhooked sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp");

	addr = find_import(appmgr_info, 0x2ED7F97A, 0x421EFC96);
	LOG("SceSysrootForDriver_421EFC96_SceAppMgr_imp stub: %p", addr);
	DACR_OFF(
		memcpy(addr, old_SceSysrootForDriver_421EFC96_SceAppMgr_imp, 16);
		sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
	);
	LOG("unhooked SceSysrootForDriver_421EFC96_SceAppMgr_imp");

	addr = find_import(appmgr_info, 0xFD00C69A, 0xD78B04A2);
	LOG("sceSblAIMgrIsCEX_SceAppMgr_imp stub: %p", addr);
	if (addr)
	{
		DACR_OFF(
			memcpy(addr, old_sceSblAIMgrIsCEX_SceAppMgr_imp, 16);
			sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
			sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		);
		LOG("unhooked sceSblAIMgrIsCEX_SceAppMgr_imp");
	}

	addr = find_import(appmgr_info, 0x9AD8E213, 0xE87D1777);
	if (addr)
	{
		LOG("sceSblACMgrIsDevelopmentMode_SceAppMgr_imp stub: %p", addr);
		DACR_OFF(
			memcpy(addr, old_sceSblACMgrIsDevelopmentMode_SceAppMgr_imp, 16);
			sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
			sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		);
		LOG("unhooked sceSblACMgrIsDevelopmentMode_SceAppMgr_imp");
	}

	addr = find_import(appmgr_info, 0x746EC971, 0xC715591F);
	LOG("SceProcessmgrForDriver_C715591F_SceAppMgr_imp stub: %p", addr);
	if (addr)
	{
		DACR_OFF(
			memcpy(addr, old_SceProcessmgrForDriver_C715591F_SceAppMgr_imp, 16);
			sceKernelIcacheInvalidateRange((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
			sceKernelDcacheCleanRangeForL1WBWA((void*)((SceUIntPtr)addr & ~0x1F), 0x20);
		);
		LOG("unhooked SceProcessmgrForDriver_C715591F_SceAppMgr_imp");
	}

	DACR_OFF(memcpy(ux0_data_path_addr, old_ux0_data_path, sizeof(old_ux0_data_path)));
	LOG("unhooked ux0:data path");

	DACR_OFF(has_sigpatches = SCE_FALSE);

	__asm__ volatile("isb" : : : "memory");
}

static void find_ux0_data_path_addr(void)
{
	char* from = (char*)(appmgr_code + appmgr_info->stub_top);
	char* to   = (char*)(appmgr_code + appmgr_size);

	while (strcmp(from, "ux0:data") != 0 && from < (to - 8))
		from += 4;

	DACR_OFF(ux0_data_path_addr = from);
}

int load_taihen(void)
{
	int state;
	ENTER_SYSCALL(state);

#ifdef DONT_LOAD_TAIHEN_TWICE
	if (has_taihen)
	{
		LOG("taiHEN was already loaded and DONT_LOAD_TAIHEN_TWICE is defined!");
		LOG("Removing sigpatches and returning success!");
		remove_sigpatches();
		EXIT_SYSCALL(state);
		return 0;
	}
#endif

	SceKernelLoadModuleOpt opt;
	opt.size = sizeof(SceKernelLoadModuleOpt);

	// load taiHEN
	SceUID taiid = sceKernelLoadModule("ur0:tai/taihen.skprx", 0, &opt);
	LOG("load taiHEN: 0x%08X", taiid);
	remove_sigpatches();
	LOG("Removed temp patches");
	int result = 0;
	int ret = sceKernelStartModule(taiid, 0, NULL, 0, NULL, &result);
	LOG("start taiHEN: 0x%08X, 0x%08X", ret, result);
	
	if (ret == 0)
		ret = result;
	if (ret < 0)
		goto end;

	opt.size = sizeof(SceKernelLoadModuleOpt);
	
	// load HENkaku kernel
	SceUID modid = sceKernelLoadModule("ur0:tai/henkaku.skprx", 0, &opt);
	LOG("load HENkaku kernel: 0x%08X", modid);
	result = 0;
	SceUID shell_pid = sceKernelSysrootGetShellPid();
	LOG("shell_pid: 0x%08X", shell_pid);
	FLOG("shell_pid: 0x%08X", shell_pid);
	ret = sceKernelStartModule(modid, sizeof(SceUID), &shell_pid, 0, NULL, &result);
	LOG("start HENkaku kernel: 0x%08X, 0x%08X", ret, result);

	if (ret == 0)
		ret = result;
	if (ret < 0)
		goto end;

end:
	EXIT_SYSCALL(state);
	return ret;
}

static void __attribute__((noinline, naked)) free_and_exit(SceUID blk, void* free, void* lr)
{
	// now free the executable memory.
	// this frees our current function so we have to ensure we do not return here
	__asm__ volatile(
		"mov lr, %2\n\t"
		"bx %1\n\t"
		 :
		 : "r"(blk),
		   "r"(free),
		   "r"(lr)
		 : "lr"
	);
}

void cleanup_memory(void)
{
	void* lr = NULL;
	__asm__ volatile("mov %0, lr" : "=r"(lr));

	LOG("calling cleanup from %x", lr);
	FLOG("calling cleanup from 0x%08X", lr);
	// TODO: Delete bootstrap.self (#37)

	// remove syscalls
	LOG("removing syscalls");
	FLOG("removing syscalls");
	sceKernelRegisterSyscall(syscall_id + 0, syscall_stub);
	sceKernelRegisterSyscall(syscall_id + 1, syscall_stub);
	sceKernelRegisterSyscall(syscall_id + 2, syscall_stub);
	sceKernelRegisterSyscall(syscall_id + 3, syscall_stub);
	sceKernelRegisterSyscall(syscall_id + 4, syscall_stub);
	if ((*syscall_lo) == (syscall_id + 5))
		*syscall_lo = syscall_id;

	LOG("freeing executable memory");
	FLOG("freeing executable memory");
	return free_and_exit(g_rx_block, sceKernelFreeMemBlock, lr);
}

int mount_savedata(SceUID pid)
{
	void* (*sceAppMgrFindProcessInfoByPid)(void* data, SceUID pid) = NULL;
	int   (*sceAppMgrMountById)(SceUID pid, void* info, int id, const char* titleid, const char* path,
	                            const char* desired_mount_point, const void* klicensee, char* mount_point) = NULL;

	int info_offset = 0;	
	switch (appmgr_info->module_nid)
	{
		case 0x97A7FDCC: // 3.18
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x3DC9);
			sceAppMgrMountById = (void*)(appmgr_code + 0x1CB79);
			info_offset = 0x578;
			break;

		case 0xF1102066: // 3.30
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x3E01);
			sceAppMgrMountById = (void*)(appmgr_code + 0x1CEFD);
			info_offset = 0x578;
			break;

		case 0xA1BE0ED6: // 3.35
		case 0x6ABD8ABA: // 3.36
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x3E01);
			sceAppMgrMountById = (void*)(appmgr_code + 0x1CF29);
			info_offset = 0x578;
			break;

		case 0xE2EDFCC3: // 3.50
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE1);
			sceAppMgrMountById = (void*)(appmgr_code + 0x198E9);
			info_offset = 0x580;
			break;

		case 0x8C76997D: // 3.51
		case 0x1D32C3FF: // 3.52
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE1);
			sceAppMgrMountById = (void*)(appmgr_code + 0x19D5D);
			info_offset = 0x580;
			break;

		case 0x94CEFE4B: // 3.55
		case 0xDFBC288C: // 3.57
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE1);
			sceAppMgrMountById = (void*)(appmgr_code + 0x19E15);
			info_offset = 0x580;
			break;

		case 0xDBB29DB7: // 3.60
		case 0xB5F8EA7C: // 3.61
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE1);
			sceAppMgrMountById = (void*)(appmgr_code + 0x19B51);
			info_offset = 0x580;
			break;

		case 0x23B967C5: // 3.63
		case 0x1C9879D6: // 3.65
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE1);
			sceAppMgrMountById = (void*)(appmgr_code + 0x19E61);
			info_offset = 0x580;
			break;

		case 0x54E2E984: // 3.67
		case 0xC3C538DE: // 3.68
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE1);
			sceAppMgrMountById = (void*)(appmgr_code + 0x19E6D);
			info_offset = 0x580;
			break;

		case 0x321E4852: // 3.69
		case 0x700DA0CD: // 3.70
		case 0xF7846B4E: // 3.71
		case 0xA8E80BA8: // 3.72
		case 0xB299D195: // 3.73
		case 0x30007BD3: // 3.74
			sceAppMgrFindProcessInfoByPid = (void*)(appmgr_code + 0x2DE9);
			sceAppMgrMountById = (void*)(appmgr_code + 0x19E95);
			info_offset = 0x580;
			break;

		default:
			return -1;
	}

	if (!sceAppMgrFindProcessInfoByPid || !sceAppMgrMountById)
		return -1;

	void* info = sceAppMgrFindProcessInfoByPid((void*)(appmgr_data + 0x500), pid);
	if (!info)
		return -1;

	char mount_point[16];
	return sceAppMgrMountById(pid, info + info_offset, 0x3ED, "PCSG90096", "ux0:user/00/savedata/PCSG90096", "savedata0:", NULL, mount_point);
}

// Install path and arguments
const char launch_path_ur[] = "ur0:/temp/bootstrap.self";
const char launch_path_ux[] = "ux0:/data/bootstrap.self";
const char launch_args[] = "\0\0\0\0-nonsuspendable\0-livearea_off\0";

int thread_main(SceSize args, void* argp)
{
	(void)args; (void)argp;

	void* lr = NULL;
	__asm__ volatile("mov %0, lr" : "=r"(lr));
	LOG("Main kernel thread, called from %x!", lr);
	FLOG("Main kernel thread, called from 0x%08X!", lr);

	char real_args[sizeof(launch_args)];
	memcpy(real_args, launch_args, sizeof(launch_args));
	*(uint16_t*)&real_args[0] = syscall_id;

	const char* launch_path = launch_path_ux;
	int fd  = sceIoOpen(launch_path, 0x603, 0006);
	int ret = fd;
	LOG("ux sceIoOpen: %x", fd);
	FLOG("ux sceIoOpen: 0x%08X", fd);
	if (fd < 0)
	{
		launch_path = launch_path_ur;
		fd  = sceIoOpen(launch_path, 0x603, 0006);
		ret = fd;
		LOG("ur sceIoOpen: %x", fd);
		FLOG("ur sceIoOpen: 0x%08X", fd);
	}

	if (fd >= 0)
	{
		ret = sceIoWrite(fd, bootstrap_self, bootstrap_self_len);
		LOG("sceIoWrite: %x", ret);
		sceIoClose(fd);

		SceAppMgrLaunchParam param;
		memset(&param, 0, sizeof(param));
		param.size = sizeof(SceAppMgrLaunchParam);
		param.attr = 0;
		LOG("Launching bootstrap, me @ 0x%04X...", syscall_id);
		FLOG("Launching bootstrap, me @ 0x%04X...", syscall_id);
		ret = sceAppMgrLaunchAppByPath(launch_path, real_args, sizeof(launch_args), 0, &param, NULL);
		LOG("sceAppMgrLaunchAppByPath: %x", ret);
		FLOG("sceAppMgrLaunchAppByPath: 0x%08X", ret);
	}

	if (ret < 0)
	{
		LOG("unable to write bootstrap!");
		FLOG("unable to write bootstrap!");
		remove_sigpatches();
		remove_pkgpatches();

		__asm__ volatile(
			"mov lr, %0\n\t"
			"mov r0, %1\n\t"
			"bx r0\n\t"
			 :
			 : "r"(lr),
			   "r"(cleanup_memory)
			 : "r0", "lr"
		);

		LOG("should not be here!");
		FLOG("should not be here!");
		while (1) { }
	}

	LOG("done with kernel thread!");
	FLOG("done with kernel thread!");
	return 0;
}

int add_syscalls(void)
{
	sceKernelRegisterSyscall(syscall_id + 0, load_taihen);
	sceKernelRegisterSyscall(syscall_id + 1, remove_pkgpatches);
	sceKernelRegisterSyscall(syscall_id + 2, remove_sigpatches);
	sceKernelRegisterSyscall(syscall_id + 3, cleanup_memory);
	sceKernelRegisterSyscall(syscall_id + 4, mount_savedata);
	return 0;
}

void resolve_imports(SceUIntPtr sysmem_base)
{
	module_info_t* sysmem_info = find_modinfo(sysmem_base, "SceSysmem");
	
	// Used to find SceKernelModulemgr from SceSysmem's .text address
	LOG("sysmem base: 0x%08X", sysmem_base);
	SceUIntPtr sysmem_data = (SceUIntPtr)(*(SceUIntPtr*)(sysmem_base + SYSMEM_TEXT2DATA_OFFSET) - SYSMEM_DATA2BASE_OFFSET);
	LOG("sysmem data base: 0x%08X", sysmem_data);

	SceUIntPtr modulemgr_base = (*(SceUIntPtr*)(sysmem_data + SYSMEM_BACKTRACE_PTR_OFFSET) - MODULEMGR_BACKTRACE_OFFSET);
	LOG("modulemgr base: 0x%08X", modulemgr_base);

	DACR_OFF(modulemgr_info = find_modinfo(modulemgr_base, "SceKernelModulemgr"));
	LOG("modulemgr modinfo: 0x%08X", modulemgr_info);

	DACR_OFF(
		sceKernelGetModuleList = find_export(modulemgr_info, MODULEMGR_GETMODULELIST_NID);
		sceKernelGetModuleInfo = find_export(modulemgr_info, MODULEMGR_GETMODULEINFO_NID);
	);

	LOG("sceKernelGetModuleList: 0x%08X", sceKernelGetModuleList);
	LOG("sceKernelGetModuleInfo: 0x%08X", sceKernelGetModuleInfo);

	SceUID  modlist[128];
	SceSize modlist_records = 128;
	SceKernelModuleInfo info;
	memset(&info, 0, sizeof(info));

	int ret = sceKernelGetModuleList(0x10005, 0x7FFFFFFF, 1, modlist, &modlist_records);
	LOG("sceKernelGetModuleList() returned 0x%x", ret);
	LOG("modlist_records: %d", modlist_records);
	module_info_t *threadmgr_info = NULL, *sblauthmgr_info = NULL, *processmgr_info = NULL, *iofilemgr_info = NULL, *sblacmgr_info = NULL;
	SceUIntPtr modulemgr_data = 0;
	for (SceSize i = 0; i < modlist_records; i++)
	{
		info.size = sizeof(info);
		ret = sceKernelGetModuleInfo(0x10005, modlist[i], &info);
		if (strcmp(info.name, "SceKernelThreadMgr") == 0)
		{
			threadmgr_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceKernelThreadMgr");
		}
		if (strcmp(info.name, "SceSblAuthMgr") == 0)
		{
			sblauthmgr_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceSblAuthMgr");
		}
		if (strcmp(info.name, "SceNpDrm") == 0)
		{
			DACR_OFF(scenpdrm_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceNpDrm"));
		}
		if (strcmp(info.name, "SceKernelModulemgr") == 0)
		{
			modulemgr_data = (SceUIntPtr)info.segments[1].vaddr;
		}
		if (strcmp(info.name, "SceAppMgr") == 0)
		{
			DACR_OFF(appmgr_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceAppMgr"));
			DACR_OFF(appmgr_code = (SceUIntPtr)info.segments[0].vaddr);
			DACR_OFF(appmgr_size = (SceUIntPtr)info.segments[0].memsz);
			DACR_OFF(appmgr_data = (SceUIntPtr)info.segments[1].vaddr);
		}
		if (strcmp(info.name, "SceIofilemgr") == 0)
		{
			iofilemgr_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceIofilemgr");
		}
		if (strcmp(info.name, "SceProcessmgr") == 0)
		{
			processmgr_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceProcessmgr");
		}
#ifdef FIX_NGS
		if (strcmp(info.name, "SceNgs") == 0)
		{
			DACR_OFF(ngs_data = (SceUIntPtr)info.segments[1].vaddr);
		}
#endif
#ifdef FIX_NETPS_UAF
		if (strcmp(info.name, "SceNetPs") == 0)
		{
			DACR_OFF(netps_code = (SceUIntPtr)info.segments[0].vaddr);
			DACR_OFF(netps_data = (SceUIntPtr)info.segments[1].vaddr);
		}
#endif
		if (strcmp(info.name, "SceSblACMgr") == 0)
		{
			sblacmgr_info = find_modinfo((SceUIntPtr)info.segments[0].vaddr, "SceSblACMgr");
		}
#ifdef DONT_LOAD_TAIHEN_TWICE
		if (strcmp(info.name, "taihen") == 0)
		{
			DACR_OFF(has_taihen = SCE_TRUE);
			LOG("taiHEN was already loaded and DONT_LOAD_TAIHEN_TWICE is defined!");
		}
#endif
	}

	LOG("threadmgr_info: 0x%08X | sblauthmgr_info: 0x%08X | scenpdrm_info: 0x%08X | sblacmgr_info: 0x%08X", threadmgr_info, sblauthmgr_info, scenpdrm_info, sblacmgr_info);

	DACR_OFF(
		orig_sceSblAuthMgrAuthHeader_SceKernelModulemgr_imp = find_export(sblauthmgr_info, 0xF3411881);
		orig_sceSblAuthMgrSetupAuthSegment_SceKernelModulemgr_imp = find_export(sblauthmgr_info, 0x89CCDA2C);
		orig_sceSblAuthMgrAuthSegment_SceKernelModulemgr_imp = find_export(sblauthmgr_info, 0xBC422443);

		sceKernelIcacheInvalidateRange = find_export(sysmem_info, SYSMEM_ICACHEINVALIDATERANGE_NID);
		sceKernelDcacheCleanRangeForL1WBWA = find_export(sysmem_info, 0x9CB9F0CE);
		sceKernelFreeMemBlock = find_export(sysmem_info, 0x009E1C61);
		sceKernelFindMemBlockByAddr = find_export(sysmem_info, 0x8A1742F6);
		sceKernelGetMemBlockBase = find_export(sysmem_info, 0xA841EDDA);
		sceKernelSysrootGetShellPid = find_export(sysmem_info, 0x05093E7B);

		sceKernelLoadModule = find_export(modulemgr_info, 0x86D8D634);
		sceKernelStartModule = find_export(modulemgr_info, 0x0675B682);
		sceKernelRegisterSyscall = find_export(modulemgr_info, MODULEMGR_REGISTERSYSCALL_NID);

		sceIoOpen = find_export(iofilemgr_info, 0x75192972);
		sceIoClose = find_export(iofilemgr_info, 0xF99DD8A3);
		sceIoWrite = find_export(iofilemgr_info, 0x21EE91F0);

		sceAppMgrLaunchAppByPath = find_export(appmgr_info, 0xB0A37065);
		
		sceKernelCreateThread = find_export(threadmgr_info, 0xC6674E7D);
		sceKernelStartThread = find_export(threadmgr_info, 0x21F5419B);
		sceKernelExitDeleteThread = find_export(threadmgr_info, 0x1D17DECF);
		sceKernelGetProcessInfo = find_export(processmgr_info, 0x0AFF3EAE);
		sceKernelGetRemoteKPLS = find_export(processmgr_info, 0xAF80F39C);
		sceSblACMgrIsDevelopmentMode = find_export(sblacmgr_info, 0xE87D1777);

#ifdef FILE_LOGGING
		ptr_snprintf = find_export(sysmem_info, 0xAE7A8981);
#endif
	);

	DACR_OFF(
		syscall_lo = (int*)(modulemgr_data + MODULEMGR_SYSCALL_LO_OFFSET);
		syscall_id = (*syscall_lo & 0xFFF) | 1; // id must not be x00
		syscall_stub = (void*)(modulemgr_base + MODULEMGR_SYSCALL_STUB_OFFSET);
	);
	LOG("old syscall_lo: 0x%03X", *syscall_lo);
	*syscall_lo = syscall_id + 5;
	LOG("new syscall_lo: 0x%03X", *syscall_lo);
}

#ifdef FIX_NETPS_UAF
void fix_netps(void)
{
	int (*sce_psnet_bnet_mutex_unlock)(void* mutex) = (void*)(netps_code + NETPS_BNET_MUTEX_UNLOCK_OFFSET);
	void *mutex = (void*)(netps_data + NETPS_BNET_JUMBO_MUTEX_OFFSET);
	sce_psnet_bnet_mutex_unlock(mutex);
}
#endif

#ifdef FIX_NGS
void fix_ngs(void)
{	
	// Get the process id
	SceKernelProcessInfo info;
	memset(&info, 0, sizeof(info));
	info.size = sizeof(SceKernelProcessInfo);
	sceKernelGetProcessInfo(0, &info);

	void* out_addr = NULL;
	SceUInt32 key = *(SceUInt32*)(ngs_data + 0xC60 + 0x10);
	sceKernelGetRemoteKPLS(info.pid, key, &out_addr, SCE_TRUE);

	if (out_addr)
	{
		uint32_t block_info = *(uint32_t*)(out_addr + 0x320);
		if (block_info)
		{
			uint32_t  num_blocks = *(uint32_t*)(block_info + 0x08);
			uint32_t* blocks = *(uint32_t**)(block_info + 0x24);

			for (uint32_t i = 0; i < num_blocks; i++)
				blocks[i] = 0;

			*(uint32_t*)(block_info + 0x08) = 0;
		}
	}
}
#endif

#ifdef MODIFY_FRAMEBUFFER
// Color is in the following format: AABBGGRR
void flash_screen(SceUIntPtr framebuf, SceUInt32 color)
{
	uint32_t prev_dacr;
	__asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(prev_dacr));
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(0x55555555));

	uint32_t base, pitch, height;
	__asm__ volatile("ldrt %0, [%1]" : "=r" (base) : "r"(framebuf + 0x04));
	__asm__ volatile("ldrt %0, [%1]" : "=r" (pitch) : "r"(framebuf + 0x08));
	__asm__ volatile("ldrt %0, [%1]" : "=r" (height) : "r"(framebuf + 0x14));

	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < pitch; x++)
		{
			__asm__ volatile("strt %0, [%1]" : : "r"(color), "r"(base += 4));
		}
	}

	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(prev_dacr));	
}
#endif

#define XSTRINGIFY(x) #x
#define STRINGIFY(x) XSTRINGIFY(x)
// Put this in the binary even if it goes unused.
// Note that it will print '363', '374', not '3.63', or '3.74'.
// Trying to change it is too much work, so I won't bother.
__attribute__((used)) const char s_build_str[] = "h-encore payload built on " __DATE__ " at " __TIME__ ", targeting FW '" STRINGIFY(FW) "'";
#undef XSTRINGIFY
#undef STRINGIFY

void start() __attribute__((weak, alias("payload")));
void _start() __attribute__((weak, alias("payload")));
void __attribute__((section(".text.start"))) payload(void* rx_block, SceUIntPtr sysmem_addr, SceUIntPtr framebuf)
{
	int ret = -1;

#ifdef DEBUG
	int (*sceKernelPrintf_local)(const char* fmt, ...) = (void*)(sysmem_addr + SYSMEM_PRINTF_OFFSET);
	DACR_OFF(sceKernelPrintf = sceKernelPrintf_local);
#endif

#ifdef RUNNING_IN_SYSCALL_CONTEXT
	int state;
	ENTER_SYSCALL(state);
#endif

#ifdef MODIFY_FRAMEBUFFER
	// flash purple
	flash_screen(framebuf, 0xFFFF00FF);
#endif

	LOG("+++ Entered kernel payload +++");
	LOG("payload=0x%08X, sp=0x%08X", payload, &ret);
	LOG("rx_block=0x%08X, sysmem_addr=0x%08X, framebuf=0x%08X", rx_block, sysmem_addr, framebuf);
	LOG("%s\n", s_build_str);

	LOG("resolving imports");
	resolve_imports(sysmem_addr);

#ifdef FIX_NGS
	LOG("fixing ngs");
	fix_ngs();
#endif
	
#ifdef FIX_NETPS_UAF
	LOG("fixing netps use-after-freee");
	fix_netps();
#endif

	LOG("set up syscalls starting at id: %x", syscall_id);
	add_syscalls();

	LOG("adding temporary patches");
	find_ux0_data_path_addr();
	temp_pkgpatches();
	temp_sigpatches();

	LOG("find rx block");
	DACR_OFF(g_rx_block = sceKernelFindMemBlockByAddr(rx_block, 0));

	LOG("flush changes");
	void* base = NULL;
	ret = sceKernelGetMemBlockBase(g_rx_block, &base);
	LOG("sceKernelGetMemBlockBase: %x, %x", ret, base);
	sceKernelDcacheCleanRangeForL1WBWA(base, 0x4000);

	LOG("starting kernel thread");
	SceUID tid = sceKernelCreateThread("stage2", thread_main, 64, 0x4000, 0, 0, NULL);
	LOG("create tid: %x", tid);
	ret = sceKernelStartThread(tid, 0, NULL);
	LOG("start thread: %x", ret);

#ifdef RUNNING_IN_SYSCALL_CONTEXT
	EXIT_SYSCALL(state);
#endif	

	LOG("killing self");
	sceKernelExitDeleteThread(0);

	LOG("should not be here!");
	while (1) { }
}
