#pragma once
#include <Engine/pch.h>

struct ResourceHandle
{
    ResourceHandle(uint64_t idx) : mIdx(idx) { };
    
private:
    uint64_t mIdx;
};
