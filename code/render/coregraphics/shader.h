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

ID_32_24_8_TYPE(ShaderId);				// resource id, 24 rightmost bits are the actual shader id
ID_32_32_TYPE(ShaderStateId);			// shader + state
ID_32_32_TYPE(ShaderProgramId);			// shader + program
ID_32_TYPE(ShaderVariableId);			// variable id within state, must keep track of state since state id is 64 bit
ID_32_TYPE(ProgramId);

struct ShaderCreateInfo
{
	const Resources::ResourceName name;
};

/// create new shader
const ShaderId CreateShader(const ShaderCreateInfo& info);
/// destroy shader
void DestroyShader(const ShaderId id);

/// create new shader state from shader
const ShaderStateId CreateShaderState(const ShaderId id);
/// create a new shared shader state
const ShaderStateId CreateSharedShaderState(const ShaderId id);
/// destroy shader state
void DestroyShaderState(const ShaderStateId id);
/// apply shader state
void ApplyShaderState(const ShaderStateId id);

/// get the number of constants from shader
SizeT GetConstantCount(const CoreGraphics::ShaderId id);
/// get the number of constant buffers from shader
SizeT GetConstantBufferCount(const CoreGraphics::ShaderId id);
/// get the shader variable using name
ShaderVariableId GetShaderVariable(const ShaderStateId id, const Util::StringAtom& name);
/// get the shader variable using index
ShaderVariableId GetShaderVariable(const ShaderStateId id, const IndexT index);

/// bind shader
void BindShader(const ShaderId id, const CoreGraphics::ShaderFeature::Mask& program);
/// bind shader and program all in one package
void BindShaderProgram(const ShaderProgramId id);

class ShaderPool;
extern ShaderPool* shaderPool;

} // CoreGraphics
