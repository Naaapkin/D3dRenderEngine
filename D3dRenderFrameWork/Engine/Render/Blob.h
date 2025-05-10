#pragma once
#include "Engine/pch.h"
class Blob
{
public:
    void Reserve(uint64_t size);
    void CopyFrom(const void* pData, uint64_t size) const;
    const byte* Binary() const;
    uint64_t Size() const;
    void Release();
    Blob();
    Blob(const void* binary, size_t size);
    Blob(size_t size);
    Blob(Blob&& other) noexcept;
    ~Blob();

    Blob& operator=(Blob&& other) noexcept;

    NON_COPYABLE(Blob)

private:
    byte* mBinary;
    uint64_t mSize;
};

inline void Blob::Reserve(uint64_t size)
{
    if (size && size > mSize)
    {
        byte* newBinary = new byte[size];
        memcpy(newBinary, mBinary, mSize);
        delete[] mBinary;
        mBinary = newBinary;
        mSize = size;
    }
}
#undef min
inline void Blob::CopyFrom(const void* pData, uint64_t size) const
{
    memcpy(mBinary, pData, std::min(mSize, size));
}

inline const byte* Blob::Binary() const
{
    return mBinary;
}

inline uint64_t Blob::Size() const
{
    return mSize;
}

inline void Blob::Release()
{
    mSize = 0;
    delete[] mBinary;
    mBinary = nullptr;
}

inline Blob::Blob(): mBinary(nullptr), mSize(0)
{}

inline Blob::Blob(const void* binary, size_t size): mBinary(size ? new byte[size] : nullptr), mSize(size)
{
    memcpy(mBinary, binary, size);
}

inline Blob::Blob(size_t size) : mBinary(new byte[size]), mSize(size) { }

inline Blob::Blob(Blob&& other) noexcept : mBinary(other.mBinary), mSize(other.mSize)
{
    other.mBinary = nullptr;
    other.mSize = 0;
}

inline Blob& Blob::operator=(Blob&& other) noexcept
{
    if (this != &other)
    {
        mBinary = other.mBinary;
        mSize = other.mSize;
    
        other.mBinary = nullptr;
        other.mSize = 0;
    }
    return *this;
}

inline Blob::~Blob()
{
    Release();
}
