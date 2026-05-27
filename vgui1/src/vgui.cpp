#include <VGUI.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

namespace vgui
{

static void* (*_malloc_func)(size_t size) = malloc;
static void  (*_free_func)(void* memblock) = free;

void vgui_setMalloc(void* (*mallocFunc)(size_t size))
{
	_malloc_func = mallocFunc;
}

void vgui_setFree(void (*freeFunc)(void* memblock))
{
	_free_func = freeFunc;
}

void vgui_strcpy(char* dst, int dstLen, const char* src)
{
	if (!dst || dstLen <= 0)
		return;
	if (!src)
	{
		dst[0] = 0;
		return;
	}
	int i;
	for (i = 0; i < dstLen - 1 && src[i]; i++)
		dst[i] = src[i];
	dst[i] = 0;
}

char* vgui_strdup(const char* src)
{
	if (!src)
		return null;
	int len = strlen(src) + 1;
	char* dst = (char*)_malloc_func(len);
	if (dst)
		memcpy(dst, src, len);
	return dst;
}

int vgui_printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vprintf(format, args);
	va_end(args);
	return ret;
}

int vgui_dprintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vprintf(format, args);
	va_end(args);
	return ret;
}

int vgui_dprintf2(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = vprintf(format, args);
	va_end(args);
	return ret;
}

}
