#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SkinPartitioner
    
    Partitions a skinned mesh into sub-groups
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "skinfragment.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class SkinPartitioner : public Core::RefCounted
{
    __DeclareClass(SkinPartitioner);

public:
    /// constructor
    SkinPartitioner();
    /// destructor
    virtual ~SkinPartitioner();

    /// splits a skin into fragments, and returns the fragment list
    void FragmentSkin(const ToolkitUtil::MeshBuilder& sourceMesh, ToolkitUtil::MeshBuilder& destinationMesh, Util::Array<Ptr<SkinFragment> >& fragmentList);
    /// joins fragments together if possible
    void Defragment(const Util::Array<Ptr<SkinFragment> >& fragmented, Util::Array<Ptr<SkinFragment> >& defragmented, MeshBuilder& mesh, IndexT& baseGroupId);
private:

    /// creates a new mesh based on the source mesh and the partitioning
    void UpdateMesh(const ToolkitUtil::MeshBuilder& sourceMesh, ToolkitUtil::MeshBuilder& destinationMesh, const Util::Array<Ptr<SkinFragment> >& fragmenList);
}; 

} // namespace ToolkitUtil
//------------------------------------------------------------------------------