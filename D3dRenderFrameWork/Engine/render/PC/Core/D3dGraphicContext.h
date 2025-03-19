#pragma once
#ifdef WIN32
#include <Engine/pch.h>
#include <Engine/render/PC/D3dRenderer.h>

class D3dGraphicContext
{
    friend D3dRenderer* ::gNativeRenderer();
    
public:
    ID3D12Device* DeviceHandle() const;
    IDXGIFactory4* FactoryHandle() const;
    ~D3dGraphicContext();
    
    DELETE_COPY_OPERATOR(D3dGraphicContext)
    DELETE_COPY_CONSTRUCTOR(D3dGraphicContext);
    DELETE_MOVE_OPERATOR(D3dGraphicContext);
    DELETE_MOVE_CONSTRUCTOR(D3dGraphicContext);

private:
#ifdef DEBUG || _DEBUG
    ComPtr<ID3D12InfoQueue> mInfoQueue;
#endif
    ComPtr<IDXGIFactory4> mFactoryHandle;
    // IDXGIAdapter** mAdapters = nullptr;
    ComPtr<ID3D12Device> mDeviceHandle;
    
    D3dGraphicContext();
};

#endif