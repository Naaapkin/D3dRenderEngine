#pragma once
#ifdef WIN32
#include "Engine/pch.h"
class D3dObject
{
public:
    virtual void release();

protected:
    ID3D12Device* device() const;
    GUID getGuid() const;
    virtual ID3D12DeviceChild* nativePtr() const;

    D3dObject();
    D3dObject(D3dObject&& other) noexcept;
    D3dObject(ID3D12DeviceChild* pObject);;
    virtual ~D3dObject();

    D3dObject& operator=(D3dObject&& other) noexcept;
    bool operator==(const D3dObject& other) const noexcept;
    bool operator!=(const D3dObject& other) const noexcept;
    
    DELETE_COPY_OPERATOR(D3dObject);
    DELETE_COPY_CONSTRUCTOR(D3dObject);

private:
    ID3D12DeviceChild* mObject;
};

inline ID3D12Device* D3dObject::device() const
{
    ID3D12Device* pDevice;
    mObject->GetDevice(IID_PPV_ARGS(&pDevice));
    return pDevice;
}

inline GUID D3dObject::getGuid() const
{
    GUID guid;
    mObject->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

inline ID3D12DeviceChild* D3dObject::nativePtr() const
{
    return mObject;
}

inline void D3dObject::release()
{
    if (!mObject) return;
    mObject->Release();
    mObject = nullptr;
}

inline D3dObject::D3dObject() = default;

inline D3dObject::D3dObject(D3dObject&& other) noexcept : mObject(other.mObject)
{
    other.mObject = nullptr;
}

inline D3dObject::D3dObject(ID3D12DeviceChild* pObject) : mObject(pObject)
{
}

inline D3dObject::~D3dObject()
{
    D3dObject::release();
}

inline D3dObject& D3dObject::operator=(D3dObject&& other) noexcept
{
    if (&other != this)
    {
        mObject = other.mObject;
        other.mObject = nullptr;
    }
    return *this;
}

inline bool D3dObject::operator==(const D3dObject& other) const noexcept
{
    return mObject == other.mObject;
}

inline bool D3dObject::operator!=(const D3dObject& other) const noexcept
{
    return mObject != other.mObject;
}
#endif
