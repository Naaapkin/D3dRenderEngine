#ifdef WIN32
#include "DescriptorHeap.h"
#include "D3dGraphicContext.h"
#include "../../common/PC/WException.h"

ID3D12Device* D3dGraphicContext::DeviceHandle() const
{
    return mDeviceHandle.Get();
}

IDXGIFactory4* D3dGraphicContext::FactoryHandle() const
{
    return mFactoryHandle.Get();
}

D3dGraphicContext::D3dGraphicContext()
{
#ifdef DEBUG | _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        // 启用 GPU 验证
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(true);
            // 设置更严格的验证
            debugController1->SetEnableSynchronizedCommandQueueValidation(true);
        }
    }
#endif
    ThrowIfFailed(
        CreateDXGIFactory1(IID_PPV_ARGS(&mFactoryHandle))
    );

    if (FAILED(D3D12CreateDevice(nullptr,   // 使用主显示器适配器
        D3D_FEATURE_LEVEL_11_0,                     // 最低支持DX11
        IID_PPV_ARGS(&mDeviceHandle))))
    {
        // 如果失败，尝试使用WARP适配器
        ComPtr<IDXGIAdapter> warpAdapter = nullptr;
        ThrowIfFailed(
            mFactoryHandle->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))
        );

        ThrowIfFailed(
            D3D12CreateDevice(warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&mDeviceHandle)
            )
        );
    }

#ifdef DEBUG | _DEBUG
    if (SUCCEEDED(mDeviceHandle->QueryInterface(IID_PPV_ARGS(&mInfoQueue))))
    {
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif
}

#ifdef DEBUG | _DEBUG
D3dGraphicContext::~D3dGraphicContext()
{
    
    // check for graphic memory leaks
    HMODULE dxgiDebugModule = GetModuleHandleA("Dxgidebug.dll");
    if (!dxgiDebugModule) dxgiDebugModule = LoadLibraryA("Dxgidebug.dll");
    if (dxgiDebugModule != nullptr) {
        // fail to load Dxgidebug.dll
        typedef HRESULT(WINAPI* DXGIGetDebugInterface_t)(REFIID, void**);
        DXGIGetDebugInterface_t DXGIGetDebugInterface = reinterpret_cast<DXGIGetDebugInterface_t>(
            GetProcAddress(dxgiDebugModule, "DXGIGetDebugInterface")
            );
        if (DXGIGetDebugInterface == nullptr) {
            // fail to get the address of func "DXGIGetDebugInterface"
            FreeLibrary(dxgiDebugModule);
            return;
        }
        IDXGIDebug* pDxgiDebug;
        HRESULT hr = DXGIGetDebugInterface(IID_PPV_ARGS(&pDxgiDebug));
        if (SUCCEEDED(hr)) {
            pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            pDxgiDebug->Release();
        }
    }
}
#else
D3dGraphicContext::~D3dGraphicContext() = default;
#endif

#endif
