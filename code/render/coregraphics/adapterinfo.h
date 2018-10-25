#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::AdapterInfo
    
    Contains information about a given display adapter. This info can be
    used to identify a specific piece of hardware or driver version.
    Use DisplayDevice::GetAdapterInfo() to obtain information about
    existing display adapters.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/guid.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class AdapterInfo
{
public:
    /// constructor
    AdapterInfo();
    /// set driver name
    void SetDriverName(const Util::String& s);
    /// get human readable driver name
    const Util::String& GetDriverName() const;
    /// set description string
    void SetDescription(const Util::String& s);
    /// get human readable description
    const Util::String& GetDescription() const;
    /// set device name
    void SetDeviceName(const Util::String& s);
    /// get human readable device name
    const Util::String& GetDeviceName() const;
    /// set driver version low part
    void SetDriverVersionLowPart(uint v);
    /// get low part of driver version
    uint GetDriverVersionLowPart() const;
    /// set driver version high part
    void SetDriverVersionHighPart(uint v);
    /// get high part of driver version
    uint GetDriverVersionHighPart() const;
    /// set vendor id
    void SetVendorId(uint id);
    /// get vendor identifier
    uint GetVendorId() const;
    /// set device id
    void SetDeviceId(uint id);
    /// get device identifier
    uint GetDeviceId() const;
    /// set subsystem id
    void SetSubSystemId(uint id);
    /// get subsystem identifier
    uint GetSubSystemId() const;
    /// set hardware revision
    void SetRevision(uint r);
    /// get hardware revision identifier
    uint GetRevision() const;
    /// set driver/chipset pair guid
    void SetGuid(const Util::Guid& g);
    /// get guid for driver/chipset pair
    const Util::Guid& GetGuid() const;

private:
    Util::String driverName;
    Util::String description;
    Util::String deviceName;
    uint driverVersionLowPart;
    uint driverVersionHighPart;
    uint vendorId;
    uint deviceId;
    uint subSystemId;
    uint revision;
    Util::Guid guid;
};

//------------------------------------------------------------------------------
/**
*/
inline
AdapterInfo::AdapterInfo() :
    driverVersionLowPart(0),
    driverVersionHighPart(0),
    vendorId(0),
    deviceId(0),
    subSystemId(0),
    revision(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetDriverName(const Util::String& s)
{
    this->driverName = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AdapterInfo::GetDriverName() const
{
    return this->driverName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetDescription(const Util::String& s)
{
    this->description = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AdapterInfo::GetDescription() const
{
    return this->description;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetDeviceName(const Util::String& s)
{
    this->deviceName = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AdapterInfo::GetDeviceName() const
{
    return this->deviceName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetDriverVersionLowPart(uint v)
{
    this->driverVersionLowPart = v;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AdapterInfo::GetDriverVersionLowPart() const
{
    return this->driverVersionLowPart;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetDriverVersionHighPart(uint v)
{
    this->driverVersionHighPart = v;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AdapterInfo::GetDriverVersionHighPart() const
{
    return this->driverVersionHighPart;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetVendorId(uint id)
{
    this->vendorId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AdapterInfo::GetVendorId() const
{
    return this->vendorId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetDeviceId(uint id)
{
    this->deviceId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AdapterInfo::GetDeviceId() const
{
    return this->deviceId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetSubSystemId(uint id)
{
    this->subSystemId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AdapterInfo::GetSubSystemId() const
{
    return this->subSystemId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetRevision(uint r)
{
    this->revision = r;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
AdapterInfo::GetRevision() const
{
    return this->revision;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AdapterInfo::SetGuid(const Util::Guid& g)
{
    this->guid = g;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Guid&
AdapterInfo::GetGuid() const
{
    return this->guid;
}

} // namespace CoreGraphics
//------------------------------------------------------------------------------

