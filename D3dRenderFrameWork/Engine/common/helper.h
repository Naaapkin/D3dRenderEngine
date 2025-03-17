#pragma once
#include <pch.h>

#if __cplusplus >= 202002L  // check cpp20
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

inline std::string GetFileNameFromPath(const std::string& filePath)
{
    size_t pos = filePath.find_last_of("/\\");
    return (pos == std::string::npos) ? filePath : filePath.substr(pos + 1);
}

inline std::wstring GetFileNameFromPath(const std::wstring& filePath)
{
    size_t pos = filePath.find_last_of(L"/\\");
    return (pos == std::wstring::npos) ? filePath : filePath.substr(pos + 1);
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