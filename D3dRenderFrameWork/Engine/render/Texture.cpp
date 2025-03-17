#include <render/Texture.h>

TextureType Texture::Type() const
{
    return mType;
}

TextureFormat Texture::Format() const
{
    return mFormat;
}

uint32_t Texture::Width() const
{
    return mWidth;
}

uint32_t Texture::Height() const
{
    return mHeight;
}

uint32_t Texture::Depth() const
{
    return mDepth;
}

uint8_t Texture::MipLevels() const
{
    return mNumMips;
}

uint8_t Texture::SampleCount() const
{
    return mSampleCount;
}

uint8_t Texture::SampleQuality() const
{
    return mSampleQuality;
}

Texture::Texture(TextureType type, uint64_t width, uint64_t height, uint32_t depth, TextureFormat format,
    uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality) :
    mType(type), mFormat(format), mNumMips(numMips),
    mWidth(width), mHeight(height), mDepth(depth),
    mSampleCount(sampleCount), mSampleQuality(sampleQuality) { }

Texture::Texture(const Texture& o) noexcept
{
    mType = o.mType;
    mFormat = o.mFormat;
    mWidth = o.mWidth;
    mHeight = o.mHeight;
    mDepth = o.mDepth;
    mNumMips = o.mNumMips;
    mSampleCount = o.mSampleCount;
    mSampleQuality = o.mSampleQuality;
}

Texture::~Texture() = default;

Texture& Texture::operator=(const Texture& o) noexcept
{
    if (this != &o)
    {
        mDepth = o.mDepth;
        mWidth = o.mWidth;
        mHeight = o.mHeight;
        mNumMips = o.mNumMips;
        mFormat = o.mFormat;
        mType = o.mType;
        mSampleCount = o.mSampleCount;
        mSampleQuality = o.mSampleQuality;
    }
    return *this;
}