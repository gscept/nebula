#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::VertexLayoutBase
    
    Base class for VertexLayout. A VertexLayout object describes what 
    components a vertex has and offers some utility functions. 
    Vertex layout objects are generally shared and created through
    the VertexLayoutServer (this is more efficient on some platforms).
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/indexbuffer.h"

namespace CoreGraphics
{
class VertexLayoutServer;
class VertexBuffer;
}

//------------------------------------------------------------------------------
namespace Base
{
class VertexLayoutBase : public Resources::Resource
{
public:
	struct VertexLayoutBaseInfo;

	/// max number of vertex streams
	static const IndexT MaxNumVertexStreams = 2;

    /// constructor
    VertexLayoutBase();
    /// destructor
    virtual ~VertexLayoutBase();
    
    /// setup the vertex layout
    static void Setup(const Util::Array<CoreGraphics::VertexComponent>& c, VertexLayoutBaseInfo& baseInfo);
	/// discard the vertex layout object
	static void Discard();
    /// return true if valid has been setup
    bool IsValid() const;

	/// set the vertex buffer associated with the stream index
	void SetStreamBuffer(IndexT streamIndex, const Ptr<CoreGraphics::VertexBuffer>& vertexBuffer);

    /// get number of components
    SizeT GetNumComponents() const;
    /// get vertex component at index
    const CoreGraphics::VertexComponent& GetComponentAt(IndexT i) const;
    /// return true if vertex component exists
    bool HasComponent(CoreGraphics::VertexComponent::SemanticName semName, IndexT semIndex) const;
    /// get index of vertex component by semantics
    IndexT FindComponent(CoreGraphics::VertexComponent::SemanticName semName, IndexT semIndex) const;
    /// get the vertex stride in number of bytes
    SizeT GetVertexByteSize() const;
    /// get vertex components
    const Util::Array<CoreGraphics::VertexComponent>& GetVertexComponents() const;    
        
	/// calculate byte size from component list
	static SizeT CalculateByteSize(const Util::Array<CoreGraphics::VertexComponent>& c);
	/// get sharing signature for a set of vertex components
	static Util::String BuildSignature(const Util::Array<CoreGraphics::VertexComponent>& c);

	struct VertexLayoutBaseInfo
	{
		Util::Array<CoreGraphics::VertexComponent> components;
		SizeT vertexByteSize;
		SizeT numUsedStreams;
		bool usedStreams[MaxNumVertexStreams];
	};

protected:
    friend class VertexLayoutServerBase;

	Ptr<CoreGraphics::VertexBuffer> vertexStreams[2];
    Util::Array<CoreGraphics::VertexComponent> components;
    SizeT vertexByteSize;
	SizeT numUsedStreams;
	bool usedStreams[MaxNumVertexStreams];
};

//------------------------------------------------------------------------------
/**
*/
inline bool
VertexLayoutBase::IsValid() const
{
    return this->components.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexLayoutBase::GetNumComponents() const
{
    return this->components.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::VertexComponent&
VertexLayoutBase::GetComponentAt(IndexT i) const
{
    return this->components[i];
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VertexLayoutBase::HasComponent(CoreGraphics::VertexComponent::SemanticName semName, IndexT semIndex) const
{
    return (InvalidIndex != this->FindComponent(semName, semIndex));
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexLayoutBase::GetVertexByteSize() const
{
    return this->vertexByteSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<CoreGraphics::VertexComponent>& 
VertexLayoutBase::GetVertexComponents() const
{
    return this->components;
}

} // namespace Base
//------------------------------------------------------------------------------
