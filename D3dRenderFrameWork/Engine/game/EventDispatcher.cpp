#include "EventDispatcher.h"

IEventDispatcher::IEventDispatcher() :
    mEventQueue{}, mEventCount(0),
    mErMap(static_cast<int>(EventType::NUM_EVENT_TYPES)) { }

void IEventDispatcher::AddReceiver(EventType eventId, IReceiveEvent* receiver)
{
    mErMap.insert({eventId, receiver});
}

void IEventDispatcher::RemoveReceiver(EventType eventId, IReceiveEvent* receiver)
{
    auto&& range = mErMap.equal_range(eventId);
    for (auto&& it = range.first; it != range.second; ++it)
    {
        if (it->second == receiver)
        {
            mErMap.erase(it);
            break;
        }
    }
}

void IEventDispatcher::RemoveAllReceivers(EventType eventId)
{
    if (eventId == EventType::NONE)
        mErMap.clear();
    else
        mErMap.erase(eventId);
}