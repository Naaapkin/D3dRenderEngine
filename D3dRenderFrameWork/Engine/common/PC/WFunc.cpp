#ifdef WIN32
#include "Engine/common/PC/WFunc.h"

void WFunc::QueryPrimaryScreenRC(RECT* rect)
{
	// 获取主显示器
	auto hmon = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO mi = { sizeof(mi) };
	if (!GetMonitorInfo(hmon, &mi)) return;
	*rect = {mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom};
}

LPWSTR WFunc::GetHRInfo(DWORD hr)
{
	LPWSTR msg = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	               nullptr, hr, 0, reinterpret_cast<LPWSTR>(&msg), 0, nullptr);
	return msg;
}

std::wstring WFunc::GetWWindowTitle(HWND windowHandle)
{
	wchar_t buffer[64];
	uint16_t length = GetWindowTextLength(windowHandle);
	GetWindowTextW(windowHandle, buffer, length);
	return { 0, length };
}

std::wstring WFunc::GetKeyName(UINT key)
{
	wchar_t keyName[256];
	UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

	switch (key) {
	case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
	case VK_PRIOR: case VK_NEXT:
	case VK_END: case VK_HOME:
	case VK_INSERT: case VK_DELETE:
	case VK_DIVIDE:
		scanCode |= 0x100;
		break;
	default:
		break;
	}

	uint16_t length = static_cast<uint16_t>(GetKeyNameTextW(scanCode << 16, keyName, 256));
	return { keyName, length };
}

bool WFunc::SafeShutdown(HWND hWnd)
{
	if (!SendMessageTimeout(hWnd, WM_CLOSE, 0, 0,
	                        SMTO_NORMAL, 5000, nullptr)) 
	{
		DestroyWindow(hWnd);
	}
	PostQuitMessage(0);
	MSG msg;
	DWORD startTime = GetTickCount();
	while (GetTickCount() - startTime < 5000) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return true;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Sleep(1);
		}
	}
	return false;
}
#endif