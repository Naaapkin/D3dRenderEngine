#include <Engine/render/Texture.h>

TextureType Texture::type() const
{
    return mType;
}

Format Texture::format() const
{
    return mFormat;
}

uint64_t Texture::width() const
{
    return mWidth;
}

uint64_t Texture::height() const
{
    return mHeight;
}

uint32_t Texture::depth() const
{
    return mDepth;
}

uint8_t Texture::mipLevels() const
{
    return mNumMips;
}

uint8_t Texture::sampleCount() const
{
    return mSampleCount;
}

uint8_t Texture::sampleQuality() const
{
    return mSampleQuality;
}

Texture::Texture(TextureType type, uint64_t width, uint64_t height, uint32_t depth, Format format,
    uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality) :
    mType(type), mFormat(format), mNumMips(numMips),
    mSampleCount(sampleCount), mSampleQuality(sampleQuality), mDepth(depth),
    mWidth(width), mHeight(height) { }

Texture::Texture(const Texture& o) noexcept = default;

Texture::Texture(TextureType type, const D3D12_RESOURCE_DESC& desc) :
        mType(type), mFormat(static_cast<Format>(desc.Format)), mNumMips(static_cast<uint8_t>(desc.MipLevels)), mSampleCount(static_cast<uint8_t>(desc.SampleDesc.Count)),
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