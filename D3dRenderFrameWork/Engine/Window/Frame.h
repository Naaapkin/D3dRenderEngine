#pragma once
#ifdef WIN32
#include "Engine/pch.h"

// frame interface
class Frame
{
public:
    static Frame* CreateFrame(const String& title, uint16_t width, uint16_t height, bool isFullScreen = false);
    
    const String& Title() const;
    uint16_t Width() const;
    uint16_t Height() const;
    bool IsFullScreen() const;
    bool IsClosed() const;

    virtual void GetClientSize(uint16_t& width, uint16_t& height) const = 0;
    virtual void SetFrameSize(uint16_t width, uint16_t height);
    virtual void SetFullScreenMode(bool isFullScreen);
    virtual void Close();


    virtual ~Frame();
    DELETE_COPY_CONSTRUCTOR(Frame)
    DELETE_COPY_OPERATOR(Frame)
    Frame& operator=(Frame&& other) noexcept;
    
protected:
    Frame();
    Frame(const String& title, uint16_t width, uint16_t height, bool isFullScreen);
    Frame(Frame&& other) noexcept;
    
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
