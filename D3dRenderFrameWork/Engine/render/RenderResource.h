#pragma once
#include "Engine/pch.h"
#include "RHIDefination.h"

template <typename TRHIObject, typename = std::enable_if_t<std::is_base_of_v<RHIObject, TRHIObject>>>
class RHIRef
{
	friend class Renderer;
public:
	RHIRef() : mIndex(MAXUINT64), mObject(nullptr) { }
	RHIRef(uint64_t index, TRHIObject* object)
		: mIndex(index),
		mObject(object)
	{
	}

	TRHIObject* operator->() const
	{
		return mObject;
	}
private:
	uint64_t mIndex;
	TRHIObject* mObject;
};

using ConstantBufferRef = RHIRef<RHIConstantBuffer>;
using TextureRef = RHIRef<RHINativeTexture>;
using BufferRef = RHIRef<RHINativeBuffer>;
using RenderTargetRef = RHIRef<RHIRenderTarget>;
using DepthStencilRef = RHIRef<RHIDepthStencil>;
using VertexBufferRef = RHIRef<RHIVertexBuffer>;
using IndexBufferRef = RHIRef<RHIIndexBuffer>;