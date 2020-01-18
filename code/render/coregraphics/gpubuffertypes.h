#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::BufferSetup
	
	Types used for buffers, how they are created and their intended usage.

	(C)2017-2020 Individual contributors, see AUTHORS file
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
		UsageImmutable,      //> read only on GPU, upload once on CPU, never read back
		UsageDynamic,        //> same as immutable, except supports multiple CPU uploads
		UsageCpu,            //> same as dynamic, but can also be read by CPU
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
		SyncingManual,				// CPU has to flush resource to GPU in order to sync it
		SyncingAutomatic				// resource is coherent on GPU and CPU when mapped
	};
	

	struct SetupFlags
	{
		CoreGraphics::GpuBufferTypes::Usage usage;
		CoreGraphics::GpuBufferTypes::Access access;
		CoreGraphics::GpuBufferTypes::Syncing syncing;
	};
};

__ImplementEnumBitOperators(GpuBufferTypes::Syncing)

} // namespace CoreGraphics
//------------------------------------------------------------------------------
