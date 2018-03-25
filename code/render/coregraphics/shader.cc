//------------------------------------------------------------------------------
//  shader.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shader.h"
#include "shaderpool.h"
#include "shaderserver.h"
namespace CoreGraphics
{

ShaderPool* shaderPool;
//------------------------------------------------------------------------------
/**
*/
inline const ShaderId
CreateShader(const ShaderCreateInfo& info)
{
	ShaderId ret = shaderPool->CreateResource(info.name, "render_system", nullptr, nullptr, true);
	ret.allocType = ShaderIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void
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
inline const ShaderStateId
ShaderCreateState(const ShaderId id, const Util::Array<IndexT>& groups, bool requiresUniqueResourceTable)
{
	return shaderPool->CreateState(id, groups, requiresUniqueResourceTable);
}

//------------------------------------------------------------------------------
/**
*/
inline const ShaderStateId
ShaderCreateState(const Resources::ResourceName name, const Util::Array<IndexT>& groups, bool requiresUniqueResourceTable)
{
	return CoreGraphics::ShaderServer::Instance()->ShaderCreateState(name, groups, requiresUniqueResourceTable);
}

//------------------------------------------------------------------------------
/**
*/
inline const ShaderStateId
ShaderCreateSharedState(const ShaderId id, const Util::Array<IndexT>& groups)
{
	return shaderPool->CreateSharedState(id, groups);
}

//------------------------------------------------------------------------------
/**
*/
inline const ShaderStateId
ShaderCreateSharedState(const Resources::ResourceName name, const Util::Array<IndexT>& groups)
{
	return CoreGraphics::ShaderServer::Instance()->ShaderCreateSharedState(name, groups);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderDestroyState(const ShaderStateId id)
{
	shaderPool->DestroyState(id);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderStateApply(const ShaderStateId id)
{
	shaderPool->ApplyState(id);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderGetNumActiveStates(const ShaderId id)
{
	shaderPool->GetNumActiveStates(id);
}

//------------------------------------------------------------------------------
/**
*/
inline DerivativeStateId
CreateDerivativeState(const ShaderStateId id, const IndexT group)
{
	return shaderPool->CreateDerivativeState(id, group);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DestroyDerivativeState(const ShaderStateId id, const DerivativeStateId& deriv)
{
	shaderPool->DestroyDerivativeState(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DerivativeStateApply(const ShaderStateId id, const DerivativeStateId& deriv)
{
	shaderPool->DerivativeStateApply(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DerivativeStateCommit(const ShaderStateId id, const DerivativeStateId& deriv)
{
	shaderPool->DerivativeStateCommit(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DerivativeStateReset(const ShaderStateId id, const DerivativeStateId& deriv)
{
	shaderPool->DerivativeStateReset(id, deriv);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderGetConstantCount(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetConstantCount(id);
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderConstantType
ShaderGetConstantType(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantType(id, i);
}

//------------------------------------------------------------------------------
/**
*/
inline Util::StringAtom
ShaderGetConstantName(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
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
inline SizeT
ShaderGetConstantBufferCount(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetConstantBufferCount(id);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderGetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferSize(id, i);
}

//------------------------------------------------------------------------------
/**
*/
inline Util::StringAtom
ShaderGetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i)
{
	return shaderPool->GetConstantBufferName(id, i);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
ShaderGetPrograms(const CoreGraphics::ShaderId id)
{
	return shaderPool->GetPrograms(id);
}

//------------------------------------------------------------------------------
/**
*/
inline Util::StringAtom
ShaderProgramGetName(const CoreGraphics::ShaderProgramId id)
{
	return shaderPool->GetProgramName(id);
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderConstantId
ShaderStateGetConstant(const ShaderStateId id, const Util::StringAtom& name)
{
	return shaderPool->ShaderStateGetConstant(id, name);
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderConstantId
ShaderStateGetConstant(const ShaderStateId id, const IndexT index)
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
inline void
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
inline void
ShaderResourceSetRenderTexture(const ShaderConstantId var, const ShaderStateId state, const RenderTextureId texture)
{
	shaderPool->ShaderResourceSetRenderTexture(var, state, texture);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderResourceSetTexture(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::TextureId texture)
{
	shaderPool->ShaderResourceSetTexture(var, state, texture);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderResourceSetConstantBuffer(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::ConstantBufferId buffer)
{
	shaderPool->ShaderResourceSetConstantBuffer(var, state, buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::TextureId image)
{
	shaderPool->ShaderResourceSetReadWriteTexture(var, state, image);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex)
{
	shaderPool->ShaderResourceSetReadWriteTexture(var, state, tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
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
inline void
ShaderBind(const ShaderId id, const CoreGraphics::ShaderFeature::Mask& program)
{
	shaderPool->ShaderBind(id, program);
}

//------------------------------------------------------------------------------
/**
*/
inline const ShaderProgramId
ShaderGetProgram(const ShaderId id, const CoreGraphics::ShaderFeature::Mask& program)
{
	return shaderPool->GetShaderProgram(id, program);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderProgramBind(const ShaderProgramId id)
{
	shaderPool->ShaderBind(id);
}

} // namespace CoreGraphics
