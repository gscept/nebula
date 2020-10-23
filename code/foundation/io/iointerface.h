#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::Interface
    
    Implements the asynchronous interface to the IO subsystem. This will
    run a minimal Nebula runtime with an IO subsystem in an extra thread.
    Communication with the IO::Interface happens by sending messages to
    the Interface object. Messages are guaranteed to be handled sequentially 
    in FIFO order (there's exactly one handler thread which handles all 
    messages).
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "interface/interfacebase.h"
#include "io/console.h"
#include "io/ioserver.h"
#include "io/iointerfaceprotocol.h"

//------------------------------------------------------------------------------
namespace IO
{
class IoInterface : public Interface::InterfaceBase
{
    __DeclareClass(IoInterface);
    __DeclareInterfaceSingleton(IoInterface);
public:
    /// constructor
    IoInterface();
    /// destructor
    virtual ~IoInterface();
    /// open the interface object
    virtual void Open();
	/// close the interface object
	virtual void Close();
};

} // namespace IO
//------------------------------------------------------------------------------
