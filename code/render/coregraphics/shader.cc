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
	return SizeT();
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ShaderGetConstantBufferCount(const CoreGraphics::ShaderId id)
{
	return SizeT();
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderVariableId
ShaderStateGetVariable(const ShaderStateId id, const Util::StringAtom& name)
{
	return shaderPool->ShaderStateGetVariable(id, name);
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderVariableId
ShaderStateGetVariable(const ShaderStateId id, const IndexT index)
{
	return shaderPool->ShaderStateGetVariable(id, index);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableSetTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId texture)
{
	shaderPool->ShaderVariableSetTexture(var, state, texture);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableSetConstantBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ConstantBufferId buffer)
{
	shaderPool->ShaderVariableSetConstantBuffer(var, state, buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId image)
{
	shaderPool->ShaderVariableSetReadWriteTexture(var, state, image);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex)
{
	shaderPool->ShaderVariableSetReadWriteTexture(var, state, tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableSetReadWriteBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf)
{
	shaderPool->ShaderVariableSetReadWriteBuffer(var, state, buf);
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
