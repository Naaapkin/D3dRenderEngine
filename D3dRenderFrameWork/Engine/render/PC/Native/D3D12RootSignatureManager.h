#pragma once
#include "D3D12Device.h"
#include "Engine/common/Exception.h"
#include "Engine/common/helper.h"
#include "Engine/render/PC/D3dUtil.h"

class D3D12RootSignatureManager : public Singleton<D3D12RootSignatureManager>
{
    friend class Singleton<D3D12RootSignatureManager>;
public:
    void Initialize(D3D12Device* pDevice);

    // 获取根签名索引（允许多线程并发读）
    uint64_t GetIndexOfName(const std::string& name);
    
    // 通过名称获取根签名（允许多线程并发读）
    ID3D12RootSignature* GetByName(const std::string& name) const;
    
    // 通过索引获取根签名（允许多线程并发读）
    ID3D12RootSignature* GetByIndex(uint64_t index) const;
    
    // 添加新根签名（单线程写，独占锁）
    uint64_t AppendSignature(const std::string& name, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

private:
    D3D12RootSignatureManager();
    D3D12RootSignatureManager(D3D12Device* pDevice);

    D3D12Device* mDevice;
    std::unordered_map<std::string, uint64_t> mSignatureIndexMap; // Key: 名称, Value: 索引
    std::vector<UComPtr<ID3D12RootSignature>> mRootSignatures;    // 存储所有根签名
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
    const auto it = mSignatureIndexMap.find(name);
    return (it != mSignatureIndexMap.end()) ? it->second : UINT64_MAX;
}

inline ID3D12RootSignature* D3D12RootSignatureManager::GetByName(const std::string& name) const
{
    std::shared_lock<std::shared_mutex> lock(mMutex); // 共享锁（读锁）
    const auto it = mSignatureIndexMap.find(name);
    return (it != mSignatureIndexMap.end()) ? mRootSignatures[it->second].Get() : nullptr;
}

inline ID3D12RootSignature* D3D12RootSignatureManager::GetByIndex(uint64_t index) const
{
    std::shared_lock<std::shared_mutex> lock(mMutex); // 共享锁（读锁）
    return (index < mRootSignatures.size()) ? mRootSignatures[index].Get() : nullptr;
}

inline uint64_t D3D12RootSignatureManager::AppendSignature(const std::string& name,
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
{
    // 第一阶段：快速检查（无锁）
    {
        std::shared_lock<std::shared_mutex> readLock(mMutex); // 读锁
        const auto it = mSignatureIndexMap.find(name);
        if (it != mSignatureIndexMap.end()) return it->second;
    }

    // 第二阶段：创建根签名（无锁，可能耗时）
    UComPtr<ID3D12RootSignature> pSignature = mDevice->CreateRootSignature(desc);
    if (!pSignature) return UINT64_MAX;

    // 第三阶段：更新容器（加写锁）
    std::unique_lock<std::shared_mutex> writeLock(mMutex); // 写锁

    // 双重检查：防止其他线程已插入同名签名
    const auto it = mSignatureIndexMap.find(name);
    if (it != mSignatureIndexMap.end())
        return it->second;

    // 存储并返回索引
    uint64_t index = mRootSignatures.size();
    mSignatureIndexMap.emplace(name, index);
    mRootSignatures.push_back(std::move(pSignature));
    return index;
}

inline D3D12RootSignatureManager::D3D12RootSignatureManager() = default;

inline D3D12RootSignatureManager::D3D12RootSignatureManager(D3D12Device* pDevice): mDevice(pDevice)
{}
