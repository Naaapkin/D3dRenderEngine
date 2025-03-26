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
struct AlignUpToMul<Numeric, 256, std::enable_if_t<std::is_arithmetic<Numeric>::value>>
{
    Numeric operator()(size_t num) const
    {
        return num = (num + 255) & ~255;
    }
};
#endif

template<typename T, typename = void>
struct HashPtrAsTyped;

template<typename T>
struct HashPtrAsTyped<T, std::enable_if_t<std::is_pointer_v<T> || std::is_array_v<T>>> {
    size_t operator()(const T& key) const noexcept {
        using PointerType = std::remove_pointer_t<T>;
        return std::hash<PointerType>{}(*key);
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