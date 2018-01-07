#pragma once
//------------------------------------------------------------------------------
/**
	A shader represents an entire shader resource, containing several stages and programs

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "resources/resourceid.h"
#include "coregraphics/shaderfeature.h"
namespace CoreGraphics
{

struct ConstantBufferId;
struct TextureId;
struct ShaderRWTextureId;
struct ShaderRWBufferId;

ID_32_24_8_TYPE(ShaderId);				// 32 bits container, 24 bits resource, 8 bits type
ID_32_24_8_TYPE(ShaderStateId);			// 32 bits state, 24 bits shader, 8 bits type
ID_32_24_8_TYPE(ShaderProgramId);		// 32 bits program, 24 bits shader, 8 bits type
ID_32_TYPE(ShaderVariableId);			// variable id within state, must keep track of state since state id is 64 bit
ID_32_TYPE(ProgramId);

ID_32_TYPE(DerivativeStateId);			// 32 bits derivative state (already created from an ordinary state)

struct ShaderCreateInfo
{
	const Resources::ResourceName name;
};

enum ShaderVariableType
{
	UnknownVariableType,
	IntVariableType,
	FloatVariableType,
	VectorVariableType,		// float4
	Vector2VariableType,	// float2
	MatrixVariableType,
	BoolVariableType,
	TextureVariableType,
	SamplerVariableType,
	ConstantBufferVariableType,
	ImageReadWriteVariableType,
	BufferReadWriteVariableType
};

/// create new shader
const ShaderId CreateShader(const ShaderCreateInfo& info);
/// destroy shader
void DestroyShader(const ShaderId id);

/// create new shader state from shader
const ShaderStateId ShaderCreateState(const ShaderId id, const Util::Array<IndexT>& groups, bool requiresUniqueResourceTable);
/// create new shader state from shader resource id
const ShaderStateId ShaderCreateState(const Resources::ResourceName name, const Util::Array<IndexT>& groups, bool requiresUniqueResourceTable);
/// create a new shared shader state
const ShaderStateId ShaderCreateSharedState(const ShaderId id, const Util::Array<IndexT>& groups);
/// create a new shared shader state
const ShaderStateId ShaderCreateSharedState(const Resources::ResourceName name, const Util::Array<IndexT>& groups);
/// destroy shader state
void ShaderDestroyState(const ShaderStateId id);
/// apply shader state
void ShaderStateApply(const ShaderStateId id);

/// create derivative
DerivativeStateId CreateDerivativeState(const ShaderStateId id, const IndexT group);
/// destroy derivative
void DestroyDerivativeState(const ShaderStateId id, const DerivativeStateId& deriv);
/// apply derivative state
void DerivativeStateApply(const ShaderStateId id, const DerivativeStateId& deriv);
/// commit derivative state
void DerivativeStateCommit(const ShaderStateId id, const DerivativeStateId& deriv);
/// reset derivative state
void DerivativeStateReset(const ShaderStateId id, const DerivativeStateId& deriv);

/// get the number of constants from shader
SizeT ShaderGetConstantCount(const CoreGraphics::ShaderId id);
/// get the number of constant buffers from shader
SizeT ShaderGetConstantBufferCount(const CoreGraphics::ShaderId id);
/// get the shader variable using name
ShaderVariableId ShaderStateGetVariable(const ShaderStateId id, const Util::StringAtom& name);
/// get the shader variable using index
ShaderVariableId ShaderStateGetVariable(const ShaderStateId id, const IndexT index);
/// set shader variable
template <class TYPE> void ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE& value);
/// set shader variable array
template <class TYPE> void ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE* value, uint32_t count);
/// set variable as texture
void ShaderVariableSetTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId texture);
/// set variable as texture
void ShaderVariableSetConstantBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ConstantBufferId buffer);
/// set variable as texture
void ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId image);
/// set variable as texture
void ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex);
/// set variable as texture
void ShaderVariableSetReadWriteBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf);

/// bind shader
void ShaderBind(const ShaderId id, const CoreGraphics::ShaderFeature::Mask& program);
/// bind shader and program all in one package
void ShaderProgramBind(const ShaderProgramId id);

class ShaderPool;
extern ShaderPool* shaderPool;

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE& value)
{
	shaderPool->ShaderVariableSet<TYPE>(var, state, value);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE* value, uint32_t count)
{
	shaderPool->ShaderVariableSetArray<TYPE>(var, state, value, count);
}

} // namespace CoreGraphics
