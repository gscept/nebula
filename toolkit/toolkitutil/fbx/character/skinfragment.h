#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SkinFragment
    
    Contains information about a skin fragment
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "toolkitutil/meshutil/meshbuilder.h"

#define MAXJOINTCOUNT 1024
#define MAXJOINTSPERFRAGMENT 255

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class SkinFragment : public Core::RefCounted
{
    __DeclareClass(SkinFragment);
public:

    typedef uint JointIndex;
    typedef uint TriangleIndex;
    /// constructor
    SkinFragment();
    /// destructor
    virtual ~SkinFragment();

    /// adds a triangle and its corresponding joint indices (returns true if triangle was unique)
    bool AddTriangle(TriangleIndex triangleIndex);
    /// sets the mesh source
    void SetMesh(const ToolkitUtil::MeshBuilder& mesh);

    /// sets the group id
    void SetGroupId(int id);
    /// gets the group id
    int GetGroupId();
    /// gets the triangles list
    const Util::Array<TriangleIndex>& GetTriangles() const;
    /// adds triangle
    void InsertTriangle(const TriangleIndex& triangle);
    /// gets the joint palette
    const Util::Array<JointIndex>& GetJointPalette() const;
    /// adds joint
    void InsertJoint(const JointIndex& joint);

    /// converts a global joint index to a local one
    JointIndex GetLocalJointIndex(JointIndex globalIndex);

private:
    /// gets a joint palette for the given triangle
    void ExtractJointPalette(const ToolkitUtil::MeshBuilder& sourceMesh, const ToolkitUtil::MeshBuilderTriangle& triangle, Util::Array<JointIndex>& jointIndices);
    /// gets the difference between the known joints and the ones in the known array
    void GetUniqueJointList(const Util::Array<JointIndex>& jointIndices, Util::Array<JointIndex>& uniqueList);

    ToolkitUtil::MeshBuilder mesh;
    bool jointMask[1024];
    int id;
    Util::Array<JointIndex> jointPalette;
    Util::Array<TriangleIndex> triangles;
}; 


//------------------------------------------------------------------------------
/**
*/
inline void 
SkinFragment::SetMesh( const ToolkitUtil::MeshBuilder& mesh )
{
    this->mesh = mesh;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
SkinFragment::SetGroupId( int id )
{
    this->id = id;
}

//------------------------------------------------------------------------------
/**
*/
inline int 
SkinFragment::GetGroupId()
{
    return this->id;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<SkinFragment::TriangleIndex>& 
SkinFragment::GetTriangles() const
{
    return this->triangles;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
SkinFragment::InsertTriangle( const TriangleIndex& triangle )
{
    this->triangles.Append(triangle);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<SkinFragment::JointIndex>& 
SkinFragment::GetJointPalette() const
{
    return this->jointPalette;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SkinFragment::InsertJoint( const JointIndex& joint )
{
    this->jointPalette.Append(joint);
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------