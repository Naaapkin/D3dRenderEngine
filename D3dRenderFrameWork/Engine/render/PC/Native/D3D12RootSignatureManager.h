#pragma once
#ifdef WIN32
#include "D3D12Device.h"
#include "Engine/common/Exception.h"
#include "Engine/common/helper.h"
#include "Engine/render/PC/D3dUtil.h"

struct D3D12RootParameter
{
    uint32_t mRootParameterIndex;
    D3D12_ROOT_PARAMETER_TYPE mType;
    union ParameterData
    {
        D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
        D3D12_GPU_DESCRIPTOR_HANDLE mDescriptorHandle;
        uint32_t m32BitConstant;
    } mParameter;

    D3D12RootParameter() = default;
    D3D12RootParameter(uint32_t index, D3D12_ROOT_PARAMETER_TYPE type, ParameterData parameter) : mRootParameterIndex(index), mType(type), mParameter(parameter) { }

    bool operator==(const D3D12RootParameter& other) const
    {
        return mRootParameterIndex == other.mRootParameterIndex;
    }
    
    bool operator<(const D3D12RootParameter& other) const
    {
        return mRootParameterIndex < other.mRootParameterIndex;
    }
};

struct RootSignatureLayout
{
    uint8_t mNumTextures;
    uint8_t mNumMaterialConstants;
};

struct D3D12RootSignature
{
    UComPtr<ID3D12RootSignature> mRootSignature;
    // std::unique_ptr<D3D12_ROOT_PARAMETER_TYPE[]> mParameters;
    uint32_t mNumRootParameters;
    RootSignatureLayout mLayout;

    D3D12RootSignature() = default;
    D3D12RootSignature(UComPtr<ID3D12RootSignature>&& pRootSignature, const D3D12_ROOT_PARAMETER_TYPE* parameters, uint32_t numRootParameters, RootSignatureLayout layout) :
        mRootSignature(std::move(pRootSignature)), mNumRootParameters(numRootParameters), mLayout(layout)
    {
        // mParameters.reset(new D3D12_ROOT_PARAMETER_TYPE[numRootParameters]);
        // memcpy(mParameters.get(), parameters, sizeof(D3D12_ROOT_PARAMETER_TYPE) * numRootParameters);
    }
};

class D3D12RootSignatureManager : NonCopyable
{
    friend class Singleton<D3D12RootSignatureManager>;
public:
    void Initialize(D3D12Device* pDevice);

    // 获取根签名索引（允许多线程并发读）
    uint64_t GetIndexOfName(const std::string& name);
    
    // 通过名称获取根签名（允许多线程并发读）
    const D3D12RootSignature* GetByName(const std::string& name) const;
    
    // 通过索引获取根签名（允许多线程并发读）
    const D3D12RootSignature* GetByIndex(uint64_t index) const;
    
    // 添加新根签名（单线程写，独占锁）
    uint64_t AppendSignature(const std::string& name, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc, const RootSignatureLayout& layout);
    D3D12RootSignatureManager();
    D3D12RootSignatureManager(D3D12Device* pDevice);;

private:
    D3D12Device* mDevice;
    std::unordered_map<std::string, uint64_t> mNameIndexMap; // Key: 名称, Value: 索引
    std::vector<D3D12RootSignature> mRootSignatures;    // 存储所有根签名
    mutable std::shared_mutex mMutex; // 读写锁（C++17）
};

inline void D3D12RootSignatureManager::Initialize(D3D12Device* pDevice)
{
    ASSERT(pDevice, TEXT("device can not be nullptr."))
    mDevice = pDevice;
}

inline uint64_t D3D12RootSignatureManager::GetIndexOfName(const std::string& name)
{
    std::shared_lock<std::shared_mutex> lock(mMutex); // 共享锁（读锁）
    const auto it = mNameIndexMap.find(name);
    return (it != mNameIndexMap.end()) ? it->second : UINT64_MAX;
}

inline const D3D12RootSignature* D3D12RootSignatureManager::GetByName(const std::string& name) const
{
    std::shared_lock<std::shared_mutex> lock(mMutex); // 共享锁（读锁）
    const auto it = mNameIndexMap.find(name);
    return (it != mNameIndexMap.end()) ? &(mRootSignatures[it->second]) : nullptr;
}

inline const D3D12RootSignature* D3D12RootSignatureManager::GetByIndex(uint64_t index) const
{
    std::shared_lock<std::shared_mutex> lock(mMutex); // 共享锁（读锁）
    return (index < mRootSignatures.size()) ? &(mRootSignatures[index]) : nullptr;
}

inline uint64_t D3D12RootSignatureManager::AppendSignature(const std::string& name,
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc, const RootSignatureLayout& layout)
{
    // first check, read lock
    {
        std::shared_lock<std::shared_mutex> readLock(mMutex); // 读锁
        const auto it = mNameIndexMap.find(name);
        if (it != mNameIndexMap.end()) return it->second;
    }

    UComPtr<ID3D12RootSignature> pSignature = mDevice->CreateRootSignature(desc);
    // std::unique_ptr<D3D12_ROOT_PARAMETER_TYPE[]> pRootParameterTypes;
    uint32_t numRootParameters = desc.Version == D3D_ROOT_SIGNATURE_VERSION_1_0 ? desc.Desc_1_0.NumParameters : desc.Desc_1_1.NumParameters;
    // D3D12_ROOT_PARAMETER_TYPE* types = desc.Version == D3D_ROOT_SIGNATURE_VERSION_1_0 ? desc.Desc_1_0.pParameters->ParameterType : desc.Desc_1_1.pParameters->ParameterType;
    // pRootParameterTypes.reset(new D3D12_ROOT_PARAMETER_TYPE[numRootParameters]);
    // memcpy(pRootParameterTypes.get(), types, numRootParameters * sizeof(D3D12_ROOT_PARAMETER_TYPE));
    if (!pSignature) return UINT64_MAX;

    // write lock
    std::unique_lock<std::shared_mutex> writeLock(mMutex); // 写锁

    // double check 
    const auto it = mNameIndexMap.find(name);
    if (it != mNameIndexMap.end())
        return it->second;

    uint64_t index = mRootSignatures.size();
    mNameIndexMap.emplace(name, index);
    mRootSignatures.emplace_back(std::move(pSignature), nullptr, numRootParameters, layout);
    return index;
}

inline D3D12RootSignatureManager::D3D12RootSignatureManager() = default;

inline D3D12RootSignatureManager::D3D12RootSignatureManager(D3D12Device* pDevice): mDevice(pDevice)
{}
#endif