#pragma once
#include "Engine/pch.h"

#if __cplusplus >= 202002L  // check cpp20
template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename Numeric, uint64_t Mul>
struct AlignUpToMul
{
    Numeric operator()(size_t num)
    {
        return (num + static_cast<Numeric>(Mul) - 1) / static_cast<Numeric>(Mul) * static_cast<Numeric>(Mul);
    }
};

template<typename Numeric>
struct AlignUpToMul<Numeric, 16>
{
    Numeric operator()(size_t num)
    {
        return num = (num + 15) & ~15;
    }
};

template<typename Numeric>
struct AlignUpToMul<Numeric, 256>
{
    Numeric operator()(size_t num)
    {
        return num = (num + 255) & ~255;
    }
};
#else
template<typename Numeric, uint64_t Mul, typename = std::enable_if_t<std::is_arithmetic<Numeric>::value>>
struct AlignUpToMul
{
    Numeric operator()(size_t num) const
    {
        return (num + static_cast<Numeric>(Mul) - 1) / static_cast<Numeric>(Mul) * static_cast<Numeric>(Mul);
    }
};

template<typename Numeric>
struct AlignUpToMul<Numeric, 16, std::enable_if_t<std::is_arithmetic<Numeric>::value>>
{
    Numeric operator()(size_t num) const
    {
        return num = (num + 15) & ~15;
    }
};

template<typename Numeric>
struct AlignUpToMul<Numeric, 256, std::enable_if_t<std::is_arithmetic_v<Numeric>>>
{
    Numeric operator()(size_t num) const
    {
        return num = (num + 255) & ~255;
    }
};

template<typename Numeric>
struct AlignUpToMul<Numeric, 512, std::enable_if_t<std::is_arithmetic_v<Numeric>>>
{
    Numeric operator()(size_t num) const
    {
        return num = (num + 511) & ~511;
    }
};
#endif

// Round up to the smallest power of 2 larger than or equal to x
static uint64_t RoundUpToPowerOfTwo(uint64_t x)
{
    if (x <= 1) return 1;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

// Round down to the largest power of 2 less than or equal to x
static uint64_t RoundDownToPowerOfTwo(uint64_t x)
{
    if (x == 0) return 0;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x - (x >> 1);
}

template<typename T, typename = void>
struct HashPtrAsTyped;

template<typename T, typename = void>
struct ComparePtrAsTyped;

// HashPtrAsTyped 特化
template<typename T>
struct HashPtrAsTyped<T, std::enable_if_t<std::is_pointer_v<T>>> {
private:
    using NakedType = std::remove_const_t<
        std::remove_pointer_t<T>
    >;

public:
    size_t operator()(const T key) const noexcept {
        return std::hash<NakedType>{}(*key);
    }
};

// ComparePtrAsTyped 特化
template<typename T>
struct ComparePtrAsTyped<T, std::enable_if_t<std::is_pointer_v<T>>> {
private:
    using NakedType = std::remove_const_t<
        std::remove_pointer_t<T>
    >;

public:
    bool operator()(const T lhs, const T rhs) const {
        return std::equal_to<NakedType>{}(*lhs, *rhs);
    }
};

inline std::string GetFileNameFromPath(const std::string& filePath, bool removeExt = false)
{
    // Find the last path separator ('\\' or '/')
    size_t pos = filePath.find_last_of("\\/");
    // If no separator is found, assume the entire string is the file name
    std::string fileName = (pos == std::string::npos) ? filePath : filePath.substr(pos + 1);

    // If requested, remove the file extension
    if (removeExt)
    {
        // Find the last dot in the file name
        size_t dotPos = fileName.find_last_of(L'.');
        // Ensure the dot is not the first character (to avoid hidden files on Unix-like systems)
        if (dotPos != std::wstring::npos && dotPos > 0)
        {
            fileName = fileName.substr(0, dotPos);
        }
    }
    return fileName;
}

inline std::wstring GetFileNameFromPath(const std::wstring& filePath, bool removeExt = false)
{
    // Find the last path separator ('\\' or '/')
    size_t pos = filePath.find_last_of(L"\\/");
    // If no separator is found, assume the entire string is the file name
    std::wstring fileName = (pos == std::wstring::npos) ? filePath : filePath.substr(pos + 1);

    // If requested, remove the file extension
    if (removeExt)
    {
        // Find the last dot in the file name
        size_t dotPos = fileName.find_last_of(L'.');
        // Ensure the dot is not the first character (to avoid hidden files on Unix-like systems)
        if (dotPos != std::wstring::npos && dotPos > 0)
        {
            fileName = fileName.substr(0, dotPos);
        }
    }
    return fileName;
}

inline std::string Utf8ToAscii(const std::wstring& wstr)
{
    char* str = new char[wstr.length() + 1];
    for (size_t i = 0; i < wstr.length(); i++)
    {
        str[i] = static_cast<char>(wstr[i]);
    }
    str[wstr.length()] = '\0';
    return str;
}

inline std::wstring AsciiToUtf8(const std::string& str)
{
    wchar_t* wstr = new wchar_t[str.length() + 1];
    for (size_t i = 0; i < str.length(); i++)
    {
        wstr[i] = static_cast<wchar_t>(str[i]);
    }
    wstr[str.length()] = '\0';
    return wstr;
}

struct NonCopyable
{
public:
    NonCopyable() = default;
    ~NonCopyable() = default;
    
protected:
    DELETE_COPY_OPERATOR(NonCopyable);
    DELETE_COPY_CONSTRUCTOR(NonCopyable);
    DEFAULT_MOVE_OPERATOR(NonCopyable);
    DEFAULT_MOVE_CONSTRUCTOR(NonCopyable);
};

struct NoHash
{
public:
    uint64_t operator()(uint64_t key) const
    {
        return key;
    }
};

template <typename T>
class Singleton {
public:
    NON_COPYABLE(Singleton<T>);
    NON_MOVEABLE(Singleton<T>);

    static T& GetInstance() {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

inline uint64_t MurmurHash (uint64_t a, uint64_t b)
{
    constexpr uint64_t m = 0xc6a4a7935bd1e995;
    constexpr int r = 47;

    uint64_t h = (a ^ (b << 32)) + (8 * m);

    a *= m;
    a ^= a >> r;
    a *= m;

    h ^= a;
    h *= m;

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    h ^= h >> 33;
    
    return h;
}

static uint32_t GetMipLevelCount(uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t maxDim = std::max({ width, height, depth });
    if (maxDim == 0) return 0;
    return static_cast<uint32_t>(std::floor(std::log2(maxDim))) + 1;
}