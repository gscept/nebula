#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::FModAudioApplication
    
    Simple audio test application.
    
    (C) 2008 Radon Labs GmbH
*/


#include "util/array.h"
#include "core/ptr.h"
#include "input/inputserver.h"
#include "timing/timer.h"
#include "core/coreserver.h"
#include "io/gamecontentserver.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"


//------------------------------------------------------------------------------
namespace Test
{
class ScriptTestApplication
{
public:
    /// constructor
    ScriptTestApplication(int argc, char ** argv);
    /// destructor
    virtual ~ScriptTestApplication();
    /// open the application
    virtual bool Open();
    /// close the application
    virtual void Close();
    /// close the application
    virtual void Run();

private:
    

private:
    Ptr<Core::CoreServer> coreServer;
    int argc;
    char**argv;
    Timing::Timer masterTime;

};

} // namespace Test
//------------------------------------------------------------------------------
