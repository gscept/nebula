//------------------------------------------------------------------------------
//  vertexlayoutbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/vertexlayoutbase.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/vertexbuffer.h"

namespace Base
{
__ImplementClass(Base::VertexLayoutBase, 'VXLB', Core::RefCounted);

using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
VertexLayoutBase::VertexLayoutBase() :
    vertexByteSize(0),
	numUsedStreams(0)
{
	memset(this->usedStreams, 0, sizeof(this->usedStreams));
}

//------------------------------------------------------------------------------
/**
*/
VertexLayoutBase::~VertexLayoutBase()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
String
VertexLayoutBase::BuildSignature(const Array<VertexComponent>& comps)
{
    n_assert(comps.Size() > 0);
    String sig;
    IndexT i;
    for (i = 0; i < comps.Size(); i++)
    {
        sig.Append(comps[i].GetSignature());
    }
    return sig;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
VertexLayoutBase::CalculateByteSize(const Util::Array<CoreGraphics::VertexComponent>& c)
{
	SizeT ret = 0;
	IndexT i;
	for (i = 0; i < c.Size(); i++)
	{
		ret += c[i].GetByteSize();
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexLayoutBase::Setup(const Array<VertexComponent>& comps)
{
    n_assert(comps.Size() > 0);
    n_assert(!this->IsValid());
    n_assert(0 == this->vertexByteSize);
    this->components = comps;
    IndexT i;
    for (i = 0; i < comps.Size(); i++)
    {
        // update the components byte offset while we're at it
        this->components[i].SetByteOffset(this->vertexByteSize);
        this->vertexByteSize += comps[i].GetByteSize();
		this->usedStreams[this->components[i].streamIndex] = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VertexLayoutBase::Discard()
{
    n_assert(this->IsValid());
    this->components.Clear();
    this->vertexByteSize = 0;
	IndexT i;
	for (i = 0; i < VertexLayoutBase::MaxNumVertexStreams; i++)
	{
		this->vertexStreams[i] = NULL;
	}
}

//------------------------------------------------------------------------------
/**
*/
IndexT
VertexLayoutBase::FindComponent(VertexComponent::SemanticName semName, IndexT semIndex) const
{
    IndexT i;
    for (i = 0; i < this->components.Size(); i++)
    {
        const VertexComponent& cur = this->components[i];
        if ((cur.GetSemanticName() == semName) && (cur.GetSemanticIndex() == semIndex))
        {
            return i;
        }
    }
    // fallthrough: not found
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexLayoutBase::SetStreamBuffer(IndexT streamIndex, const Ptr<CoreGraphics::VertexBuffer>& vertexBuffer)
{
	this->vertexStreams[streamIndex] = vertexBuffer;
}

} // namespace Base
