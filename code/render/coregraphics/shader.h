#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/shader.h
    
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

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "resources/resourceid.h"
#include "coregraphics/shaderfeature.h"
#include "coregraphics/buffer.h"
#include "util/variant.h"
#include "util/dictionary.h"

namespace AnyFX
{
class ShaderEffect;
}

namespace CoreGraphics
{

struct TextureId;
struct ShaderRWBufferId;
struct ResourceTableId;
struct ResourceTableLayoutId;
struct ResourcePipelineId;
struct ResourceTableSet;

RESOURCE_ID_TYPE(ShaderId);             
ID_24_8_24_8_NAMED_TYPE(ShaderProgramId, programId, programGeneration, shaderId, shaderGeneration, program, shader);     // 32 bits shader, 24 bits program, 8 bits type

ID_32_TYPE(DerivativeStateId);          // 32 bits derivative state (already created from an ordinary state)

struct ShaderCreateInfo
{
    Resources::ResourceName name;
    AnyFX::ShaderEffect* effect;
};

enum ShaderConstantType
{
    UnknownVariableType,
    IntVariableType,
    FloatVariableType,
    VectorVariableType,     // vec4
    Vector2VariableType,    // vec2
    MatrixVariableType,
    BoolVariableType,
    TextureVariableType,
    SamplerVariableType,
    ConstantBufferVariableType,
    ImageReadWriteVariableType,
    BufferReadWriteVariableType,
    TextureHandleType,      // just a uint
    ImageHandleType,        // just a uint
    SamplerHandleType       // just a uint
};


/// Get constant type as string
const Util::String ConstantTypeToString(const ShaderConstantType& type);

/// Create new shader
const ShaderId CreateShader(const ShaderCreateInfo& info);
/// Destroy shader
void DestroyShader(const ShaderId id);
/// Reload shader
void ReloadShader(const ShaderId id, const AnyFX::ShaderEffect* effect);

/// get shader by name
const ShaderId ShaderGet(const Resources::ResourceName& name);
/// Get shader feature mask from string
CoreGraphics::ShaderFeature::Mask ShaderFeatureMask(const Util::String& mask);

/// create resource table from shader
const ResourceTableId ShaderCreateResourceTable(const ShaderId id, const IndexT group, const uint overallocationSize = 1, const char* name = nullptr);
/// create resource table set from shader
ResourceTableSet ShaderCreateResourceTableSet(const ShaderId id, const IndexT group, const uint overallocationSize = 1, const char* name = nullptr);
/// Returns true if there is a resource table for the given group in the shader
const bool ShaderHasResourceTable(const ShaderId id, const IndexT group);
/// create constant buffer from shader using name (don't use too frequently)
const BufferId ShaderCreateConstantBuffer(const ShaderId id, const Util::StringAtom& name, BufferAccessMode mode = BufferAccessMode::HostCached);
/// create constant buffer from index
const BufferId ShaderCreateConstantBuffer(const ShaderId id, const IndexT cbIndex, BufferAccessMode mode = BufferAccessMode::HostCached);
/// create constant buffer from index
const BufferId ShaderCreateConstantBuffer(const ShaderId id, const IndexT group, const IndexT cbIndex, BufferAccessMode mode = BufferAccessMode::HostCached);
/// Calculate buffer index using binding mask and slot
const uint ShaderCalculateConstantBufferIndex(const uint64_t bindingMask, const IndexT slot);

/// Get name of shader
const Resources::ResourceName ShaderGetName(const ShaderId id);
/// get the number of constants from shader
const SizeT ShaderGetConstantCount(const ShaderId id);
/// get type of variable by index
const ShaderConstantType ShaderGetConstantType(const ShaderId id, const IndexT i);
/// get type of variable by index
const ShaderConstantType ShaderGetConstantType(const ShaderId id, const Util::StringAtom& name);
/// get name of variable by index
const Util::StringAtom ShaderGetConstantName(const ShaderId id, const IndexT i);
/// get constant buffer binding by name
const IndexT ShaderGetConstantBinding(const ShaderId id, const Util::StringAtom& name);
/// get constant buffer binding by index
const IndexT ShaderGetConstantBinding(const ShaderId id, const IndexT cIndex);
/// get name of constant buffer wherein constant with name resides
const Util::StringAtom ShaderGetConstantBlockName(const ShaderId id, const Util::StringAtom& name);
/// get name of constant buffer where in constant with index resides
const Util::StringAtom ShaderGetConstantBlockName(const ShaderId id, const IndexT cIndex);
/// get count of constant buffer bindings (for iteration)
const SizeT ShaderGetConstantBindingsCount(const ShaderId id);
/// get group to which constant is bound (the constant buffer which it resides in)
const IndexT ShaderGetConstantGroup(const ShaderId id, const Util::StringAtom& name);
/// get binding inside group of the constant buffer the constant lies in
const IndexT ShaderGetConstantSlot(const ShaderId id, const Util::StringAtom& name);

/// get resource table layout
CoreGraphics::ResourceTableLayoutId ShaderGetResourceTableLayout(const ShaderId id, const IndexT group);
/// get pipeline layout for shader
CoreGraphics::ResourcePipelineId ShaderGetResourcePipeline(const ShaderId id);

/// get the number of constant buffers from shader
const SizeT ShaderGetConstantBufferCount(const ShaderId id);
/// get size of constant buffer
const SizeT ShaderGetConstantBufferSize(const ShaderId id, const IndexT i);
/// get name of constant buffer
const Util::StringAtom ShaderGetConstantBufferName(const ShaderId id, const IndexT i);
/// get binding inside group of constant buffer
const IndexT ShaderGetConstantBufferResourceSlot(const ShaderId id, const IndexT i);
/// get group of constant buffer
const IndexT ShaderGetConstantBufferResourceGroup(const ShaderId id, const IndexT i);
/// Get mask of constant buffers
const uint64_t ShaderGetConstantBufferBindingMask(const ShaderId id, const IndexT group);
/// Get size of constant buffer at binding point
const uint64_t ShaderGetConstantBufferSize(const ShaderId id, const IndexT group, const IndexT i);

/// get slot of any shader resource
const IndexT ShaderGetResourceSlot(const ShaderId id, const Util::StringAtom& name);

/// get programs
const Util::Dictionary<ShaderFeature::Mask, ShaderProgramId>& ShaderGetPrograms(const ShaderId id);
/// get name of program
const Util::StringAtom ShaderProgramGetName(const ShaderProgramId id);

/// get shader program id from masks, this allows us to apply a shader program directly in the future
const CoreGraphics::ShaderProgramId ShaderGetProgram(const ShaderId id, const CoreGraphics::ShaderFeature::Mask mask);

/// Get shader program raytracing bits
union RayTracingBits
{
    struct 
    {
        bool hasGen : 1;
        bool hasCallable : 1;
        bool hasAnyHit : 1;
        bool hasClosestHit : 1;
        bool hasIntersect : 1;
        bool hasMiss : 1;
    } bitField;
    uint bits;
};
RayTracingBits ShaderProgramGetRaytracingBits(const ShaderProgramId id);

} // namespace CoreGraphics
