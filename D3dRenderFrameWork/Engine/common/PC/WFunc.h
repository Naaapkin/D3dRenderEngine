#pragma once
#ifdef WIN32
#include <Engine/pch.h>
namespace WFunc
{
    void QueryPrimaryScreenRC(RECT* rect);
    LPWSTR GetHRInfo(DWORD hr);
    std::wstring GetWWindowTitle(HWND windowHandle);
    std::wstring GetKeyName(UINT key);
    bool SafeShutdown(HWND hWnd);
}
#endif