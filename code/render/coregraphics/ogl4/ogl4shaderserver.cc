//------------------------------------------------------------------------------
//  OGL4ShaderServer.cc
//  (C) 2013 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/streamshaderloader.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/ogl4/ogl4shaderserver.h"
#include "materials/materialserver.h"
#include "framesync/framesynctimer.h"
#include "afxapi.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShaderServer, 'D1SS', Base::ShaderServerBase);
__ImplementSingleton(OpenGL4::OGL4ShaderServer);

using namespace Resources;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderServer::OGL4ShaderServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderServer::~OGL4ShaderServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
OGL4ShaderServer::Open()
{
    n_assert(!this->IsOpen());

	// create anyfx factory
	this->factory = n_new(AnyFX::EffectFactory);
    ShaderServerBase::Open();

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderServer::Close()
{
    n_assert(this->IsOpen());
	n_delete(this->factory);
    ShaderServerBase::Close();
}


} // namespace OpenGL4

