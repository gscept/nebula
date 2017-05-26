//------------------------------------------------------------------------------
//  ogl4vertexlayoutserver.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4vertexlayoutserver.h"
#include "coregraphics/vertexlayout.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4VertexLayoutServer, 'D1VS', Base::VertexLayoutServerBase);
__ImplementSingleton(OpenGL4::OGL4VertexLayoutServer);

using namespace Util;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
OGL4VertexLayoutServer::OGL4VertexLayoutServer() :
	isOpen(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
OGL4VertexLayoutServer::~OGL4VertexLayoutServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4VertexLayoutServer::Open()
{
	n_assert(!this->isOpen);
	n_assert(this->vertexLayouts.IsEmpty());
	this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4VertexLayoutServer::Close()
{
	n_assert(this->isOpen);
	this->isOpen = false;
	IndexT i;
	for (i = 0; i < this->vertexLayouts.Size(); i++)
	{
		this->vertexLayouts[i]->Discard();
	}
	this->vertexLayouts.Clear();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<CoreGraphics::VertexLayout> 
OGL4VertexLayoutServer::CreateSharedVertexLayout( const Util::Array<CoreGraphics::VertexComponent>& vertexComponents )
{
	n_assert(this->IsOpen());
	n_assert(vertexComponents.Size() > 0);

	// create new instance
	Ptr<VertexLayout> newVertexLayout = VertexLayout::Create();
	newVertexLayout->Setup(vertexComponents);
	this->vertexLayouts.Append(newVertexLayout);
	return newVertexLayout;
}

} // namespace OpenGL4

