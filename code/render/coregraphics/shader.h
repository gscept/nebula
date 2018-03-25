#pragma once
//------------------------------------------------------------------------------
/**
	A shader represents an entire shader resource, containing several stages and programs.

	ShaderId - A shader resource handle. It cannot be used for anything other than quering
	the shader file (usually suffixed with .fx) for reflection information.

	ShaderStateId - An instantiation of either all or a subset of the variables in that shader.
	The state contains the, well, state of all resources (constant buffers, textures, read/write buffers)
	as well as the constant values.

	ShaderProgramId - The shader will most likely have a set of shader permutations, and those permutations
	are called programs. The ShaderProgramId binds both the shader, and an associated permutation in one
	call.

	ShaderConstantId - The resources applied with the ShaderStateId are more atomically defined 
	as constants, and serves both as individual shader constants (variables in CPU time, constant in GPU time)
	as well as the resource binds to that state. They are bound to a inherently bound to a state, and such
	modifying a constant requires both the state and constant id for that state. 

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "resources/resourceid.h"
#include "coregraphics/shaderfeature.h"
#include "util/variant.h"

namespace CoreGraphics
{

struct ConstantBufferId;
struct TextureId;
struct ShaderRWTextureId;
struct ShaderRWBufferId;
struct RenderTextureId;

RESOURCE_ID_TYPE(ShaderId);				// 32 bits container, 24 bits resource, 8 bits type
ID_24_8_24_8_NAMED_TYPE(ShaderStateId, shaderId, shaderType, stateId, stateType);		// 32 bits shader, 24 bits state, 8 bits type
ID_24_8_24_8_NAMED_TYPE(ShaderProgramId, shaderId, shaderType, programId, programType);		// 32 bits shader, 24 bits program, 8 bits type
ID_32_TYPE(ShaderConstantId);			// variable id within state, must provide state when setting

ID_32_TYPE(DerivativeStateId);			// 32 bits derivative state (already created from an ordinary state)

struct ShaderCreateInfo
{
	const Resources::ResourceName name;
};

enum ShaderConstantType
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
/// get constant type as string
Util::String ConstantTypeToString(const ShaderConstantType& type);

/// create new shader
const ShaderId CreateShader(const ShaderCreateInfo& info);
/// destroy shader
void DestroyShader(const ShaderId id);

/// get shader by name
const ShaderId ShaderGet(const Resources::ResourceName& name);
/// get shader by state
const ShaderId ShaderGet(const ShaderStateId state);

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
/// get number of active states
SizeT ShaderGetNumActiveStates(const ShaderId id);

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
SizeT ShaderGetConstantCount(const ShaderId id);
/// get type of variable by index
ShaderConstantType ShaderGetConstantType(const ShaderId id, const IndexT i);
/// get name of variable by index
Util::StringAtom ShaderGetConstantName(const ShaderId id, const IndexT i);

/// get the number of constant buffers from shader
SizeT ShaderGetConstantBufferCount(const ShaderId id);
/// get size of constant buffer
SizeT ShaderGetConstantBufferSize(const ShaderId id, const IndexT i);
/// get name of constnat buffer
Util::StringAtom ShaderGetConstantBufferName(const ShaderId id, const IndexT i);

/// get programs
const Util::Dictionary<ShaderFeature::Mask, ShaderProgramId>& ShaderGetPrograms(const ShaderId id);
/// get name of program
Util::StringAtom ShaderProgramGetName(const ShaderProgramId id);

/// get the shader variable using name
ShaderConstantId ShaderStateGetConstant(const ShaderStateId id, const Util::StringAtom& name);
/// get the shader variable using index
ShaderConstantId ShaderStateGetConstant(const ShaderStateId id, const IndexT index);
/// get the type of constant in state (is identical to ShaderGetConstantType if you are using the shader directly)
ShaderConstantType ShaderConstantGetType(const ShaderConstantId var, const ShaderStateId state);

/// set shader variable
void ShaderConstantSet(const ShaderConstantId var, const ShaderStateId state, const Util::Variant& value);
/// set shader variable
template <class TYPE> void ShaderConstantSet(const ShaderConstantId var, const ShaderStateId state, const TYPE& value);
/// set shader variable array
template <class TYPE> void ShaderConstantSetArray(const ShaderConstantId var, const ShaderStateId state, const TYPE* value, uint32_t count);
/// set constant as render texture in texture sampler slot
void ShaderResourceSetRenderTexture(const ShaderConstantId var, const ShaderStateId state, const RenderTextureId texture);
/// set constant as texture
void ShaderResourceSetTexture(const ShaderConstantId var, const ShaderStateId state, const TextureId texture);
/// set constant as constant buffer
void ShaderResourceSetConstantBuffer(const ShaderConstantId var, const ShaderStateId state, const ConstantBufferId buffer);
/// set constant as texture in read-write slot
void ShaderResourceSetReadWriteTexture(const ShaderConstantId var, const ShaderStateId state, const TextureId image);
/// set constant as texture
void ShaderResourceSetReadWriteTexture(const ShaderConstantId var, const ShaderStateId state, const ShaderRWTextureId tex);
/// set constant as texture
void ShaderResourceSetReadWriteBuffer(const ShaderConstantId var, const ShaderStateId state, const ShaderRWBufferId buf);

/// convert shader feature from string to mask
ShaderFeature::Mask ShaderFeatureFromString(const Util::String& str);

/// bind shader
void ShaderBind(const ShaderId id, const ShaderFeature::Mask& program);
/// get shader program id from masks, this allows us to apply a shader program directly in the future
const ShaderProgramId ShaderGetProgram(const ShaderId id, const ShaderFeature::Mask& program);
/// bind shader and program all in one package
void ShaderProgramBind(const ShaderProgramId id);

class ShaderPool;
extern ShaderPool* shaderPool;

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const TYPE& value)
{
	shaderPool->ShaderConstantSet<TYPE>(var, state, value);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const TYPE* value, uint32_t count)
{
	shaderPool->ShaderConstantSetArray<TYPE>(var, state, value, count);
}

} // namespace CoreGraphics
