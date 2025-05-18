#ifdef WIN32
#include "Engine/Window/Frame.h"

const String& Frame::Title() const
{
    return mTitle;
}

uint16_t Frame::Width() const
{
    return mFrameWidth;
}

uint16_t Frame::Height() const
{
    return mFrameHeight;
}

bool Frame::IsFullScreen() const
{
    return mIsFullScreen;
}

void Frame::SetFrameSize(uint16_t width, uint16_t height)
{
    this->mFrameWidth = width;
    this->mFrameHeight = height;
}

bool Frame::IsClosed() const
{
    return mIsClosed;
}

void Frame::Close()
{
    mIsClosed = true;
}

void Frame::SetFullScreenMode(bool isFullScreen)
{
    mIsFullScreen = isFullScreen;
}

Frame::Frame() :
    mTitle(DEFAULT_FRAME_TITLE),
    mFrameWidth(DEFAULT_FRAME_WIDTH),
    mFrameHeight(DEFAULT_FRAME_HEIGHT),
    mIsFullScreen(false),
    mIsClosed(false) { }

Frame::Frame(const String& title, uint16_t width, uint16_t height, bool isFullScreen = false) :
    mTitle(title),
    mFrameWidth(width),
    mFrameHeight(height),
    mIsFullScreen(isFullScreen),
    mIsClosed(false) { }

Frame::Frame(Frame&& other)  noexcept :
    mTitle(std::move(other.mTitle)),
    mFrameWidth(DEFAULT_FRAME_WIDTH),
    mFrameHeight(DEFAULT_FRAME_HEIGHT),
    mIsFullScreen(other.mIsFullScreen),
    mIsClosed(other.mIsClosed)
{
    other.mIsClosed = true;
}

Frame::~Frame() = default;

Frame& Frame::operator=(Frame&& other) noexcept
{
    mTitle = std::move(other.mTitle);
    mFrameWidth = other.mFrameWidth;
    mFrameHeight = other.mFrameHeight;
    mIsFullScreen = other.mIsFullScreen;
    mIsClosed = other.mIsClosed;
    other.mIsClosed = true;
    return *this;
}
#endif
