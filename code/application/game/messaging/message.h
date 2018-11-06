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
class NAME : public Game::Message<NAME, __VA_ARGS__> \
{ \
	public: \
		NAME() = delete; \
		~NAME() = delete; \
		\
		constexpr static const char* GetName() \
		{ \
			return #NAME; \
		}; \
		\
		constexpr static const uint GetFourCC() \
		{ \
			return FOURCC; \
		}; \
};

///@note	This is placed within the object!
#define __RegisterMsg(MSGTYPE, METHOD) \
this->messageListeners.Append(MSGTYPE::Register( \
	MSGTYPE::Delegate::FromMethod< \
		std::remove_pointer<decltype(this)>::type, \
		&std::remove_pointer<decltype(this)>::type::METHOD \
	>(this) \
));

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
template <class MSG, class ... TYPES>
class Message
{
public:
	ID_32_TYPE(MessageQueueId)

	Message();
	~Message();

	Message<MSG, TYPES...>(const Message<MSG, TYPES...>&) = delete;
	void operator=(const Message<MSG, TYPES...>&) = delete;

	/// Type definition for this message's delegate
	using Delegate = Util::Delegate<const TYPES& ...>;

	/// Register a listener to this message. Returns an ID for the listener so that we can associate it.
	static MessageListener Register(Delegate&& callback);

	/// Deregister a listener
	static void Deregister(MessageListener listener);

	/// Send a message to an entity
	static void Send(const TYPES& ... values);

	/// Creates a new message queue for deferred dispatching
	static MessageQueueId AllocateMessageQueue();

	/// Add a message to a message queue
	static void Defer(MessageQueueId qid, const TYPES& ... values);
	
	/// Dispatch all messages in a message queue
	static void DispatchMessageQueue(MessageQueueId id);

	/// Deallocate a message queue. All messages in the queue will be destroyed.
	static void DeAllocateMessageQueue(MessageQueueId id);

	/// Check whether a message queue is still valid.
	static bool IsMessageQueueValid(MessageQueueId id);

	/// Send a network distributed message
	static void Distribute(const TYPES& ...);

	/// Deregisters all listeners at once.
	static void DeregisterAll();

	/// Returns whether a MessageListenerId is still registered or not.
	static bool IsValid(MessageListenerId listener);

protected:
	friend MSG;
	/// Internal singleton instance
	static Message<MSG, TYPES...>* Instance()
	{
		static Message<MSG, TYPES...> instance;
		return &instance;
	}

	/// Registry between component and index in list.
	Util::HashTable<MessageListenerId, IndexT> listenerMap;

	/// contains the callback and the listener it's attached to.
	Util::ArrayAllocator<
		MessageListenerId,
		Delegate
	> callbacks;	

	/// id generation pool for the deferred messages queues.
	Ids::IdGenerationPool messageQueueIdPool;
	/// Deferred messages
	Util::Array<Util::ArrayAllocator<TYPES ...>> messageQueues;

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
	Ids::IdGenerationPool listenerPool;


private:
	template<std::size_t...Is>
	void send_expander(Util::ArrayAllocator<TYPES...>& data, const SizeT& index, std::index_sequence<Is...>)
	{
		Send(data.Get<Is>(index)...);
	}

	void send_expander(Util::ArrayAllocator<TYPES...>& data, const SizeT& index)
	{
		this->send_expander(data, index, std::make_index_sequence<sizeof...(TYPES)>());
	}
};

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline
Message<MSG, TYPES...>::Message()
{
	this->name = MSG::GetName();
	this->fourcc = MSG::GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline
Message<MSG, TYPES...>::~Message()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline MessageListener
Message<MSG, TYPES...>::Register(Delegate&& callback)
{
	MessageListenerId l;
	Instance()->listenerPool.Allocate(l.id);
	IndexT index = Instance()->callbacks.Alloc();
	Instance()->callbacks.Set(index, l, callback);
	Instance()->listenerMap.Add(l, index);
	MessageListener listener = { Instance()->fourcc, l };
	return listener;
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::Deregister(MessageListener listener)
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
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::Send(const TYPES& ... values)
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
*/
template<typename MSG, class ...TYPES>
inline typename Message<MSG, TYPES...>::MessageQueueId
Message<MSG, TYPES...>::AllocateMessageQueue()
{
	auto instance = Instance();
	MessageQueueId id;
	instance->messageQueueIdPool.Allocate(id.id);
	
	if (Ids::Index(id.id) >= instance->messageQueues.Size())
	{
		// This should garantuee that the id is the newest element
		n_assert(instance->messageQueues.Size() == Ids::Index(id.id));
		instance->messageQueues.Append({});
	}
	
	return id;
}

template<typename MSG, class ...TYPES>
inline void 
Message<MSG, TYPES...>::Defer(MessageQueueId qid, const TYPES & ...values)
{
	auto instance = Instance();
	SizeT index = Ids::Index(qid.id);

	auto i = instance->messageQueues[index].Alloc();
	instance->messageQueues[index].Set(i, values...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::DispatchMessageQueue(MessageQueueId id)
{
	auto instance = Instance();
	
	n_assert(instance->messageQueues.Size() > Ids::Index(id.id));
	n_assert(instance->messageQueueIdPool.IsValid(id.id));

	auto& data = instance->messageQueues[Ids::Index(id.id)];

	SizeT size = data.Size();
	for (SizeT i = 0; i < size; i++)
	{
		instance->send_expander(data, i);
	}

	// TODO:	Should we call reset here instead?
	data.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<typename MSG, class ...TYPES> inline void
Message<MSG, TYPES...>::DeAllocateMessageQueue(MessageQueueId id)
{
	auto instance = Instance();
	
	n_assert(instance->messageQueues.Size() > Ids::Index(id.id));
	n_assert(instance->messageQueueIdPool.IsValid(id.id));

	instance->messageQueues[Ids::Index(id.id)].Clear();
	instance->messageQueueIdPool.Deallocate(id.id);
}

template<typename MSG, class ...TYPES> inline bool
Message<MSG, TYPES...>::IsMessageQueueValid(MessageQueueId id)
{
	return Instance()->messageQueueIdPool.IsValid(id.id);
}



//------------------------------------------------------------------------------
/**
	@todo	Implement networked messages.
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::Distribute(const TYPES& ...)
{
	n_error("Network distributed messages not yet supported!");
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::DeregisterAll()
{
	auto instance = Instance();
	SizeT size = instance->callbacks.Size();
	for (SizeT i = 0; i < size; i++)
	{
		instance->listenerPool.Deallocate(instance->callbacks.Get<0>(i).id);
	}
	instance->callbacks.Clear();
	instance->distributedMessages.Clear();
	instance->listenerMap.Clear();	
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline bool
Message<MSG, TYPES...>::IsValid(MessageListenerId listener)
{
	return Instance()->listenerPool.IsValid(listener.id);
}

} // namespace Msg
