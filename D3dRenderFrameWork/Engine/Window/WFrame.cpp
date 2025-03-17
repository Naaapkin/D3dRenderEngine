#include "WFrame.h"
#include "../../common/PC/WFunc.h"
#include "../../common/PC/WException.h"

IFrame* IFrame::CreateFrame(const String& title, uint16_t width, uint16_t height, bool isFullScreen)
{
    return new WFrame(title, width, height, isFullScreen);
}

LRESULT DefaultFrameProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WFrame* frame;
    switch (message)
    {
    case WM_NCHITTEST:
        {
            const int res = DefWindowProc(hwnd, message, wParam, lParam);
            // 禁用缩放
            if (res >= HTLEFT && res <= HTBOTTOMRIGHT) return HTBORDER;
            return res;
        }
    case WM_NCCREATE:
        frame = static_cast<WFrame*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        if (frame) SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(frame));
        return DefWindowProc(hwnd, message, wParam, lParam);
    case WM_DPICHANGED:
        if ((frame = reinterpret_cast<WFrame*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))) 
            frame->OnDpiChanged(HIWORD(wParam));
        return 0;
    case WM_CLOSE:
        if ((frame = reinterpret_cast<WFrame*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))))
            frame->IFrame::Close();
        else
            DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

WindowClass::WindowClass(const String& name, WNDPROC pWindowProc)
{
    this->mName = name;
    
    WNDCLASS wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hInstance = (HINSTANCE)::GetModuleHandle(nullptr);
    wc.lpfnWndProc = pWindowProc ? pWindowProc : DefaultFrameProc;;
    wc.lpszClassName = name.c_str();
    wc.lpszMenuName = nullptr;

    RegisterClass(&wc);
}

WindowClass::~WindowClass()
{
    UnregisterClass(mName.c_str(),
        (HINSTANCE)::GetModuleHandle(nullptr));
}

const String& WindowClass::GetName() const
{
    return mName;
}

HWND WFrame::WindowHandle() const
{
    return mWindowHandle;
}

float WFrame::DPIScalar() const
{
    return mDpi / 96.0f;
}

void WFrame::OnDpiChanged(WORD dpi)
{
    RECT clientRect;
    GetClientRect(mWindowHandle, &clientRect);
    float scalar = dpi / static_cast<float>(mDpi);
    SetWindowPos(mWindowHandle, nullptr,
        0, 0,       // ignored by using flag SWP_NOMOVE 
        (clientRect.right - clientRect.left) * scalar, (clientRect.bottom - clientRect.top) * scalar,   // new size
        SWP_NOMOVE | SWP_NOZORDER);
    mDpi = dpi;
}

void WFrame::GetClientSize(uint16_t& width, uint16_t& height) const
{
    RECT rect;
    GetClientRect(mWindowHandle, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

void WFrame::Close()
{
    IFrame::Close();
    SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, 0);
    DestroyWindow(mWindowHandle);
}

void WFrame::SetFrameSize(uint16_t width, uint16_t height)
{
    IFrame::SetFrameSize(width, height);
    SetWindowPos(mWindowHandle, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void WFrame::SetFullScreenMode(bool isFullScreen)
{
    if (isFullScreen == IsFullScreen()) return;
    IFrame::SetFullScreenMode(isFullScreen);
    if (!isFullScreen)
    {
        SetWindowLongPtr(mWindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos(mWindowHandle, HWND_NOTOPMOST, 0, 0, Width(), Height(), SWP_FRAMECHANGED);
        return;
    }
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(MonitorFromWindow(mWindowHandle, MONITOR_DEFAULTTOPRIMARY), &mi);
    SetWindowLongPtr(mWindowHandle, GWL_STYLE, WS_OVERLAPPED);
    SetWindowPos(mWindowHandle, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED);
}

WFrame::WFrame() :
    IFrame(),
    mWindowClassPtr(new WindowClass(DEFAULT_FRAME_TITLE, DefaultFrameProc))
{
    InitializeWinFrame();
}

WFrame::WFrame(const String& title, uint16_t width, uint16_t height, bool mode) :
    IFrame(title, width, height, mode),
    mWindowClassPtr(new WindowClass(title, DefaultFrameProc))
{
    InitializeWinFrame();
}

WFrame::WFrame(WFrame&& other) noexcept :
    IFrame(std::move(other)),
    mWindowClassPtr(other.mWindowClassPtr),
    mWindowHandle(other.mWindowHandle),
    mDpi(other.mDpi)
{
    other.mWindowClassPtr = nullptr;
    SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

WFrame::~WFrame()
{
    
    if (!mWindowHandle) return;
    if (!IsClosed()) WFrame::Close();
    delete mWindowClassPtr;
}

WFrame& WFrame::operator=(WFrame&& other) noexcept
{
    if (this != &other)
    {
        IFrame::operator=(std::move(other));
        mWindowClassPtr = other.mWindowClassPtr;
        mWindowHandle = other.mWindowHandle;
        mDpi = other.mDpi;
        other.mWindowClassPtr = nullptr;
        SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
    return *this;
}

void WFrame::InitializeWinFrame()
{
    SetProcessDPIAware();
    mWindowHandle = CreateWindow(mWindowClassPtr->GetName().c_str(), Title().c_str(),
        IsFullScreen() ? WS_OVERLAPPED : WS_OVERLAPPEDWINDOW,
        0, 0, Width(), Height(),
        nullptr, nullptr, (HINSTANCE)::GetModuleHandle(nullptr), this);
    if (!mWindowHandle)
    {
        const wchar_t* errMsg = WFunc::GetHRInfo(GetLastError());
        THROW_EXCEPTION(errMsg);
    }
    mDpi = GetDpiForWindow(mWindowHandle);

    ::ShowWindow(mWindowHandle, SW_SHOWDEFAULT);
    ::UpdateWindow(mWindowHandle);
}
