//------------------------------------------------------------------------------
//  shader.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
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
	ShaderId ret = shaderPool->CreateResource(info.name, nullptr, 0, "render_system", nullptr, nullptr, true);
	ret.resourceType = ShaderIdType;
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
const BufferId
ShaderCreateConstantBuffer(const ShaderId id, const Util::StringAtom& name, BufferAccessMode mode)
{
	return shaderPool->CreateConstantBuffer(id, name, mode);
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
ShaderCreateConstantBuffer(const ShaderId id, const IndexT cbIndex, BufferAccessMode mode)
{
	return shaderPool->CreateConstantBuffer(id, cbIndex, mode);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantCount(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetConstantCount(id);
}

//------------------------------------------------------------------------------
/**
*/
const ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantType(id, i);
}

//------------------------------------------------------------------------------
/**
*/
const ShaderConstantType
ShaderGetConstantType(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetConstantType(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantName(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
ShaderGetConstantBinding(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetConstantBinding(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantBinding(const ShaderId id, const IndexT cIndex)
{
	return shaderPool->GetConstantBinding(id, cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBlockName(const ShaderId id, const Util::StringAtom& name)
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
const IndexT 
ShaderGetConstantGroup(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetConstantGroup(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ShaderGetConstantSlot(const ShaderId id, const Util::StringAtom& name)
{
	return shaderPool->GetConstantSlot(id, name);
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
const SizeT
ShaderGetConstantBufferCount(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetConstantBufferCount(id);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
ShaderGetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferSize(id, i);
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
ShaderGetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
ShaderGetConstantBufferResourceSlot(const ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferResourceSlot(id, i);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
ShaderGetConstantBufferResourceGroup(const ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferResourceGroup(id, i);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
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
const Util::StringAtom
ShaderProgramGetName(const CoreGraphics::ShaderProgramId id)
{
	return shaderPool->GetProgramName(id);
}

//------------------------------------------------------------------------------
/**
*/
const ShaderFeature::Mask
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
