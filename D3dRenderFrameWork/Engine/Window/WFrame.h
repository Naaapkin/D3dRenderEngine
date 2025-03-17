#pragma once
#ifdef WIN32
#include "Frame.h"

LRESULT DefaultFrameProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

struct WindowClass final
{
public:
    WindowClass(const String& name,
        ::WNDPROC pWindowProc = ::DefWindowProc);
    ~WindowClass();

    const String& GetName() const;

    DELETE_COPY_CONSTRUCTOR(WindowClass);
    DELETE_COPY_OPERATOR(WindowClass);
    DEFAULT_MOVE_CONSTRUCTOR(WindowClass);
    DEFAULT_MOVE_OPERATOR(WindowClass);
    
private:
    String mName;
};

class WFrame : public IFrame
{
public:
    HWND WindowHandle() const;
    float DPIScalar() const;
    void OnDpiChanged(WORD dpi);
    
    void GetClientSize(uint16_t& width, uint16_t& height) const override;
    void SetFrameSize(uint16_t width, uint16_t height) override;
    void SetFullScreenMode(bool isFullScreen) override;
    void Close() override;

    WFrame();
    WFrame(WFrame&& other) noexcept;
    WFrame(const String& title, uint16_t width, uint16_t height, bool mode);
    ~WFrame() override;
    
    DELETE_COPY_CONSTRUCTOR(WFrame);
    DELETE_COPY_OPERATOR(WFrame);

    WFrame& operator=(WFrame&& other) noexcept;

private:
    void InitializeWinFrame();

    WindowClass* mWindowClassPtr;
    HWND mWindowHandle;
    uint16_t mDpi;
};
#endif