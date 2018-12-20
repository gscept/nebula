//------------------------------------------------------------------------------
//  VertexLayoutServerBase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/vertexlayoutserverbase.h"
#include "coregraphics/vertexlayout.h"

namespace Base
{
__ImplementClass(Base::VertexLayoutServerBase, 'VSVB', Core::RefCounted);
__ImplementSingleton(Base::VertexLayoutServerBase);

using namespace Util;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
VertexLayoutServerBase::VertexLayoutServerBase() :
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VertexLayoutServerBase::~VertexLayoutServerBase()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexLayoutServerBase::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->vertexLayouts.IsEmpty());
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexLayoutServerBase::Close()
{
    n_assert(this->isOpen);
    this->isOpen = false;
    IndexT i;
    for (i = 0; i < this->vertexLayouts.Size(); i++)
    {
        this->vertexLayouts.ValueAtIndex(i)->Discard();
    }
    this->vertexLayouts.Clear();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<VertexLayout>
VertexLayoutServerBase::CreateSharedVertexLayout(const Util::Array<VertexComponent>& vertexComponents)
{
    n_assert(this->IsOpen());
    n_assert(vertexComponents.Size() > 0);

    // get sharing signature from vertex components
    StringAtom signature = VertexLayout::BuildSignature(vertexComponents);
    if (this->vertexLayouts.Contains(signature))
    {
        // return existing instance
        return this->vertexLayouts[signature];
    }
    else
    {
        // create new instance
        Ptr<VertexLayout> newVertexLayout = VertexLayout::Create();
        newVertexLayout->Setup(vertexComponents);
        this->vertexLayouts.Add(signature, newVertexLayout);
        return newVertexLayout;
    }
}

//------------------------------------------------------------------------------
/**
*/
SizeT
VertexLayoutServerBase::CalculateVertexSize(const Util::Array<CoreGraphics::VertexComponent>& vertexComponents)
{
	n_assert(this->IsOpen());
	n_assert(vertexComponents.Size() > 0);

	SizeT retval = 0;

	IndexT i;
	for (i = 0; i < vertexComponents.Size(); i++)
	{
		retval += vertexComponents[i].GetByteSize();
	}
	return retval;
}

} // namespace Base
