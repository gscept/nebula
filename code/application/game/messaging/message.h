#pragma once
//------------------------------------------------------------------------------
/**
	Message

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idgenerationpool.h"
#include "util/delegate.h"
#include "util/hashtable.h"
#include "util/arrayallocator.h"
#include "util/string.h"

namespace Game
{

ID_32_TYPE(MessageListenerId)

#define __DeclareMsg(MSGTYPE, ...) class MSGTYPE : public Game::Message<__VA_ARGS__> { MSGTYPE(){}; ~MSGTYPE(){};};

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
	static MessageListenerId Register(Delegate&& callback);

	/// Deregister a listener
	static void Deregister(MessageListenerId listener);

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
inline MessageListenerId
Message<TYPES...>::Register(Delegate&& callback)
{
	MessageListenerId l;
	Instance()->pool.Allocate(l.id);
	IndexT index = Instance()->callbacks.Alloc();
	Instance()->callbacks.Set(index, l, callback);
	Instance()->listenerMap.Add(l, index);
	return l;
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Message<TYPES...>::Deregister(MessageListenerId listener)
{
	auto instance = Instance();
	IndexT index = instance->listenerMap[listener];
	instance->listenerMap.Erase(listener);
	instance->callbacks.EraseIndexSwap(index);
	if (instance->callbacks.Size() != 0)
	{
		instance->listenerMap[instance->callbacks.Get<0>(index)] = index;
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
