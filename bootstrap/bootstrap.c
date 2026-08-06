/* bootstrap.c -- h-encore bootstrap menu
 *
 * Copyright (C) 2018 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <vitasdk.h>
#include <stdbool.h>
#include "pspdebug.h"

#define WHITE 0xFFFFFFFF
#define AZURE 0xFFFF7F00

#define printf psvDebugScreenPrintf

// GitHub enforces TLS 1.2, unfortunately.
#define VITASHELL_BASE_ADDRESS "https://raw.githubusercontent.com/RealYoti/VitaShell/master/release"

#define INCLUDE_EXTERN_RESOURCE(name) extern unsigned char _binary_res_##name##_start; extern unsigned char _binary_res_##name##_size; \

INCLUDE_EXTERN_RESOURCE(taihen_skprx);
INCLUDE_EXTERN_RESOURCE(henkaku_skprx);
INCLUDE_EXTERN_RESOURCE(henkaku_suprx);

static const char taihen_config_recovery_header[] =
	"# This file is used as an alternative if ux0:tai/config.txt is not found.\n";

static const char taihen_config_header[] =
	"# For users plugins, you must refresh taiHEN from HENkaku Settings for\n"
	"# changes to take place.\n"
	"# For kernel plugins, you must reboot for changes to take place.\n";

static const char taihen_config[] =
	"*KERNEL\n"
	"# henkaku.skprx is hard-coded to load and is not listed here\n"
	"*main\n"
	"# main is a special titleid for SceShell\n"
	"ur0:tai/henkaku.suprx\n"
	"*NPXS10015\n"
	"# this is for modifying the version string\n"
	"ur0:tai/henkaku.suprx\n"
	"*NPXS10016\n"
	"# this is for modifying the version string in settings widget\n"
	"ur0:tai/henkaku.suprx\n";

enum Items
{
	EXIT,
	INSTALL_HENKAKU,
	DOWNLOAD_VITASHELL,
	PERSONALIZE_SAVEDATA,
	RESET_TAIHEN_CONFIG,
};

static const char* items[] =
{
	"Exit",
	"Install HENkaku",
	"Download VitaShell",
	"Personalize savedata",
	"Reset taiHEN config.txt",
};

#define N_ITEMS (sizeof(items) / sizeof(char*))

static int __attribute__((naked, noinline)) call_syscall(int a1, int a2, int a3, int num)
{
	__asm__ volatile("mov r12, %0\nsvc 0\nbx lr\n" : : "r"(num));
}

static int write_file(const char* file, const void* buf, size_t size)
{
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	int written = sceIoWrite(fd, buf, size);
	sceIoClose(fd);
	return written;
}

bool exists(const char* path)
{
	SceIoStat stat;
	if (sceIoGetstat(path, &stat) < 0)
		return false;
	else
		return true;
}

int load_sce_paf(void)
{
	static uint32_t argp[] = { 0x400000, 0xEA60, 0x40000, 0, 0 };

	int result = -1;

	SceSysmoduleOpt option;
	option.flags     = 0x10;
	option.result    = &result;
	option.unused[0] = -1;
	option.unused[1] = -1;
	return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, &option);
}

int unload_sce_paf(void)
{
	int result = -1;

	SceSysmoduleOpt option;
	option.flags     = 0x10;
	option.result    = &result;
	option.unused[0] = -1;
	option.unused[1] = -1;
	return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &option);
}

int promote_app(const char* path)
{
	int res;

	load_sce_paf();

	res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
	if (res < 0)
		return res;

	res = scePromoterUtilityInit();
	if (res < 0)
		return res;

	res = scePromoterUtilityPromotePkgWithRif(path, 1);
	if (res < 0)
		return res;

	res = scePromoterUtilityExit();
	if (res < 0)
		return res;

	res = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
	if (res < 0)
		return res;

	unload_sce_paf();

	return res;
}

int remove(const char* path)
{
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0)
	{
		int res = 0;
		do {
			SceIoDirent dir;
			sceClibMemset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0)
			{
				char new_path[256];
				sceClibSnprintf(new_path, sizeof(new_path), "%s/%s", path, dir.d_name);
				res = remove(new_path);
			}
		} while (res > 0);
		sceIoDclose(dfd);
		return sceIoRmdir(path);
	}
	return sceIoRemove(path);
}

int download(const char* src, const char* dst)
{
	int ret;
	int statusCode;
	int tmplId = -1, connId = -1, reqId = -1;
	SceUID fd = -1;

	ret = sceHttpCreateTemplate("h-encore/1.00 libhttp/1.1", SCE_HTTP_VERSION_1_1, SCE_TRUE);
	if (ret < 0)
		goto ERROR_EXIT;

	tmplId = ret;

	ret = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
	if (ret < 0)
		goto ERROR_EXIT;

	connId = ret;

	ret = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
	if (ret < 0)
		goto ERROR_EXIT;

	reqId = ret;

	ret = sceHttpSendRequest(reqId, NULL, 0);
	if (ret < 0)
		goto ERROR_EXIT;

	ret = sceHttpGetStatusCode(reqId, &statusCode);
	if (ret < 0)
		goto ERROR_EXIT;

	if (statusCode == 200)
	{
		uint8_t buf[4096];
		uint64_t size = 0;
		uint32_t value = 0;

		ret = sceHttpGetResponseContentLength(reqId, &size);
		if (ret < 0)
			goto ERROR_EXIT;

		ret = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
		if (ret < 0)
			goto ERROR_EXIT;

		fd = ret;

		int x = psvDebugScreenGetX();
		int y = psvDebugScreenGetY();

		while (true)
		{
			int read = sceHttpReadData(reqId, buf, sizeof(buf));

			if (read < 0)
			{
				ret = read;
				break;
			}

			if (read == 0)
				break;

			int written = sceIoWrite(fd, buf, read);

			if (written < 0)
			{
				ret = written;
				break;
			}

			value += read;

			printf("%d%%", (int)((value * 100) / (uint32_t)size));
			psvDebugScreenSetXY(x, y);
		}
	}

ERROR_EXIT:
	if (fd >= 0)
		sceIoClose(fd);

	if (reqId >= 0)
		sceHttpDeleteRequest(reqId);

	if (connId >= 0)
		sceHttpDeleteConnection(connId);

	if (tmplId >= 0)
		sceHttpDeleteTemplate(tmplId);

	return ret;
}

void init_net(void)
{
	static char memory[16 * 1024];

	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);

	SceNetInitParam param;
	param.memory = memory;
	param.size = sizeof(memory);
	param.flags = 0;

	sceNetInit(&param);
	sceNetCtlInit();

	sceSslInit(300 * 1024);
	sceHttpInit(40 * 1024);

	sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);
}

void finish_net(void)
{
	sceSslTerm();
	sceHttpTerm();
	sceNetCtlTerm();
	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

typedef struct
{
	const char* src;
	const char* dst;
} DownloadSrcDst;

static const DownloadSrcDst vitashell_src_dst[] =
{
	{ "eboot.bin",    "ux0:temp/app/eboot.bin" },
	{ "icon0.png",    "ux0:temp/app/sce_sys/icon0.png" },
	{ "param.sfo",    "ux0:temp/app/sce_sys/param.sfo" },
	{ "bg.png",       "ux0:temp/app/sce_sys/livearea/contents/bg.png" },
	{ "startup.png",  "ux0:temp/app/sce_sys/livearea/contents/startup.png" },
	{ "template.xml", "ux0:temp/app/sce_sys/livearea/contents/template.xml" },
	{ "head.bin",     "ux0:temp/app/sce_sys/package/head.bin" },
};

int download_vitashell(void)
{
	char url[256];
	int res;

	init_net();

	remove("ux0:patch/VITASHELL");

	sceIoMkdir("ux0:VitaShell", 0777);
	sceIoMkdir("ux0:temp", 6);
	sceIoMkdir("ux0:temp/app", 6);
	sceIoMkdir("ux0:temp/app/sce_sys", 6);
	sceIoMkdir("ux0:temp/app/sce_sys/livearea", 6);
	sceIoMkdir("ux0:temp/app/sce_sys/livearea/contents", 6);
	sceIoMkdir("ux0:temp/app/sce_sys/package", 6);

	for (int i = 0; i < sizeof(vitashell_src_dst) / sizeof(DownloadSrcDst); i++)
	{
		printf(" > Downloading %s...", vitashell_src_dst[i].src);
		sceClibSnprintf(url, sizeof(url), "%s/%s", VITASHELL_BASE_ADDRESS, vitashell_src_dst[i].src);
		res = download(url, vitashell_src_dst[i].dst);
		printf("\n");
		if (res < 0)
			return res;
	}

	printf(" > Installing application...\n");
	res = promote_app("ux0:temp/app");
	if (res < 0)
		return res;

	finish_net();

	return 0;
}

int install_henkaku(void)
{
	int res;

	sceIoMkdir("ur0:tai", 0006);

	res = write_file("ur0:tai/taihen.skprx", (void*)&_binary_res_taihen_skprx_start, (size_t)&_binary_res_taihen_skprx_size);
	if (res < 0)
		return res;
	res = write_file("ur0:tai/henkaku.skprx", (void*)&_binary_res_henkaku_skprx_start, (size_t)&_binary_res_henkaku_skprx_size);
	if (res < 0)
		return res;
	res = write_file("ur0:tai/henkaku.suprx", (void*)&_binary_res_henkaku_suprx_start, (size_t)&_binary_res_henkaku_suprx_size);
	if (res < 0)
		return res;

	return 0;
}

int write_taihen_config(const char* path, bool recovery)
{
	int fd;

	// write default config
	sceIoRemove(path);
	fd = sceIoOpen(path, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
	if (recovery)
		sceIoWrite(fd, taihen_config_recovery_header, sizeof(taihen_config_recovery_header) - 1);
	sceIoWrite(fd, taihen_config_header, sizeof(taihen_config_header) - 1);
	sceIoWrite(fd, taihen_config, sizeof(taihen_config) - 1);
	sceIoClose(fd);
	return 0;
}

int reset_taihen_config(void)
{
	sceIoMkdir("ux0:tai", 0006);
	sceIoMkdir("ur0:tai", 0006);

	write_taihen_config("ux0:tai/config.txt", 0);
	write_taihen_config("ur0:tai/config.txt", 1);
	return 0;
}

int personalize_savedata(int syscall_id)
{
	int res;
	int fd;
	uint64_t aid;

	res = call_syscall(sceKernelGetProcessId(), 0, 0, syscall_id + 4);
	if (res < 0 && res != 0x80800003)
		return res;

	res = sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &aid, sizeof(uint64_t));
	if (res < 0)
		return res;

	fd = sceIoOpen("savedata0:sce_sys/param.sfo", SCE_O_RDWR, 0777);
	if (fd < 0)
		return fd;

	sceIoLseek(fd, 0xE4, SCE_SEEK_SET);
	sceIoWrite(fd, &aid, sizeof(uint64_t));
	sceIoClose(fd);

	return 0;
}

int enter_cross = 0;
uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0;

void read_pad(void)
{
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(0, &pad, 1);

	old_buttons = current_buttons;
	current_buttons = pad.buttons;
	pressed_buttons = current_buttons & ~old_buttons;
}

bool wait_confirm(const char* msg)
{
	printf(msg);
	printf(" > Press %c to confirm or %c to decline.\n", enter_cross ? 'X' : 'O', enter_cross ? 'O' : 'X');

	while (true)
	{
		read_pad();

		if ((enter_cross && pressed_buttons & SCE_CTRL_CROSS) || (!enter_cross && pressed_buttons & SCE_CTRL_CIRCLE))
			return true;

		if ((enter_cross && pressed_buttons & SCE_CTRL_CIRCLE) || (!enter_cross && pressed_buttons & SCE_CTRL_CROSS))
			return false;

		sceKernelDelayThread(10 * 1000);
	}

	return false;
}

void print_result(int res)
{
	if (res < 0)
		printf(" > Failed! 0x%08X\n", res);
	else
		printf(" > Success!\n");

	sceKernelDelayThread((res < 0) ? (5 * 1000 * 1000) : (1 * 1000 * 1000));
}

uint32_t cur_fw_version = 0xFFFFFFFF;
int print_menu(int sel)
{
	psvDebugScreenSetXY(0, 0);
	psvDebugScreenSetTextColor(AZURE);
	printf("\n h-encore bootstrap menu\n");
	printf(" Built on " __DATE__ " at " __TIME__ "!\n");
	if (cur_fw_version != 0xFFFFFFFF)
	{
		if ((cur_fw_version & 0x0000F000) != 0) // Unlikely, only 1.691 and 1.692.
			printf(" Firmware version: %X.%03X\n\n", (cur_fw_version >> 24) & 0xFF, (cur_fw_version >> 12) & 0xFFF);
		else
			printf(" Firmware version: %X.%02X\n\n", (cur_fw_version >> 24) & 0xFF, (cur_fw_version >> 16) & 0xFF);
	}

	for (int i = 0; i < N_ITEMS; i++)
	{
		psvDebugScreenSetTextColor(sel == i ? AZURE : WHITE);
		printf(" [%c] %s\n", sel == i ? '*' : ' ', items[i]);
	}

	printf("\n");

	psvDebugScreenSetTextColor(AZURE);
	printf("----------------------------\n\n");
	return 0;
}

int _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize args, void* argp)
{
	int res = -1;
	int syscall_id = *(uint16_t*)argp;
	sceClibPrintf("syscall_id: 0x%04X\n", syscall_id);

	SceKernelSystemSwVersion ver;
	sceClibMemset(&ver, 0, sizeof(ver));
	ver.size = sizeof(ver);
	if (sceKernelGetSystemSwVersion(&ver) >= 0)
		cur_fw_version = ver.version;

	sceAppMgrDestroyOtherApp();

	sceShellUtilInitEvents(0);
	sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	sceClibMemset(&init_param, 0, sizeof(SceAppUtilInitParam));
	sceClibMemset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

	int enter_button = SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
	enter_cross = enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;

	psvDebugScreenInit();
	psvDebugScreenClearLineDisable();

	int sel = 0;
	print_menu(sel);

	while (true)
	{
		read_pad();

		if (pressed_buttons & SCE_CTRL_UP)
		{
			if (sel > 0)
				sel--;
			else
				sel = N_ITEMS - 1;

			print_menu(sel);
		}

		if (pressed_buttons & SCE_CTRL_DOWN)
		{
			if (sel < N_ITEMS - 1)
				sel++;
			else
				sel = 0;

			print_menu(sel);
		}

		// Quit and clean up nicely, without starting HENkaku.
		// This is intended as a proof-of-concept, since porting taiHEN/HENkaku is annoying.
		if (pressed_buttons & SCE_CTRL_SELECT)
		{
			sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

			// Remove pkg patches
			call_syscall(0, 0, 0, syscall_id + 1);

			// Remove sig patches
			call_syscall(0, 0, 0, syscall_id + 2);

			// Clean up
			call_syscall(0, 0, 0, syscall_id + 3);

			sceKernelExitProcess(0);
			return 0;
		}

		if ((enter_cross && pressed_buttons & SCE_CTRL_CROSS) || (!enter_cross && pressed_buttons & SCE_CTRL_CIRCLE))
		{
			psvDebugScreenSetTextColor(AZURE);

			if (sel == EXIT)
			{
				printf(" > Exiting...\n");
				sceKernelDelayThread(500 * 1000);
				break;
			}
			else if (sel == INSTALL_HENKAKU)
			{
				printf(" > Installing HENkaku...\n");
				sceKernelDelayThread(500 * 1000);
				res = install_henkaku();
			}
			else if (sel == DOWNLOAD_VITASHELL)
			{
				printf(" > Downloading VitaShell...\n");
				sceKernelDelayThread(500 * 1000);
				res = download_vitashell();
			}
			else if (sel == PERSONALIZE_SAVEDATA)
			{
				printf(" > Personalizing savedata...\n");
				sceKernelDelayThread(500 * 1000);
				res = personalize_savedata(syscall_id);
			}
			else if (sel == RESET_TAIHEN_CONFIG)
			{
				if (wait_confirm(" > Are you sure you want to reset taiHEN config.txt?\n"))
				{
					printf(" > Resetting taiHEN config.txt...\n");
					sceKernelDelayThread(500 * 1000);
					res = reset_taihen_config();
				}
				else
				{
					sel = 0;
					psvDebugScreenClear();
					print_menu(sel);
					continue;
				}
			}

			print_result(res);

			sel = 0;
			psvDebugScreenClear();
			print_menu(sel);
		}

		sceKernelDelayThread(10 * 1000);
	}

	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

	// Install HENkaku if any of the modules are missing
	if (!exists("ur0:tai/henkaku.suprx") || !exists("ur0:tai/henkaku.skprx") || !exists("ur0:tai/taihen.skprx"))
	{
		printf(" > Installing HENkaku...\n");
		sceKernelDelayThread(500 * 1000);
		res = install_henkaku();
		print_result(res);
	}

	// Write taiHEN configs if both at ur0: and ux0: don't exist
	if (!exists("ur0:tai/config.txt") && !exists("ux0:tai/config.txt"))
	{
		printf(" > Writing taiHEN config.txt...\n");
		sceKernelDelayThread(500 * 1000);
		res = reset_taihen_config();
		print_result(res);
	}

	// Remove pkg patches
	res = call_syscall(0, 0, 0, syscall_id + 1);

	if (res >= 0)
	{
		// Start HENkaku
		res = call_syscall(0, 0, 0, syscall_id + 0);
	}
	else
	{
		// Remove sig patches
		call_syscall(0, 0, 0, syscall_id + 2);
	}

	// Clean up
	call_syscall(0, 0, 0, syscall_id + 3);

	if (res < 0 && res != 0x8002D013 && res != 0x8002D017)
	{
		printf(" > Failed to load HENkaku! 0x%08X\n", res);
		printf(" > Please relaunch the exploit and select 'Install HENkaku'.\n");
		sceKernelDelayThread(5 * 1000 * 1000);
	}

	sceKernelExitProcess(0);
	return 0;
}
