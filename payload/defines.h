typedef int SceUID;

typedef int SceBool;
#define SCE_FALSE ((SceBool)0)
#define SCE_TRUE ((SceBool)1)

typedef signed int SceInt32;
typedef unsigned int SceUInt32;

typedef unsigned int SceUIntPtr;

typedef signed int SceSSize;
typedef unsigned int SceSize;

// SceKernelModulemgr
typedef struct SceKernelLoadModuleOpt
{
	SceSize size; // Size of this structure
} SceKernelLoadModuleOpt;

typedef struct SceKernelStartModuleOpt
{
	SceSize   size;     // Size of this structure
	SceUInt32 flags;    // should be 0
	SceUInt32 prologue; // should be 0
	SceUInt32 start;    // should be 0
} SceKernelStartModuleOpt;

// SceAppMgr
typedef struct SceAppMgrLaunchParam // size is 0x34
{
	SceSize   size;         // Size of this structure
	SceUInt32 attr;         // set to 0x80000000 to break on launch
	SceUInt32 initPriority;
	SceUInt32 unk_C;
	SceUInt32 unk_10;
	SceUInt32 unk_14;
	SceUInt32 budgetId;
	SceUInt32 unk_1C;
	SceUInt32 unk_20;
	SceUInt32 unk_24;
	SceUInt32 unk_28;
	SceUInt32 unk_2C;
	SceUInt32 unk_30;
} SceAppMgrLaunchParam;

// SceKernelThreadmgr
typedef SceInt32 (*SceKernelThreadEntry)(SceSize argSize, void* pArgBlock);
typedef struct SceKernelThreadOptParam // Min size is 4 bytes. Max size is 0x14 bytes.
{
	SceSize   size;          // Size of this structure
	SceUInt32 attr;          // A bit pattern that specifies valid members in a structure
	SceUInt32 kStackMemType;
	SceUInt32 uStackMemType;
	SceUInt32 uTLSMemType;
	SceUInt32 uStackMemid;
	SceUInt32 data_0x18;
} SceKernelThreadOptParam;

// SceProcessmgr
typedef struct SceKernelProcessInfo // size is 0x24 or 0x4C on FW 0.931, 0xE8 on FW 3.60
{
	SceSize   size;           // Size of this structure
	SceUID    pid;            // Process ID
	SceUID    addressSpaceId;
	SceUInt32 unk_C;          // processCreationFlags
	SceUInt32 status;         // Process status. ex: suspended = 0x11/0x10011, started = |2.
	SceUID    ppid;           // Parent process ID
	SceUID    budgetId;
	SceUInt32 cpuAffinity;    // CPU affinity. ex: 7
	SceUInt32 unk_20;         // might be initial priority. ex: 0x7F (127)
	SceUID    modid;          // Module ID
	void*     entrypoint;     // Process entry point
	SceUInt32 process_type;
	char      name[32];       // Process name. ProcessObject->loadprocessparam.module_name if non-empty, else copy from sceGUIDGetName2ForDriver if not NULL, else \0
	void*     unk_50;         // ?maxFHCount?
	void*     unk_54;         // ?EntryHeap_uid?
	SceUID    entryHeapId;    // GUID/PUID EntryHeap GUID
	SceUInt32 unk_5C;         // ex: -1
	SceUInt32 unk_60;         // ?module_inhibit_state?
	SceUInt32 unk_64;         // ex: 0x32
	void*     KPLS_slots[32];
} SceKernelProcessInfo;