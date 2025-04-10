#pragma once
#ifdef WIN32
#include "Engine/pch.h"

class D3D12Device;

class D3D12DeviceChild : NonCopyable
{
public:
    virtual void release();

protected:
    void CreateD3D12Object(D3D12Device* parent);
    D3D12Device* Parent() const;
    ID3D12Device* Device() const;
    GUID Guid() const;
    virtual ID3D12DeviceChild* nativePtr() const;
    
    D3D12DeviceChild(D3D12Device* parent = nullptr);
    D3D12DeviceChild(D3D12DeviceChild&& other) noexcept;
    D3D12DeviceChild(ID3D12DeviceChild* pObject);;
    virtual ~D3D12DeviceChild();

    D3D12DeviceChild& operator=(D3D12DeviceChild&& other) noexcept;
    bool operator==(const D3D12DeviceChild& other) const noexcept;
    bool operator!=(const D3D12DeviceChild& other) const noexcept;

    std::string mName;
private:
    D3D12Device* mDevice;
    ID3D12DeviceChild* mObject;
};

inline ID3D12Device* D3D12DeviceChild::Device() const
{
    ID3D12Device* pDevice;
    mObject->GetDevice(IID_PPV_ARGS(&pDevice));
    return pDevice;
}

inline GUID D3D12DeviceChild::Guid() const
{
    GUID guid;
    mObject->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

inline ID3D12DeviceChild* D3D12DeviceChild::nativePtr() const
{
    return mObject;
}

inline void D3D12DeviceChild::release()
{
    if (!mObject) return;
    mObject->Release();
    mObject = nullptr;
}

inline void D3D12DeviceChild::CreateD3D12Object(D3D12Device* parent)
{
    mDevice = parent;
}

inline D3D12Device* D3D12DeviceChild::Parent() const
{
    return mDevice;
}

inline D3D12DeviceChild::D3D12DeviceChild(D3D12Device* parent) : mDevice(parent), mObject(nullptr)
{
}

inline D3D12DeviceChild::D3D12DeviceChild(D3D12DeviceChild&& other) noexcept : mDevice(other.mDevice), mObject(other.mObject)
{
    other.mObject = nullptr;
    other.mDevice = nullptr;
}

inline D3D12DeviceChild::D3D12DeviceChild(ID3D12DeviceChild* pObject) : mDevice(nullptr), mObject(pObject)
{
}

inline D3D12DeviceChild::~D3D12DeviceChild()
{
    D3D12DeviceChild::release();
}

inline D3D12DeviceChild& D3D12DeviceChild::operator=(D3D12DeviceChild&& other) noexcept
{
    if (&other != this)
    {
        mObject = other.mObject;
        other.mObject = nullptr;
    }
    return *this;
}

inline bool D3D12DeviceChild::operator==(const D3D12DeviceChild& other) const noexcept
{
    return mObject == other.mObject;
}

inline bool D3D12DeviceChild::operator!=(const D3D12DeviceChild& other) const noexcept
{
    return mObject != other.mObject;
}
#endif
