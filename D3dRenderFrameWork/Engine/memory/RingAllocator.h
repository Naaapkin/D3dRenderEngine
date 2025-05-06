#pragma once
#include "Engine/pch.h"
#include "Engine/common/helper.h"

template <typename T>
class RingAllocator : NonCopyable {
public:
    // 分配句柄
    class Handle
    {
    public:
        Handle() = default;
        
        explicit Handle(uint64_t offset, RingAllocator<T>* allocator)
            : m_offset(offset), m_allocator(allocator) {}
            
        T* Get() const {
            return m_allocator ? m_allocator->Resolve(m_offset) : nullptr;
        }
        
        T& operator*() const { return *Get(); }
        T* operator->() const { return Get(); }
        explicit operator bool() const { return m_allocator != nullptr; }

    private:
        uint64_t m_offset = 0;
        RingAllocator<T>* m_allocator = nullptr;
    };

    // 构造函数
    explicit RingAllocator(size_t capacity)
        : m_capacity(capacity), 
          m_buffer(std::make_unique<T[]>(capacity)) {}
    
    // 分配元素
    Handle Allocate() {
        uint64_t current_head = m_head.load(std::memory_order_relaxed);
        uint64_t current_tail = m_tail.load(std::memory_order_acquire);
        
        // 检查是否有空间
        if ((current_head + 1) % m_capacity == current_tail) {
            return Handle(); // 缓冲区已满
        }
        
        // 分配位置
        uint64_t alloc_pos = current_head;
        
        // 尝试移动head指针
        while (!m_head.compare_exchange_weak(
            current_head, 
            (current_head + 1) % m_capacity,
            std::memory_order_release,
            std::memory_order_relaxed)) {
            
            // CAS失败，重新检查空间
            current_tail = m_tail.load(std::memory_order_acquire);
            if ((current_head + 1) % m_capacity == current_tail) {
                return Handle();
            }
        }
        
        return Handle(alloc_pos, this);
    }
    
    // 释放元素
    void Free(const Handle& handle) {
        if (!handle) return;
        
        uint64_t current_tail = m_tail.load(std::memory_order_relaxed);
        uint64_t expected_tail = handle.m_offset;
        
        // 确保释放顺序正确
        while (true) {
            if (current_tail != expected_tail) {
                // 不是下一个要释放的元素，等待
                std::this_thread::yield();
                current_tail = m_tail.load(std::memory_order_relaxed);
                continue;
            }
            
            // 尝试移动tail指针
            if (m_tail.compare_exchange_weak(
                current_tail,
                (current_tail + 1) % m_capacity,
                std::memory_order_release,
                std::memory_order_relaxed)) {
                break;
            }
        }
    }
    
    // 解析句柄
    T* Resolve(uint64_t offset) const {
        if (offset >= m_capacity) return nullptr;
        return &m_buffer[offset];
    }
    
    // 获取容量
    size_t Capacity() const { return m_capacity; }
    
    // 获取可用空间
    size_t Available() const {
        uint64_t head = m_head.load(std::memory_order_acquire);
        uint64_t tail = m_tail.load(std::memory_order_acquire);
        
        if (head >= tail) {
            return m_capacity - (head - tail) - 1;
        } else {
            return tail - head - 1;
        }
    }

private:
    const size_t m_capacity;
    std::unique_ptr<T[]> m_buffer;
    
    // 原子计数器
    std::atomic<uint64_t> m_head{0}; // 下一个分配位置
    std::atomic<uint64_t> m_tail{0}; // 下一个释放位置
};
