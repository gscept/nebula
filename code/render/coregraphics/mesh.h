#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::Mesh
  
    A mesh maintains a vertex buffer, an optional index buffer
    and a number of PrimitiveGroup objects. Meshes can be loaded directly 
    from a mesh resource file.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "resources/resource.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/primitivegroup.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class Mesh : public Resources::Resource
{
    __DeclareClass(Mesh);
public:
    /// constructor
	Mesh();
    /// destructor
    virtual ~Mesh();

    /// unload mesh resource
    virtual void Unload();
    
    /// return true if the mesh has a vertex buffer
    bool HasVertexBuffer() const;
    /// set the vertex buffer object
    void SetVertexBuffer(const Ptr<CoreGraphics::VertexBuffer>& vb);
    /// get the vertex buffer object
    const Ptr<CoreGraphics::VertexBuffer>& GetVertexBuffer() const;
    /// return true if the mesh has an index buffer
    bool HasIndexBuffer() const;
    /// set the index buffer object
    void SetIndexBuffer(const Ptr<CoreGraphics::IndexBuffer>& ib);
    /// get the index buffer object
    const Ptr<CoreGraphics::IndexBuffer>& GetIndexBuffer() const;
	/// set mesh topology
	void SetTopology(const CoreGraphics::PrimitiveTopology::Code& topo);
	/// get mesh topology
	const CoreGraphics::PrimitiveTopology::Code& GetTopology() const;
    /// set primitive groups
    void SetPrimitiveGroups(const Util::Array<CoreGraphics::PrimitiveGroup>& groups);
    /// get the number of primitive groups in the mesh
    SizeT GetNumPrimitiveGroups() const;
    /// get primitive group at index
    const CoreGraphics::PrimitiveGroup& GetPrimitiveGroupAtIndex(IndexT i) const;    

    /// apply mesh data for rendering in renderdevice
    void ApplyPrimitives(IndexT primGroupIndex);
	/// apply mesh resources and topology
	void ApplySharedMesh();
	/// apply primitive group
	void ApplyPrimitiveGroup(IndexT primGroupIndex);
 
protected:   
    Ptr<CoreGraphics::VertexBuffer> vertexBuffer;
    Ptr<CoreGraphics::IndexBuffer> indexBuffer;
	CoreGraphics::PrimitiveTopology::Code topology;
    Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Mesh::HasVertexBuffer() const
{
    return this->vertexBuffer.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Mesh::SetVertexBuffer(const Ptr<CoreGraphics::VertexBuffer>& vb)
{
    this->vertexBuffer = vb;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::VertexBuffer>&
Mesh::GetVertexBuffer() const
{
    return this->vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Mesh::HasIndexBuffer() const
{
    return this->indexBuffer.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Mesh::SetIndexBuffer(const Ptr<CoreGraphics::IndexBuffer>& ib)
{
    this->indexBuffer = ib;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::IndexBuffer>&
Mesh::GetIndexBuffer() const
{
    return this->indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Mesh::SetTopology(const CoreGraphics::PrimitiveTopology::Code& topo)
{
	this->topology = topo;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::PrimitiveTopology::Code&
Mesh::GetTopology() const
{
	return this->topology;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Mesh::SetPrimitiveGroups(const Util::Array<CoreGraphics::PrimitiveGroup>& groups)
{
    this->primitiveGroups = groups;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Mesh::GetNumPrimitiveGroups() const
{
    return this->primitiveGroups.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::PrimitiveGroup&
Mesh::GetPrimitiveGroupAtIndex(IndexT i) const
{
    return this->primitiveGroups[i];
}
    
} // namespace CoreGraphics
//------------------------------------------------------------------------------


