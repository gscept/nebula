//------------------------------------------------------------------------------
//  d3d11vertexlayoutserver.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11vertexlayoutserver.h"
#include "coregraphics/vertexlayout.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11VertexLayoutServer, 'D1VS', Base::VertexLayoutServerBase);
__ImplementSingleton(Direct3D11::D3D11VertexLayoutServer);

using namespace Util;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
D3D11VertexLayoutServer::D3D11VertexLayoutServer() :
	isOpen(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D11VertexLayoutServer::~D3D11VertexLayoutServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11VertexLayoutServer::Open()
{
	n_assert(!this->isOpen);
	n_assert(this->vertexLayouts.IsEmpty());
	this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11VertexLayoutServer::Close()
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
} // namespace Direct3D11
