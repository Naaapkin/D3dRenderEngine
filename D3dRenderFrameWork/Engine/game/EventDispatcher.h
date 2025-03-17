#pragma once
#include "pch.h"

enum class EventType : uint8_t
{
	QUIT,

	NUM_EVENT_TYPES,
	NONE
};

struct IReceiveEvent
{
	virtual ~IReceiveEvent() = default;
	virtual void OnEvent(EventType eventId) = 0;
	
	DEFAULT_MOVE_CONSTRUCTOR(IReceiveEvent)
	DEFAULT_MOVE_OPERATOR(IReceiveEvent)
	DEFAULT_COPY_CONSTRUCTOR(IReceiveEvent)
	DEFAULT_COPY_OPERATOR(IReceiveEvent)
};

class IEventDispatcher
{
public:
	static IEventDispatcher* CreateDispatcher();
	
	IEventDispatcher();
	virtual ~IEventDispatcher() = default;
	virtual bool Dispatch() = 0;
	virtual void PostEvent(EventType eventId) = 0;
	virtual void AddReceiver(EventType eventId, IReceiveEvent* receiver);
	virtual void RemoveReceiver(EventType eventId, IReceiveEvent* receiver);
	virtual void RemoveAllReceivers(EventType eventId);

	DEFAULT_MOVE_CONSTRUCTOR(IEventDispatcher)
	DEFAULT_MOVE_OPERATOR(IEventDispatcher)
	DEFAULT_COPY_CONSTRUCTOR(IEventDispatcher)
	DEFAULT_COPY_OPERATOR(IEventDispatcher)

protected:
	constexpr static uint16_t MAX_EVENT_PER_TICK = 512;
	
	EventType mEventQueue[MAX_EVENT_PER_TICK];
	uint16_t mEventCount;
	std::unordered_multimap<EventType, IReceiveEvent*> mErMap;
};
