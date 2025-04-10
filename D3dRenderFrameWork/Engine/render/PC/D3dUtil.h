#pragma once
#ifdef WIN32
#include "Engine/pch.h"

template<typename T, typename = std::enable_if_t<std::is_base_of_v<IUnknown, T>>>
class UComPtr : NonCopyable
{
public:
    T* Get() const { return mPtr; }
    T* Reset(T* ptr) { std::swap(mPtr, ptr); return ptr; }
    T** GetAddressOf() { return &mPtr; }
    T* const* GetAddressOf() const { return &mPtr; }
    void Release() { if (mPtr) {mPtr->Release(); mPtr = nullptr; } }
    T* operator->() const { return mPtr; }
    UComPtr() : mPtr(nullptr) {}
    UComPtr(T* ptr) : mPtr(ptr) {}
    UComPtr(UComPtr&& other) noexcept : mPtr(other.mPtr) { other.mPtr = nullptr; }
    ~UComPtr() { Release(); }

    UComPtr& operator=(UComPtr&& other) noexcept { Release(); other.mPtr = nullptr; return *this; }
    
private:
    T* mPtr;
};

template<typename T>
class UComPtr<T[]>;

template<typename T, std::size_t N>
class UComPtr<T[N]>;

template<typename T>
struct alignas(256) Constant
{
    T value;

    Constant(T value) : value(value) { }
    operator T() const { return value; }
    T* operator->() { return value.operator->(); }
};

struct PassData
{
    // light info
    DirectX::XMVECTOR mLightDirection;
    DirectX::XMVECTOR mLightIntensity;
    DirectX::XMVECTOR mLightColor;
    DirectX::XMVECTOR mAmbientColor;
};

DXGI_FORMAT GetParaInfoFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc);
D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPipelineStateDesc();
uint64_t gGetConstantsBufferSize(ID3D12ShaderReflection* pReflector, const std::string& name);
uint64_t gGetConstantsBufferSize(ID3D12ShaderReflection* pReflector, const std::wstring& name);
bool gImplicitTransit(uint32_t stateBefore, uint32_t& stateAfter, bool isBufferOrSimultaneous);
#endif
