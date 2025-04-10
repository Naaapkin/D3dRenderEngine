#pragma once
#include "Engine/pch.h"

enum class ResourceState : uint32_t;

struct ResourceHandle
{
    uint64_t mIndex;

    ResourceHandle() = default;
    ResourceHandle(uint64_t index) : mIndex(index) { }

    ~ResourceHandle() = default;
    DEFAULT_COPY_CONSTRUCTOR(ResourceHandle)
    DEFAULT_COPY_OPERATOR(ResourceHandle)
    DEFAULT_MOVE_CONSTRUCTOR(ResourceHandle)
    DEFAULT_MOVE_OPERATOR(ResourceHandle)
};