#pragma once
#include "pch.h"
#include "WFunc.h"

#define ThrowIfFailed(x) \
{ \
HRESULT hr = (x); \
if(FAILED(hr)) { \
throw Exception(static_cast<const TCHAR*>(WFunc::GetHRInfo(hr)), __FILEW__, L#x, __LINE__); \
} \
}
    
#define THROW_EXCEPTION(message) throw Exception(message, __FILEW__, __FUNCTIONW__, __LINE__);
#define ASSERT(condition, message) if (!(condition)) THROW_EXCEPTION(message)
    
class Exception
{
public:
    Exception() = default;
    Exception(const TCHAR* message) : mMessage(message), mFile(nullptr), mFunction(nullptr), mLine(-1) {}
    Exception(const TCHAR* message, const TCHAR* file, const TCHAR* function, uint32_t line) : mMessage(message), mFile(file), mFunction(function), mLine(line) {}

    const TCHAR* Message() const
    {
        return mMessage;
    }

    const TCHAR* File() const
    {
        return mFile;
    }

    const TCHAR* Function() const
    {
        return mFunction;
    }

    uint32_t Line() const
    {
        return mLine;
    }

    String ToString() const
    {
        // return std::format(L"Exception: {} at {}, {}, LINE {}", mMessage, mFile, mFunction, mLine);
        return L"Exception: " + String(mMessage) + L" at " + String(mFile) + L", " + String(mFunction) + L", LINE " + TO_STRING(mLine);
    }

private :
    const TCHAR* mMessage;
    const TCHAR* mFile;
    const TCHAR* mFunction;
    uint32_t mLine;
};