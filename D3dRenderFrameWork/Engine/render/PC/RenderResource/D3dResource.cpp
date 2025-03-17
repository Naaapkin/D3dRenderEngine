#ifdef WIN32
#include <Engine/render/PC/RenderResource/D3dResource.h>
#include <Engine/common/PC/WException.h>

ID3D12Resource* D3dResource::NativePtr() const
{
    return mResource.Get();
}

uint64_t D3dResource::SubResourceCount() const
{
    return mSubResourceCount;
}

ResourceState* D3dResource::ResourceStates() const
{
	return mResourceStates;
}

GUID D3dResource::GetGuid() const
{
    GUID guid;
    mResource->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

void D3dResource::Release()
{
    mResource.Reset();
    delete[] mResourceStates;
    mResourceStates = nullptr;
}

D3dResource::~D3dResource()
{
    D3dResource::Release();
}

D3dResource::D3dResource(ID3D12Resource* pResource, 
                         uint64_t subResourceCount, 
						 ResourceState* resourceStates) :
		mResource(pResource),
		mSubResourceCount(subResourceCount),
		mResourceStates(resourceStates) { }

D3dResource CreateD3dResource(ID3D12Device* pDevice, D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES& heapProp,
                              const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState)
{
    uint64_t subResourceCount = static_cast<uint64_t>(desc.MipLevels) * desc.DepthOrArraySize * D3D12GetFormatPlaneCount(pDevice, desc.Format);
    ID3D12Resource* pBuffer = nullptr;
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &heapProp,
        heapFlags,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&pBuffer)
    ));
    auto pResource = pBuffer;
    auto resourceGroupStates = new ResourceState[subResourceCount];
    memset(resourceGroupStates, initialState, subResourceCount);
    return { pResource, subResourceCount, resourceGroupStates };
}

#endif