#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::PhysicsStreamMeshLoader
    
    Setup a physics mesh object from a stream. Supports the following file formats:
        
    - nvx3 (Nebula binary mesh file format)
    
    
    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/streamresourceloader.h"
#include "coregraphics/base/resourcebase.h"

namespace Physics
{
class StreamPhysicsMeshLoader : public Resources::StreamResourceLoader
{
    __DeclareClass(StreamPhysicsMeshLoader);
public:
    /// constructor
    StreamPhysicsMeshLoader();    

	virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
private:
    
   
protected:

};

} // namespace Physics
//------------------------------------------------------------------------------
