#ifdef WIN32
#include "EventDispatcherWin.h"

IEventDispatcher* IEventDispatcher::CreateDispatcher()
{
    return new EventDispatcherWin();
}

bool EventDispatcherWin::Dispatch()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    auto size = (std::min)(mEventCount, MAX_EVENT_PER_TICK);
    for (int i = 0; i < size;)
    {
        const EventType& e = mEventQueue[i++];
        auto&& pair = mErMap.equal_range(e);
        for (auto& it = pair.first; it != pair.second; ++it)
        {
            it->second->OnEvent(e);
        }
    }
    return true;
}

void EventDispatcherWin::PostEvent(EventType eventId)
{
    // TODO: 
}
#endif
