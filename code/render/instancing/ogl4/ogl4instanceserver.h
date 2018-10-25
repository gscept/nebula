#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::OGL4InstanceServer
    
    OpenGL4 implementation of the instance server.
    
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "instancing/base/instanceserverbase.h"
#include "coregraphics/shaderfeature.h"
namespace Instancing
{
class OGL4InstanceServer : public InstanceServerBase
{
	__DeclareSingleton(OGL4InstanceServer);
	__DeclareClass(OGL4InstanceServer);
public:
	/// constructor
	OGL4InstanceServer();
	/// destructor
	virtual ~OGL4InstanceServer();

	/// opens server
	bool Open();
	/// close server
	void Close();

	/// render
    void Render(IndexT frameIndex);

private:
	CoreGraphics::ShaderFeature::Mask instancingFeatureBits;
}; 
} // namespace Instancing
//------------------------------------------------------------------------------