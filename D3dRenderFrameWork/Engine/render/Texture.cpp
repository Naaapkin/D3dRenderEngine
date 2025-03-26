#include <Engine/render/Texture.h>

TextureType Texture::Type() const
{
    return mType;
}

TextureFormat Texture::Format() const
{
    return mFormat;
}

uint64_t Texture::Width() const
{
    return mWidth;
}

uint64_t Texture::Height() const
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
    mSampleCount(sampleCount), mSampleQuality(sampleQuality), mDepth(depth),
    mWidth(width), mHeight(height) { }

Texture::Texture(const Texture& o) noexcept = default;

Texture::Texture(TextureType type, const D3D12_RESOURCE_DESC& desc) :
        mType(type), mFormat(static_cast<TextureFormat>(desc.Format)), mNumMips(static_cast<uint8_t>(desc.MipLevels)), mSampleCount(static_cast<uint8_t>(desc.SampleDesc.Count)),
        mSampleQuality(static_cast<uint8_t>(desc.SampleDesc.Quality)), mDepth(desc.DepthOrArraySize),
        mWidth(desc.Width), mHeight(desc.Height) { }

Texture::Texture() :
    mType(), mFormat(),
    mNumMips(), mSampleCount(), mSampleQuality(),
    mDepth(), mWidth(),
    mHeight() { }

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