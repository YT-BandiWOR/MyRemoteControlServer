#pragma once
#include <Windows.h>

Size2i GetScreenSize() {
	Size2i size = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	return size;
}