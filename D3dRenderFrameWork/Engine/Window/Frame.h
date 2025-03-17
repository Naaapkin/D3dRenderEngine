#pragma once
#include "pch.h"

#ifdef WIN32
class IFrame
{
public:
    static IFrame* CreateFrame(const String& title, uint16_t width, uint16_t height, bool isFullScreen = false);
    
    const String& Title() const;
    uint16_t Width() const;
    uint16_t Height() const;
    bool IsFullScreen() const;
    bool IsClosed() const;

    virtual void GetClientSize(uint16_t& width, uint16_t& height) const = 0;
    virtual void SetFrameSize(uint16_t width, uint16_t height);
    virtual void SetFullScreenMode(bool isFullScreen);
    virtual void Close();

    IFrame();
    IFrame(const String& title, uint16_t width, uint16_t height, bool isFullScreen);
    IFrame(IFrame&& other) noexcept;
    virtual ~IFrame();
    DELETE_COPY_CONSTRUCTOR(IFrame)
    DELETE_COPY_OPERATOR(IFrame)
    IFrame& operator=(IFrame&& other) noexcept;
    
protected:
    constexpr static uint16_t DEFAULT_FRAME_WIDTH = 1280;
    constexpr static uint16_t DEFAULT_FRAME_HEIGHT = 720;
    constexpr static TCHAR DEFAULT_FRAME_TITLE[] = TEXT("Frame");

private:
    String mTitle;
    uint16_t mFrameWidth;
    uint16_t mFrameHeight;
    bool mIsFullScreen;
    bool mIsClosed;
};
#endif
