#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::DebugInterface
    
    Interface object of the Debug subsystem. This just creates a DebugHandler
    which runs the DebugServer in its own thread.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "interface/interfacebase.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace Debug
{
class DebugInterface : public Interface::InterfaceBase
{
    __DeclareClass(DebugInterface);
    __DeclareInterfaceSingleton(DebugInterface);
public:
    /// constructor
    DebugInterface();
    /// destructor
    virtual ~DebugInterface();
    /// open the interface object
    virtual void Open();
};

} // namespace Debug
//------------------------------------------------------------------------------
    