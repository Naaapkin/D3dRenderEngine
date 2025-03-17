#pragma once
#ifndef UNICODE
#define UNICODE
#endif

#include <fstream>
#include <stack>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <string>
#include <format>
#include <vector>
#include <exception>

#ifdef UNICODE
#define TO_STRING(str) std::to_wstring(str)
using String = std::wstring;
#else
#define TO_STRING(str) std::to_string(str)
using String = std::string;
#endif

#define DELETE_MOVE_CONSTRUCTOR(name) name(name&& other) noexcept = delete;
#define DELETE_COPY_CONSTRUCTOR(name) name(const name& other) = delete;
#define DELETE_MOVE_OPERATOR(name) name& operator=(name&& other) noexcept = delete;
#define DELETE_COPY_OPERATOR(name) name& operator=(const name& other) = delete;
#define DEFAULT_MOVE_CONSTRUCTOR(name) name(name&& other) noexcept = default;
#define DEFAULT_COPY_CONSTRUCTOR(name) name(const name& other) = default;
#define DEFAULT_MOVE_OPERATOR(name) name& operator=(name&& other) noexcept = default;
#define DEFAULT_COPY_OPERATOR(name) name& operator=(const name& other) = default;

template<typename THash>
struct Hash
{
	size_t operator()(const THash& hash) const
	{
		return std::hash<THash>{}(hash);
	}
};

#ifdef WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <WindowsX.h>
#include <sdkddkver.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dinput.h>
#include <hidusage.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include "Engine/render/d3dx12.h"
#include "dxgi.h"

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#ifdef _DEBUG || DEBUG
typedef HRESULT(WINAPI* pDXGIGetDebugInterface)(REFIID riid, void** pDebug);

const String WARNING = L"[Warning|Rythes]: ";
const String INFO = L"[  Info |Rythes]: ";
#define RY_WARN(message) OutputDebugString((WARNING + String{message}).c_str())
#define RY_INFO(message) OutputDebugString((INFO + String{message}).c_str())
#endif

using namespace Microsoft::WRL;

#if __cplusplus >= 202002L  // check cpp20
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;
#endif
#endif