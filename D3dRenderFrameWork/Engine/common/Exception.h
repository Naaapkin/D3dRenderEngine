#pragma once
#include "Engine/pch.h"

class Exception
{
public:
    Exception() = default;
    Exception(const Char* message) : mMessage(message), mFile(nullptr), mLine(-1) {}
    Exception(const Char* message, const Char* file, uint32_t line) : mMessage(message), mFile(file), mLine(line) {}

    const Char* Message() const
    {
        return mMessage;
    }

    const Char* File() const
    {
        return mFile;
    }

    uint32_t Line() const
    {
        return mLine;
    }

    String ToString() const
    {
        // return std::format(L"Exception: {} at {}, {}, LINE {}", mMessage, mFile, mFunction, mLine);
        return TEXT("Exception: ") + String(mMessage) + TEXT(" at ") + String(mFile) + TEXT(", ") + TEXT(", LINE ") + TO_STRING(mLine);
    }

private:
    const Char* mMessage;
    const Char* mFile;
    uint32_t mLine;
};

#define THROW_EXCEPTION(message) throw Exception(message, __REFLECTION_FILE_NAME__, __LINE__);
#define ASSERT(condition, message) if (!(condition)) THROW_EXCEPTION(message)