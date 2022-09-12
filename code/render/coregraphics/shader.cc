//------------------------------------------------------------------------------
//  shader.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shader.h"
#include "shaderserver.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
const ShaderId
ShaderGet(const Resources::ResourceName& name)
{
	return CoreGraphics::ShaderServer::Instance()->GetShader(name);
}

//------------------------------------------------------------------------------
/**
*/
const Util::String
ConstantTypeToString(const ShaderConstantType& type)
{
	switch (type)
	{
	case UnknownVariableType:			return "unknown";
	case IntVariableType:				return "int";
	case FloatVariableType:				return "float";
	case VectorVariableType:			return "vector";
	case Vector2VariableType:			return "vector2";
	case MatrixVariableType:			return "matrix";
	case BoolVariableType:				return "bool";
	case TextureVariableType:			return "texture";
	case SamplerVariableType:			return "sampler";
	case ConstantBufferVariableType:	return "constantbuffer";
	case ImageReadWriteVariableType:	return "image";
	case BufferReadWriteVariableType:	return "storagebuffer";
	default:
		n_error("ConstantTypeToString(): invalid type code!");
		return "";
	}
}

//------------------------------------------------------------------------------
/**
*/
const ShaderFeature::Mask
ShaderFeatureFromString(const Util::String& str)
{
	return ShaderServer::Instance()->FeatureStringToMask(str);
}

} // namespace CoreGraphics
