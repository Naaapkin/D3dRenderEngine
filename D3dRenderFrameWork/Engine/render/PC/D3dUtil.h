#pragma once
#ifdef WIN32
#include <Engine/pch.h>

DXGI_FORMAT GetParaFormatFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc);
ID3DBlob* LoadCompiledShaderObject(const String& path);
bool CanImplicitTransit(const uint32_t& stateBefore, uint32_t& stateAfter, bool isBufferOrSimultaneous);
#endif
