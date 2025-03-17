
#ifdef WIN32
#include "Frame.h"

const String& IFrame::Title() const
{
    return mTitle;
}

uint16_t IFrame::Width() const
{
    return mFrameWidth;
}

uint16_t IFrame::Height() const
{
    return mFrameHeight;
}

bool IFrame::IsFullScreen() const
{
    return mIsFullScreen;
}

void IFrame::SetFrameSize(uint16_t width, uint16_t height)
{
    this->mFrameWidth = width;
    this->mFrameHeight = height;
}

bool IFrame::IsClosed() const
{
    return mIsClosed;
}

void IFrame::Close()
{
    mIsClosed = true;
}

void IFrame::SetFullScreenMode(bool isFullScreen)
{
    mIsFullScreen = isFullScreen;
}

IFrame::IFrame() :
    mTitle(DEFAULT_FRAME_TITLE),
    mFrameWidth(DEFAULT_FRAME_WIDTH),
    mFrameHeight(DEFAULT_FRAME_HEIGHT),
    mIsFullScreen(false),
    mIsClosed(false) { }

IFrame::IFrame(const String& title, uint16_t width, uint16_t height, bool isFullScreen = false) :
    mTitle(title),
    mFrameWidth(DEFAULT_FRAME_WIDTH),
    mFrameHeight(DEFAULT_FRAME_HEIGHT),
    mIsFullScreen(isFullScreen),
    mIsClosed(false) { }

IFrame::IFrame(IFrame&& other)  noexcept :
    mTitle(std::move(other.mTitle)),
    mFrameWidth(DEFAULT_FRAME_WIDTH),
    mFrameHeight(DEFAULT_FRAME_HEIGHT),
    mIsFullScreen(other.mIsFullScreen),
    mIsClosed(other.mIsClosed)
{
    other.mIsClosed = true;
}

IFrame::~IFrame() = default;

IFrame& IFrame::operator=(IFrame&& other) noexcept
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
