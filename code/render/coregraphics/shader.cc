//------------------------------------------------------------------------------
//  shader.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shader.h"
#include "shaderpool.h"
#include "shaderserver.h"
namespace CoreGraphics
{

ShaderPool* shaderPool;
//------------------------------------------------------------------------------
/**
*/
const ShaderId
CreateShader(const ShaderCreateInfo& info)
{
	ShaderId ret = shaderPool->CreateResource(info.name, "render_system", nullptr, nullptr, true);
	ret.allocType = ShaderIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyShader(const ShaderId id)
{
	shaderPool->DiscardResource(id);
}

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
const ResourceTableId
ShaderCreateResourceTable(const ShaderId id, const IndexT group)
{
	return shaderPool->CreateResourceTable(id, group);
}

//------------------------------------------------------------------------------
/**
*/
const ConstantBufferId 
ShaderCreateConstantBuffer(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->CreateConstantBuffer(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const ConstantBufferId 
ShaderCreateConstantBuffer(const ShaderId id, const IndexT cbIndex)
{
	return shaderPool->CreateConstantBuffer(id, cbIndex);
}

//------------------------------------------------------------------------------
/**
*/
const ConstantBinding 
ShaderGetConstantBinding(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetConstantBinding(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const ConstantBinding 
ShaderGetConstantBinding(const ShaderId id, const IndexT cIndex)
{
	return shaderPool->GetConstantBinding(id, cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom 
ShaderGetConstantBlockName(const ShaderId id, const Util::StringAtom & name)
{
	return shaderPool->GetConstantBlockName(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom 
ShaderGetConstantBlockName(const ShaderId id, const IndexT cIndex)
{
	return shaderPool->GetConstantBlockName(id, cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBindingsCount(const ShaderId id)
{
	return shaderPool->GetConstantBindingsCount(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
ShaderGetResourceTableLayout(const ShaderId id, const IndexT group)
{
	return shaderPool->GetResourceTableLayout(id, group);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
ShaderGetResourcePipeline(const ShaderId id)
{
	return shaderPool->GetResourcePipeline(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderGetConstantCount(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetConstantCount(id);
}

//------------------------------------------------------------------------------
/**
*/
ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantType(id, i);
}

//------------------------------------------------------------------------------
/**
*/
ShaderConstantType 
ShaderGetConstantType(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetConstantType(id, name);
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
ShaderGetConstantName(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
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
SizeT
ShaderGetConstantBufferCount(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetConstantBufferCount(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderGetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferSize(id, i);
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
ShaderGetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ShaderGetResourceSlot(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetResourceSlot(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
ShaderGetPrograms(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetPrograms(id);
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
ShaderProgramGetName(const CoreGraphics::ShaderProgramId id)
{
	return shaderPool->GetProgramName(id);
}

//------------------------------------------------------------------------------
/**
*/
ShaderFeature::Mask
ShaderFeatureFromString(const Util::String& str)
{
	return ShaderServer::Instance()->FeatureStringToMask(str);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderProgramId
ShaderGetProgram(const ShaderId id, const CoreGraphics::ShaderFeature::Mask program)
{
	return shaderPool->GetShaderProgram(id, program);
}

} // namespace CoreGraphics
