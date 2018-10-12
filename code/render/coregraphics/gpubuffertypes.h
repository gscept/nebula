#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::BufferSetup
	
	Types used for buffers, how they are created and their intended usage.

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
namespace GpuBufferTypes
{
	/// resource usage flags
	enum Usage
	{
		UsageImmutable,      //> can only be read by GPU, not written, cannot be accessed by CPU
		UsageDynamic,        //> can only be read by GPU, can only be written by CPU
		UsageCpu,            //> a resource which is only accessible by the CPU and can't be used for rendering
	};

	// cpu access flags
	enum Access
	{
		AccessNone			= 0,							// CPU does not require access to the resource (best)
		AccessWrite			= 1,							// CPU has write access
		AccessRead			= 2,							// CPU has read access
		AccessReadWrite		= AccessWrite + AccessRead,		// CPU has read/write access
	};

	// mapping types
	enum MapType
	{
		MapRead					= 1,						// gain read access, must be UsageDynamic and AccessRead
		MapWrite				= 2,						// gain write access, must be UsageDynamic and AccessWrite
		MapReadWrite			= MapRead + MapWrite,       // gain read/write access, must be UsageDynamic and AccessReadWrite
		MapWriteDiscard			= 4,						// gain write access, discard previous content, must be UsageDynamic and AccessWrite
		MapWriteNoOverwrite		= 8,						// gain write access, must be UsageDynamic and AccessWrite, see D3D10 docs for details
	};

	// streaming methods
	enum Syncing
	{
		SyncingFlush,				// CPU has to flush resource to GPU in order to sync it
		SyncingPersistent,			// buffer is persistently mapped, no need to unmap
		SyncingCoherent				// resource is coherent on GPU and CPU when mapped
	};

	struct SetupFlags
	{
		CoreGraphics::GpuBufferTypes::Usage usage;
		CoreGraphics::GpuBufferTypes::Access access;
		CoreGraphics::GpuBufferTypes::Syncing syncing;
	};
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------
