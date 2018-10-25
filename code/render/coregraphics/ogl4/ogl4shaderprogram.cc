//------------------------------------------------------------------------------
//  ogl4shaderprogram.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4shaderprogram.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShaderProgram, 'D1SV', Base::ShaderVariationBase);

using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderProgram::OGL4ShaderProgram() :
	program(0),
	usePatches(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderProgram::~OGL4ShaderProgram()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderProgram::Setup(AnyFX::EffectProgram* program)
{
	n_assert(0 == this->program);

	this->program = program;
	String mask = program->GetAnnotationString("Mask").c_str();
	String name = program->GetName().c_str();
	this->usePatches = program->SupportsTessellation();

	this->SetFeatureMask(ShaderServer::Instance()->FeatureStringToMask(mask));
	this->SetName(name);
	this->SetNumPasses(1);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShaderProgram::Apply()
{
	n_assert(this->program);
	this->program->Apply();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShaderProgram::Commit()
{
	n_assert(this->program);
	this->program->Commit();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShaderProgram::SetWireframe(bool b)
{
    if (b)    this->program->GetRenderState()->SetFillMode(AnyFX::EffectRenderState::Line);
    else      this->program->GetRenderState()->Reset();
}
} // namespace OpenGL4
