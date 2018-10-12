#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::D3D11InstanceServer
    
    Implements a DirectX 11 instance server.
    
    (C) 2012 Gustav Sterbrant
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "instancing/base/instanceserverbase.h"
#include "coregraphics/shaderfeature.h"

namespace Instancing
{
class D3D11InstanceServer : public InstanceServerBase
{
	__DeclareSingleton(D3D11InstanceServer);	
	__DeclareClass(D3D11InstanceServer);
public:
	/// constructor
	D3D11InstanceServer();
	/// destructor
	virtual ~D3D11InstanceServer();

	/// opens server
	bool Open();
	/// closes server
	void Close();

	/// render
	void Render();

private:
	CoreGraphics::ShaderFeature::Mask instancingFeatureBits;
}; 

} // namespace Instancing
//------------------------------------------------------------------------------