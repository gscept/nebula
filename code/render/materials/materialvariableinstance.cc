//------------------------------------------------------------------------------
//  materialvariableinstance.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materials/materialvariableinstance.h"

namespace Materials
{
__ImplementClass(Materials::MaterialVariableInstance, 'MTVI', Core::RefCounted);

using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
MaterialVariableInstance::MaterialVariableInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MaterialVariableInstance::~MaterialVariableInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void MaterialVariableInstance::Setup( const Ptr<MaterialVariable>& var )
{
	this->Prepare(var->GetType());
	this->Bind(var);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialVariableInstance::Cleanup()
{
	// release value, we might have a texture here!
	this->value.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void MaterialVariableInstance::Discard()
{
	n_assert(this->IsValid());
	this->materialVariable->DiscardVariableInstance(this);
	this->materialVariable = 0;
}

//------------------------------------------------------------------------------
/**
*/
void MaterialVariableInstance::Prepare( MaterialVariable::Type type )
{
	n_assert(!this->materialVariable.isvalid());
	switch (type)
	{
	case MaterialVariable::IntType:
		this->value.SetType(Variant::Int);
		break;

	case MaterialVariable::FloatType:
		this->value.SetType(Variant::Float);
		break;

	case MaterialVariable::VectorType:
		this->value.SetType(Variant::Float4);
		break;

	case MaterialVariable::MatrixType:
		this->value.SetType(Variant::Matrix44);
		break;

	case MaterialVariable::BoolType:
		this->value.SetType(Variant::Bool);
		break;

	case MaterialVariable::TextureType:
		this->value.SetType(Variant::Object);
		break;

	default:
		n_error("MaterialVariableInstance::Prepare(): Invalid ShaderVariable::Type in switch!\n");
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void MaterialVariableInstance::Bind( const Ptr<MaterialVariable>& var )
{
	n_assert(!this->materialVariable.isvalid());
	this->materialVariable = var;
}

//------------------------------------------------------------------------------
/**
    @todo: hmm, the dynamic type switch is sort of lame...
*/
void
MaterialVariableInstance::Apply()
{
    n_assert(this->materialVariable.isvalid());
    switch (this->value.GetType())
    {
        case Variant::Int:
            this->materialVariable->SetInt(this->value.GetInt());
            break;
        case Variant::Float:
            this->materialVariable->SetFloat(this->value.GetFloat());
            break;
		case Variant::Float2:
			this->materialVariable->SetFloat2(this->value.GetFloat2());
			break;
        case Variant::Float4:
            this->materialVariable->SetFloat4(this->value.GetFloat4());
            break;
        case Variant::Matrix44:
            this->materialVariable->SetMatrix(this->value.GetMatrix44());
            break;
        case Variant::Bool:
            this->materialVariable->SetBool(this->value.GetBool());
            break;
        case Variant::Object:
            // @note: implicit Ptr<> creation!
            if (this->value.GetObject() != 0)
            {
                this->materialVariable->SetTexture((Texture*)this->value.GetObject());
            }
            break;
        default:
            n_error("MaterialVariableInstance::Apply(): invalid data type for scalar!");
            break;
    }

	// apply actual settings
	this->materialVariable->Apply();
}


} // namespace Materials