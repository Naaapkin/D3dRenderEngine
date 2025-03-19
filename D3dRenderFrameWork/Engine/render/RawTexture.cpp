#ifdef WIN32
#include <Engine/render/RawTexture.h>
#include <Engine/common/Exception.h>

const byte* RawTexture::dataPtr() const
{
    return mData;
}

const byte* RawTexture::subDatePtr(uint8_t mip) const
{
    return mData + GetMip(mip);
}

void RawTexture::SetData(const byte* data) const
{
    uint64_t size = GetMip(MipLevels());
    memcpy(mData, data, size);
}

void RawTexture::SetSubData(uint8_t mip, const byte* subData) const
{
#ifdef DEBUG || _DEBUG
    ASSERT(mip < MipLevels(), TEXT("index out of bound"));
#endif
    uint64_t index = GetMip(MipLevels());
    memcpy(mData + index, subData, GetMipSize(mip));
}

RawTexture::RawTexture(TextureType type,
    uint64_t width, uint64_t height, uint32_t depth, TextureFormat format,
    const byte* data, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality) :
    Texture(type, width, height, depth, format, numMips, sampleCount, sampleQuality) 
{
    uint64_t size = GetMip(numMips);
    mData = new byte[size];
    memcpy(mData, data, height * width * depth);
}

RawTexture::RawTexture(const RawTexture& o) noexcept: Texture(o)
{
    uint64_t size = GetMip(MipLevels());
    mData = new uint8_t[size];
    memcpy(mData, o.mData, size);
}

RawTexture::~RawTexture()
{
    delete[] mData;
}

RawTexture& RawTexture::operator=(const RawTexture& o) noexcept
{
    if (this != &o)
    {
        Texture::operator=(o);
        uint64_t size = GetMip(MipLevels());
        mData = new uint8_t[size];
        memcpy(mData, o.mData, size);
    }
    return *this;
}


uint64_t RawTexture::GetMip(uint8_t mip) const
{
    uint64_t index = 0;
    uint64_t width = Width();
    uint64_t height = Height();
    uint64_t depth = Depth();
    while (mip)
    {
        index += width * height * depth;
        width = width > 2 ? width >> 1 : width;
        height = height > 2 ? height >> 1 : height;
        depth = depth >= 2 ? depth >> 1 : depth;
        mip--;
    }
    return index;
}

uint64_t RawTexture::GetMipSize(uint8_t mip) const
{
    uint64_t width = Width() << mip;
    uint64_t height = Height() << mip;
    uint64_t depth = Depth() << mip;
    width = width > 0 ? width : 1;
    height = height > 0 ? height : 1;
    depth = depth > 0 ? depth : 1;
    return width * height * depth;
}
#endif