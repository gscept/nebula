#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11VertexLayoutServer
    
    Implements a layout server for handling vertex layouts with DX11
    
    (C) 2012 gscept
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "coregraphics/vertexcomponent.h"
#include "util/stringatom.h"
#include "coregraphics/base/vertexlayoutserverbase.h"

namespace CoreGraphics
{    
	class VertexLayout;
}

namespace Direct3D11
{
class D3D11VertexLayoutServer : public Base::VertexLayoutServerBase
{
	__DeclareClass(D3D11VertexLayoutServer);
	__DeclareSingleton(D3D11VertexLayoutServer);
public:
	/// constructor
	D3D11VertexLayoutServer();
	/// destructor
	virtual ~D3D11VertexLayoutServer();

	/// opens the server
	void Open();
	/// closes the server
	void Close();
	/// returns true if server is open
	bool IsOpen();
	
protected:

	bool isOpen;
}; 


//------------------------------------------------------------------------------
/**
*/
inline bool 
D3D11VertexLayoutServer::IsOpen()
{
	return this->isOpen;
}
} // namespace Direct3D11
//------------------------------------------------------------------------------