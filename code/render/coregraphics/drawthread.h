#pragma once
//------------------------------------------------------------------------------
/**
    A CoreGraphics thread meant to only record draw commands to command buffers

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "threading/thread.h"
#include "threading/event.h"
namespace CoreGraphics
{

class DrawThread : public Threading::Thread
{
public:

	/// constructor
	DrawThread();

	enum CommandType
	{
		BeginCommand,
		ResetCommand,
		EndCommand,
		GraphicsPipeline,
		ComputePipeline,
		InputAssemblyVertex,
		InputAssemblyIndex,
		Draw,
		Dispatch,
		BindDescriptors,
		PushRange,
		Viewport,
		ViewportArray,
		ScissorRect,
		ScissorRectArray,
		StencilRefs,
		StencilReadMask,
		StencilWriteMask,
		UpdateBuffer,
		SetEvent,					// sets event to flagged
		ResetEvent,					// resets event to unflagged
		WaitForEvent,
		Barrier,
		Sync,
		Timestamp,
		BeginQuery,
		EndQuery,
		BeginMarker,
		EndMarker,
		InsertMarker
	};

	struct Command
	{
		IndexT size;
		CommandType type;
	};

	struct SyncCommand
	{
		static const CommandType Type = Sync;
		Threading::Event* event;
	};

	struct CommandBuffer
	{
		byte* buffer;
		SizeT size;
		SizeT capacity;

		CommandBuffer()
			: buffer(nullptr)
			, size(0)
			, capacity(1024)
		{};

		/// append to buffer
		void Append(const byte* buf, SizeT numBytes)
		{
			// if not enough space, resize
			if (this->buffer == nullptr)
			{
				this->capacity = Math::n_max(numBytes, this->capacity);
				this->buffer = n_new_array(byte, this->capacity);
			}
			else if (this->size + numBytes > this->capacity)
			{
				// grow, but stop growing exponentially at 2^16 
				const SizeT grow = Math::n_max(numBytes, this->capacity >> 1);
				this->capacity += grow;
				byte* newBuf = n_new_array(byte, this->capacity);

				// copy over old contents
				memcpy(newBuf, this->buffer, this->size);
				n_delete_array(this->buffer);
				this->buffer = newBuf;
			}

			// write command to buffer
			memcpy(this->buffer + this->size, buf, numBytes);
			this->size += numBytes;
		}

		/// get size
		const SizeT Size() const
		{ 
			return this->size; 
		}

		/// clear buffer
		void Clear()
		{
			n_delete_array(this->buffer);
			this->buffer = nullptr;
			this->capacity = 0;
			this->size = 0;
		}

		/// reset buffer
		void Reset()
		{
			this->size = 0;
		}
	};

	/// called if thread needs a wakeup call before stopping
	void EmitWakeupSignal() override;

	/// push command to thread
	template<typename T> void Push(const T& command);
	/// flush commands
	void Flush();

	/// submit sync call
	void Signal(Threading::Event* event);
protected:
	Threading::CriticalSection lock;
	Threading::Event signalEvent;

	Util::Array<Command> commands;
	CommandBuffer commandBuffer;
	Threading::Event* event;
};

/// create new draw thread, define in implementation thread
extern DrawThread* CreateDrawThread();


//------------------------------------------------------------------------------
/**
*/
template<typename T>
inline void
DrawThread::Push(const T& command)
{
	// lock resources
	this->lock.Enter();

	// create entry
	Command entry;
	entry.size = sizeof(T);
	entry.type = T::Type;
	this->commands.Append(entry);

	// record memory of command
	this->commandBuffer.Append(reinterpret_cast<const byte*>(&command), sizeof(T));

	// release lock
	this->lock.Leave();
}

} // namespace CoreGraphics
