#pragma once
//------------------------------------------------------------------------------
/**
    Implements a mesh loader from stream into Vulkan. Doubtful this is Vulkan specific...
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/meshresource.h"
#include "coregraphics/mesh.h"
namespace CoreGraphics
{

class MeshLoader : public Resources::ResourceLoader
{
    __DeclareClass(MeshLoader);
public:
    /// constructor
    MeshLoader();
    /// destructor
    virtual ~MeshLoader();

	struct StreamMeshLoadMetaData
	{
		bool copySource;
	};

private:
    
    /// perform load
    Resources::ResourceUnknownId LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;

    /// setup mesh from nvx3 file in memory
    void SetupMeshFromNvx(const Ptr<IO::Stream>& stream, const MeshResourceId entry);

};

} // namespace CoreGraphics
