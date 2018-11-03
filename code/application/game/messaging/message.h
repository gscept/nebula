#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::Message

	Messages the main communications channel between components.

	Messages aren't exclusive to component however, any method or function can be
	used as a callback from a message which means you can hook into a message
	from anywhere. This can be useful for debugging, tools, managers etc.

	Messages are declared with the __DeclareMsg macro.
	A component can register a message callback by using the __RegisterMsg macro
	in any member method (preferably SetupAcceptedMessages).

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idgenerationpool.h"
#include "util/delegate.h"
#include "util/hashtable.h"
#include "util/arrayallocator.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "util/fourcc.h"

#define __DeclareMsg(NAME, FOURCC, ...) \
class NAME : public Game::Message<__VA_ARGS__> \
{ \
	NAME() \
	{ \
		this->name = #NAME; \
		this->fourcc = FOURCC; \
	}; \
	~NAME()\
	{ \
	}; \
};

///@note	This is placed within the object!
#define __RegisterMsg(MSGTYPE, METHOD) \
auto listener = MSGTYPE::Register( \
	MSGTYPE::Delegate::FromMethod< \
		std::remove_pointer<decltype(this)>::type, \
		&std::remove_pointer<decltype(this)>::type::METHOD \
	>(this) \
); \
this->messageListeners.Append(listener);

namespace Game
{

ID_32_TYPE(MessageListenerId)

//------------------------------------------------------------------------------
/**
*/
struct MessageListener
{
	Util::FourCC messageId;
	MessageListenerId listenerId;
};

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
class Message
{
public:
	Message();
	~Message();

	Message<TYPES...>(const Message<TYPES...>&) = delete;
	void operator=(const Message<TYPES...>&) = delete;

	/// Type definition for this message's delegate
	using Delegate = Util::Delegate<const TYPES& ...>;

	/// Register a listener to this message. Returns an ID for the listener so that we can associate it.
	static MessageListener Register(Delegate&& callback);

	/// Deregister a listener
	static void Deregister(MessageListener listener);

	/// Send a message to an entity
	static void Send(const TYPES& ... values);

	/// Send a network distributed message
	static void Distribute(const TYPES& ...);

	/// Deregisters all listeners at once.
	static void DeregisterAll();

	/// Returns whether a MessageListenerId is still registered or not.
	static bool IsValid(MessageListenerId listener);

private:
	/// Internal singleton instance
	static Message<TYPES...>* Instance()
	{
		static Message<TYPES...> instance;
		return &instance;
	}

	/// Registry between component and index in list.
	Util::HashTable<MessageListenerId, IndexT> listenerMap;

	/// contains the callback and the listener it's attached to.
	Util::ArrayAllocator<
		MessageListenerId,
		Delegate
	> callbacks;

#ifndef __PUBLIC_BUILD__
	/// Contains the sender of the distributed message.
	Util::Array<Util::String> sender;
#endif
	/// Contains the arguments of a recieved distributed message.
	/// @todo	This should be updated by some NetworkMessageManager-y object.
	Util::ArrayAllocator<
		TYPES ...
	> distributedMessages;

protected:
	Util::StringAtom name;
	Util::FourCC fourcc;
	Ids::IdGenerationPool pool;
};

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline
Message<TYPES...>::Message()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline
Message<TYPES...>::~Message()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline MessageListener
Message<TYPES...>::Register(Delegate&& callback)
{
	MessageListenerId l;
	Instance()->pool.Allocate(l.id);
	IndexT index = Instance()->callbacks.Alloc();
	Instance()->callbacks.Set(index, l, callback);
	Instance()->listenerMap.Add(l, index);
	MessageListener listener = { Instance()->fourcc, l };
	return listener;
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Message<TYPES...>::Deregister(MessageListener listener)
{
	auto instance = Instance();
	n_assert(listener.messageId == instance->fourcc);
	IndexT index = instance->listenerMap[listener.listenerId];
	if (index != InvalidIndex)
	{
		instance->listenerMap.Erase(listener.listenerId);
		instance->callbacks.EraseIndexSwap(index);
		if (instance->callbacks.Size() != 0)
		{
			instance->listenerMap[instance->callbacks.Get<0>(index)] = index;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Message<TYPES...>::Send(const TYPES& ... values)
{
	auto instance = Instance();
	SizeT size = instance->callbacks.Size();
	for (SizeT i = 0; i < size; ++i)
	{
		// n_assert(instance->callbacks.Get<1>(i).GetObject() != nullptr);
		instance->callbacks.Get<1>(i)(values...);
	}
}

//------------------------------------------------------------------------------
/**
	@todo	Implement networked messages.
*/
template <class ... TYPES>
inline void
Message<TYPES...>::Distribute(const TYPES& ...)
{
	n_error("Network distributed messages not yet supported!");
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Message<TYPES...>::DeregisterAll()
{
	auto instance = Instance();
	SizeT size = instance->callbacks.Size();
	for (SizeT i = 0; i < size; i++)
	{
		instance->pool.Deallocate(instance->callbacks.Get<0>(i).id);
	}
	instance->callbacks.Clear();
	instance->distributedMessages.Clear();
	instance->listenerMap.Clear();	
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline bool
Message<TYPES...>::IsValid(MessageListenerId listener)
{
	return Instance()->pool.IsValid(listener.id);
}

} // namespace Msg
