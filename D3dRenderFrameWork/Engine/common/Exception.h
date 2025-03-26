#pragma once
#include "Engine/pch.h"

class Exception
{
public:
    Exception() = default;
    Exception(const Char* message) : mMessage(message), mFile(nullptr), mFunction(nullptr), mLine(-1) {}
    Exception(const Char* message, const Char* file, const Char* function, uint32_t line) : mMessage(message), mFile(file), mFunction(function), mLine(line) {}

    const Char* Message() const
    {
        return mMessage;
    }

    const Char* File() const
    {
        return mFile;
    }

    const Char* Function() const
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
        return TEXT("Exception: ") + String(mMessage) + TEXT(" at ") + String(mFile) + TEXT(", ") + String(mFunction) + TEXT(", LINE ") + TO_STRING(mLine);
    }

private:
    const Char* mMessage;
    const Char* mFile;
    const Char* mFunction;
    uint32_t mLine;
};

#define THROW_EXCEPTION(message) throw Exception(message, __REFLECTION_FILE_NAME__, __REFLECTION_FUNC_NAME__, __LINE__);
#define ASSERT(condition, message) if (!(condition)) THROW_EXCEPTION(message)

#ifdef WIN32
#include <Engine/common/PC/WFunc.h>
#define ThrowIfFailed(x) \
{ \
HRESULT hr = (x); \
if(FAILED(hr)) { \
throw Exception(static_cast<const Char*>(WFunc::GetHRInfo(hr)), __FILEW__, L#x, __LINE__); \
} \
}
#endif