#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4VertexLayoutServer
    
    Implements a layout server for handling vertex layouts with DX11
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#include "coregraphics/vertexcomponent.h"
#include "util/stringatom.h"
#include "coregraphics/base/vertexlayoutserverbase.h"

//------------------------------------------------------------------------------

namespace CoreGraphics
{    
	class VertexLayout;
}

namespace OpenGL4
{
class OGL4VertexLayoutServer : public Base::VertexLayoutServerBase
{
	__DeclareClass(OGL4VertexLayoutServer);
	__DeclareSingleton(OGL4VertexLayoutServer);
public:
	/// constructor
	OGL4VertexLayoutServer();
	/// destructor
	virtual ~OGL4VertexLayoutServer();

	/// opens the server
	void Open();
	/// closes the server
	void Close();
	/// returns true if server is open
	bool IsOpen();
	
	/// create vertex layout
	Ptr<CoreGraphics::VertexLayout> CreateSharedVertexLayout(const Util::Array<CoreGraphics::VertexComponent>& vertexComponents);

protected:

	bool isOpen;
	Util::Array<Ptr<CoreGraphics::VertexLayout> > vertexLayouts;
}; 


//------------------------------------------------------------------------------
/**
*/
inline bool 
OGL4VertexLayoutServer::IsOpen()
{
	return this->isOpen;
}
} // namespace OpenGL4
//------------------------------------------------------------------------------