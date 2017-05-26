#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ResourceBase
    
    Base class for all CoreGraphics resource classes.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"

//------------------------------------------------------------------------------
namespace Base
{
class ResourceBase : public Resources::Resource
{
    __DeclareClass(ResourceBase);
public:
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

    /// constructor
    ResourceBase();
    /// destructor
    virtual ~ResourceBase();

    /// set resource usage type
    void SetUsage(Usage usage);
    /// get resource usage type
    Usage GetUsage() const;
    /// set resource cpu access type
    void SetAccess(Access access);
    /// get cpu access type
    Access GetAccess() const;

	/// set streaming method
	void SetSyncing(Syncing method);
	/// get streaming method
	Syncing GetSyncing() const;

protected:
    Usage usage;
    Access access;
	Syncing syncing;

};

//------------------------------------------------------------------------------
/**
*/
inline void
ResourceBase::SetUsage(Usage u)
{
    this->usage = u;
}

//------------------------------------------------------------------------------
/**
*/
inline ResourceBase::Usage
ResourceBase::GetUsage() const
{
    return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ResourceBase::SetAccess(Access a)
{
    this->access = a;
}

//------------------------------------------------------------------------------
/**
*/
inline ResourceBase::Access
ResourceBase::GetAccess() const
{
    return this->access;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ResourceBase::SetSyncing(Syncing s)
{
	this->syncing = s;
}

//------------------------------------------------------------------------------
/**
*/
inline ResourceBase::Syncing
ResourceBase::GetSyncing() const
{
	return this->syncing;
}

} // namespace Base
//------------------------------------------------------------------------------
