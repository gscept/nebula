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
DestroyShader(const ShaderId& id)
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
	Create fake shader id, this should work since shaders are unique resources
*/
const ShaderId
ShaderGet(const ShaderStateId state)
{
	ShaderId ret;
	ret.allocId = state.shaderId;
	ret.allocType = state.shaderType;
	ret.poolId = -1;
	ret.poolIndex = shaderPool->GetUniqueId();
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
const ShaderStateId
ShaderCreateState(const ShaderId& id, const Util::Array<IndexT>& groups, bool requiresUniqueResourceTable)
{
	return shaderPool->CreateState(id, groups, requiresUniqueResourceTable);
}

//------------------------------------------------------------------------------
/**
*/
const ShaderStateId
ShaderCreateState(const Resources::ResourceName name, const Util::Array<IndexT>& groups, bool requiresUniqueResourceTable)
{
	return CoreGraphics::ShaderServer::Instance()->ShaderCreateState(name, groups, requiresUniqueResourceTable);
}

//------------------------------------------------------------------------------
/**
*/
const ShaderStateId
ShaderCreateSharedState(const ShaderId& id, const Util::Array<IndexT>& groups)
{
	return shaderPool->CreateSlicedState(id, groups);
}

//------------------------------------------------------------------------------
/**
*/
const ShaderStateId
ShaderCreateSharedState(const Resources::ResourceName name, const Util::Array<IndexT>& groups)
{
	return CoreGraphics::ShaderServer::Instance()->ShaderCreateSharedState(name, groups);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderDestroyState(const ShaderStateId& id)
{
	shaderPool->DestroyState(id);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderStateCommit(const ShaderStateId & id)
{
	shaderPool->CommitState(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderGetNumActiveStates(const ShaderId& id)
{
	return shaderPool->GetNumActiveStates(id);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
ShaderGetResourceTableLayout(const ShaderId& id, const IndexT group)
{
	return shaderPool->GetResourceTableLayout(id, group);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
ShaderGetResourcePipeline(const ShaderId& id)
{
	return shaderPool->GetResourcePipeline(id);
}

//------------------------------------------------------------------------------
/**
*/
DerivativeStateId
CreateDerivativeState(const ShaderStateId& id, const IndexT group)
{
	return shaderPool->CreateDerivativeState(id, group);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyDerivativeState(const ShaderStateId& id, const DerivativeStateId& deriv)
{
	shaderPool->DestroyDerivativeState(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
void
DerivativeStateApply(const ShaderStateId& id, const DerivativeStateId& deriv)
{
	shaderPool->DerivativeStateApply(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
void
DerivativeStateCommit(const ShaderStateId& id, const DerivativeStateId& deriv)
{
	shaderPool->DerivativeStateCommit(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
void
DerivativeStateReset(const ShaderStateId& id, const DerivativeStateId& deriv)
{
	shaderPool->DerivativeStateReset(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderGetConstantCount(const CoreGraphics::ShaderId& id)
{
	return shaderPool->GetConstantCount(id);
}

//------------------------------------------------------------------------------
/**
*/
ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId& id, const IndexT i)
{
	return shaderPool->GetConstantType(id, i);
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
ShaderGetConstantName(const CoreGraphics::ShaderId& id, const IndexT i)
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
ShaderGetConstantBufferCount(const CoreGraphics::ShaderId& id)
{
	return shaderPool->GetConstantBufferCount(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderGetConstantBufferSize(const CoreGraphics::ShaderId& id, const IndexT i)
{
	return shaderPool->GetConstantBufferSize(id, i);
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
ShaderGetConstantBufferName(const CoreGraphics::ShaderId& id, const IndexT i)
{
	return shaderPool->GetConstantBufferName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ShaderGetResourceSlot(const ShaderId& id, const Util::StringAtom& name)
{
	return shaderPool->GetResourceSlot(id, name);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
ShaderGetPrograms(const CoreGraphics::ShaderId& id)
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
ShaderConstantId
ShaderStateGetConstant(const ShaderStateId& id, const Util::StringAtom& name)
{
	return shaderPool->ShaderStateGetConstant(id, name);
}

//------------------------------------------------------------------------------
/**
*/
ShaderConstantId
ShaderStateGetConstant(const ShaderStateId& id, const IndexT index)
{
	return shaderPool->ShaderStateGetConstant(id, index);
}

//------------------------------------------------------------------------------
/**
*/
ShaderConstantType
ShaderConstantGetType(const ShaderConstantId var, const ShaderStateId state)
{
	return shaderPool->ShaderConstantGetType(var, state);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConstantSet(const ShaderConstantId var, const ShaderStateId state, const Util::Variant& value)
{
	switch (value.GetType())
	{
	case Util::Variant::Float:
		ShaderConstantSet(var, state, value.GetFloat());
		break;
	case Util::Variant::Int:
		ShaderConstantSet(var, state, value.GetInt());
		break;
	case Util::Variant::Bool:
		ShaderConstantSet(var, state, value.GetBool());
		break;
	case Util::Variant::Float4:
		ShaderConstantSet(var, state, value.GetFloat4());
		break;
	case Util::Variant::Float2:
		ShaderConstantSet(var, state, value.GetFloat2());
		break;
	case Util::Variant::Matrix44:
		ShaderConstantSet(var, state, value.GetMatrix44());
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderResourceSetRenderTexture(const ShaderConstantId var, const ShaderStateId state, const RenderTextureId texture)
{
	shaderPool->ShaderResourceSetRenderTexture(var, state, texture);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderResourceSetTexture(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::TextureId texture)
{
	shaderPool->ShaderResourceSetTexture(var, state, texture);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderResourceSetConstantBuffer(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::ConstantBufferId buffer)
{
	shaderPool->ShaderResourceSetConstantBuffer(var, state, buffer);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::TextureId image)
{
	shaderPool->ShaderResourceSetReadWriteTexture(var, state, image);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex)
{
	shaderPool->ShaderResourceSetReadWriteTexture(var, state, tex);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderResourceSetReadWriteBuffer(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf)
{
	shaderPool->ShaderResourceSetReadWriteBuffer(var, state, buf);
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
const ShaderProgramId
ShaderGetProgram(const ShaderId& id, const CoreGraphics::ShaderFeature::Mask& program)
{
	return shaderPool->GetShaderProgram(id, program);
}

} // namespace CoreGraphics
