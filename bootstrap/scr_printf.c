/*
 * PSP Software Development Kit - http://www.psvdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * scr_printf.c - Debug screen functions.
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 *
 * $Id: scr_printf.c 2450 2009-01-04 23:53:02Z oopo $
 */
#include <vitasdk.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pspdebug.h"

#define PSV_SCREEN_WIDTH 960
#define PSV_SCREEN_HEIGHT 544
#define PSV_LINE_SIZE 960

static void _psvDebugScreenClearLine(int y);

static int X = 0, Y = 0;
static int MX = 68, MY = 34;
static uint32_t bg_col = 0, fg_col = 0xFFFFFFFF;
static bool bg_enable = true;
static void* g_vram_base = NULL;
static int g_vram_offset = 0;
static int g_vram_mode = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
static bool clearline_en = true;
static bool init = false;

static void clear_screen_32(uint32_t color)
{
	uint32_t* vram = g_vram_base;
	vram += g_vram_offset / 4;

	for (size_t x = 0; x < PSV_LINE_SIZE * PSV_SCREEN_HEIGHT; x++)
		*vram++ = color; 
}

static void clear_screen(uint32_t color)
{
	if (g_vram_mode == SCE_DISPLAY_PIXELFORMAT_A8B8G8R8)
		clear_screen_32(color);
}

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

void psvDebugScreenInitEx(void* vram_base, int mode, int setup)
{
	switch (mode)
	{
	case SCE_DISPLAY_PIXELFORMAT_A8B8G8R8:
		break;
	default:
		mode = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
		break;
	};

	X = 0;
	Y = 0;

	// Place vram in uncached memory
	if (vram_base == NULL)
	{
		int block = sceKernelAllocMemBlock("PsvDebugScreenVram", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(PSV_LINE_SIZE * PSV_SCREEN_HEIGHT * 4, 256 * 1024), NULL);
		sceKernelGetMemBlockBase(block, &vram_base);
	}

	g_vram_base   = vram_base;
	g_vram_offset = 0;
	g_vram_mode   = mode;
	if (setup)
	{
		SceDisplayFrameBuf framebuf;
		framebuf.size        = sizeof(SceDisplayFrameBuf);
		framebuf.base        = g_vram_base;
		framebuf.pitch       = PSV_LINE_SIZE;
		framebuf.pixelformat = mode;
		framebuf.width       = PSV_SCREEN_WIDTH;
		framebuf.height      = PSV_SCREEN_HEIGHT;
		sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);
	}

	clear_screen(bg_col);
	init = true;
}

void psvDebugScreenInit(void)
{
	X = 0;
	Y = 0;
	psvDebugScreenInitEx(NULL, SCE_DISPLAY_PIXELFORMAT_A8B8G8R8, 1);
}

void psvDebugScreenEnableBackColor(bool enable) 
{
	bg_enable = enable;
}

void psvDebugScreenSetBackColor(uint32_t colour)
{
	bg_col = colour;
}

void psvDebugScreenSetTextColor(uint32_t colour)
{
	fg_col = colour;
}

void psvDebugScreenSetColorMode(int mode)
{
	switch (mode)
	{
	case SCE_DISPLAY_PIXELFORMAT_A8B8G8R8:
		break;
	default:
		mode = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
		break;
	};

	g_vram_mode = mode;
}

int psvDebugScreenGetX(void)
{
	return X;
}

int psvDebugScreenGetY(void)
{
	return Y;
}

void psvDebugScreenClear(void)
{
	if (!init)
		return;

	for (int y = 0; y < MY; y++)
		_psvDebugScreenClearLine(y);

	psvDebugScreenSetXY(0, 0);
	clear_screen(bg_col);
}

void psvDebugScreenSetXY(int x, int y)
{
	if (x < MX && x >= 0)
		X = x;

	if (y < MY && y >= 0)
		Y = y;
}

void psvDebugScreenSetOffset(int offset)
{
	g_vram_offset = offset;
}

void psvDebugScreenSetBase(void* base)
{
	g_vram_base = base;
}

extern const unsigned char msx[];

static void debug_put_char_32(int x, int y, uint32_t color, uint32_t bgc, uint8_t ch)
{
	if (!init)
		return;

	x *= 2;
	y *= 2;

	uint32_t* vram = g_vram_base;
	vram += (g_vram_offset / 4) + x;
	vram += y * PSV_LINE_SIZE;

	const uint8_t* font = &msx[(int)ch * 8];
	for (int i = 0, l = 0; i < 8; i++, l += 8, font++)
	{
		uint32_t* vram_ptr = vram;
		for (int j = 0; j < 8; j++)
		{
			if ((*font & (128 >> j)))
			{
				vram_ptr[0] = color; 
				vram_ptr[1] = color; 
				vram_ptr[0 + PSV_LINE_SIZE] = color; 
				vram_ptr[1 + PSV_LINE_SIZE] = color; 
			}
			else if (bg_enable)
			{
				vram_ptr[0] = bgc; 
				vram_ptr[1] = bgc; 
				vram_ptr[0 + PSV_LINE_SIZE] = bgc; 
				vram_ptr[1 + PSV_LINE_SIZE] = bgc; 
			}

			vram_ptr += 2;
		}
		vram += 2 * PSV_LINE_SIZE;
	}
}

void psvDebugScreenPutChar(int x, int y, uint32_t color, uint8_t ch)
{
	if (g_vram_mode == SCE_DISPLAY_PIXELFORMAT_A8B8G8R8)
		debug_put_char_32(x, y, color, bg_col, ch);
}

static void _psvDebugScreenClearLine(int y)
{
	if (!clearline_en && !bg_enable)
		return;

	for (int i = 0; i < MX; i++)
		psvDebugScreenPutChar(i * 7, y * 8, bg_col, 219);
		
}

void psvDebugScreenClearLineEnable(void)
{
	clearline_en = true;
}

void psvDebugScreenClearLineDisable(void)
{
	clearline_en = false;
}

// Print non-nul terminated strings
size_t psvDebugScreenPrintData(const char* buff, size_t size)
{
	if (!init)
		return 0;

	size_t i = 0;
	for (i = 0; i < size; i++)
	{
		char c = buff[i];
		switch (c)
		{
		case '\r':
			X = 0;
			break;
		case '\n':
			X = 0;
			Y++;
			if (Y == MY)
				Y = 0;
			_psvDebugScreenClearLine(Y);
			break;
		case '\t':
			for (int j = 0; j < 5; j++)
			{
				psvDebugScreenPutChar(X * 7, Y * 8, fg_col, ' ');
				X++;
			}
			break;
		default:
			psvDebugScreenPutChar(X * 7, Y * 8, fg_col, c);
			X++;
			if (X == MX)
			{
				X = 0;
				Y++;
				if (Y == MY)
					Y = 0;
				_psvDebugScreenClearLine(Y);
			}
		}
	}

	return i;
}

int psvDebugScreenPuts(const char* str)
{
	return psvDebugScreenPrintData(str, sceClibStrnlen(str, 2048));
}

void psvDebugScreenPrintf(const char* format, ...)
{
	va_list opt;
	char buff[2048];
	int  bufsz;

	va_start(opt, format);
	bufsz = sceClibVsnprintf(buff, sizeof(buff), format, opt);
	(void)psvDebugScreenPrintData(buff, bufsz);
}
