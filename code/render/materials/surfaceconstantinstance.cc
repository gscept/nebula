//------------------------------------------------------------------------------
//  surfaceconstantinstance.cc
//  (C) 2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materials/surfaceconstantinstance.h"
#include "materials/surfaceconstant.h"

using namespace Util;
namespace Materials
{
__ImplementClass(Materials::SurfaceConstantInstance, 'SUCI', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
SurfaceConstantInstance::SurfaceConstantInstance() :
    constant(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SurfaceConstantInstance::~SurfaceConstantInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstantInstance::Setup(const Ptr<SurfaceConstant>& constant)
{
    this->constant = constant;

    IndexT i;
    for (i = 0; i < this->constant->variablesByShader.Size(); i++)
    {
		const Ptr<CoreGraphics::ShaderState>& shader = this->constant->variablesByShader.KeyAtIndex(i);
        const Ptr<CoreGraphics::ShaderVariable>& var = this->constant->variablesByShader.ValueAtIndex(i);
        this->variablesByShader.Add(shader, var->CreateInstance());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstantInstance::Discard()
{
    this->constant = NULL;
    IndexT i;
    for (i = 0; i < this->variablesByShader.Size(); i++)
    {
        this->variablesByShader.ValueAtIndex(i)->Discard();
    }
    this->variablesByShader.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstantInstance::SetValue(const Util::Variant& value)
{
    // basically go through and set the variable value for each instance
    IndexT i;
    for (i = 0; i < this->variablesByShader.Size(); i++)
    {
        const Ptr<CoreGraphics::ShaderVariableInstance>& var = this->variablesByShader.ValueAtIndex(i);
        var->SetValue(value);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstantInstance::SetTexture(const Ptr<CoreGraphics::Texture>& tex)
{
    this->SetValue(Util::Variant(tex));
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstantInstance::Apply(const Ptr<CoreGraphics::ShaderState>& shader)
{
    // hmm, maybe shader instances deserves the zero-indexed treatment?
    if (!this->variablesByShader.Contains(shader)) return;
    const Ptr<CoreGraphics::ShaderVariableInstance>& var = this->variablesByShader[shader];
    var->Apply();
}


} // namespace Materials