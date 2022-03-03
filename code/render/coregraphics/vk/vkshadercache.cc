//------------------------------------------------------------------------------
// vkshadercache.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshadercache.h"
#include "vkshader.h"
#include "effectfactory.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/shaderserver.h"

using namespace CoreGraphics;
using namespace IO;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderCache, 'VKSL', Resources::ResourceStreamCache);

//------------------------------------------------------------------------------
/**
*/

VkShaderCache::VkShaderCache() :
    shaderAlloc(0x00FFFFFF),
    activeShaderProgram(Ids::InvalidId64),
    activeMask(0) 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/

VkShaderCache::~VkShaderCache()
{

}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceCache::LoadStatus
VkShaderCache::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    n_assert(this->GetState(id) == Resources::Resource::Pending);

    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    // load effect from memory
    AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

    // catch any potential error coming from AnyFX
    if (!effect)
    {
        n_error("VkShaderCache::LoadFromStream(): failed to load shader '%s'!",
            this->GetName(id).Value());
        return ResourceCache::Failed;
    }

    VkReflectionInfo& reflectionInfo = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId);
    VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    VkShaderRuntimeInfo& runtimeInfo = this->shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId);

    setupInfo.id = ShaderIdentifier::FromName(this->GetName(id));
    setupInfo.name = this->GetName(id);
    setupInfo.dev = Vulkan::GetCurrentDevice();

    // the setup code is massive, so just let it be in VkShader...
    VkShaderSetup(
        setupInfo.dev,
        this->GetName(id),
        Vulkan::GetCurrentProperties(),
        effect,
        setupInfo.constantRangeLayout,
        setupInfo.immutableSamplers,
        setupInfo.descriptorSetLayouts,
        setupInfo.descriptorSetLayoutMap,
        setupInfo.pipelineLayout,
        setupInfo.resourceIndexMap,
        setupInfo.constantBindings
        );

    // setup variables
    const std::vector<AnyFX::VariableBase*> variables = effect->GetVariables();
    for (size_t i = 0; i < variables.size(); i++)
    {
        AnyFX::VariableBase* var = variables[i];
        VkReflectionInfo::Variable refl;
        refl.name = var->name.c_str();
        refl.blockBinding = -1;
        refl.blockSet = -1;
        refl.type = var->type;
        if (var->parentBlock)
        {
            refl.blockName = var->parentBlock->name.c_str();
            refl.blockBinding = var->parentBlock->binding;
            refl.blockSet = var->parentBlock->set;
        }

        reflectionInfo.variables.Append(refl);
        reflectionInfo.variablesByName.Add(refl.name, refl);
    }

    // setup varblocks (uniform buffers)
    const std::vector<AnyFX::VarblockBase*> varblocks = effect->GetVarblocks();
    for (size_t i = 0; i < varblocks.size(); i++)
    {
        AnyFX::VarblockBase* var = varblocks[i];
        VkReflectionInfo::UniformBuffer refl;
        refl.name = var->name.c_str();
        refl.binding = var->binding;
        refl.set = var->set;
        refl.byteSize = var->alignedSize;

        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
    }

    // setup shader variations
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    for (size_t i = 0; i < programs.size(); i++)
    {
        // get program object from shader subsystem
        VkShaderProgramAllocator& programAllocator = this->shaderAlloc.Get<Shader_ProgramAllocator>(id.resourceId);
        AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);

        // allocate new program object and set it up
        Ids::Id32 programId = programAllocator.Alloc();
        VkShaderProgramSetup(programId, this->GetName(id), program, setupInfo.pipelineLayout, this->shaderAlloc.Get<Shader_ProgramAllocator>(id.resourceId));

        // make an ID which is the shader id and program id
        ShaderProgramId shaderProgramId;
        shaderProgramId.programId = programId;
        shaderProgramId.shaderId = id.resourceId;
        shaderProgramId.shaderType = id.resourceType;
        runtimeInfo.programMap.Add(programAllocator.Get<0>(programId).mask, shaderProgramId);
    }

    // delete the AnyFX effect;
    delete effect;

    // set active variation
    runtimeInfo.activeMask = runtimeInfo.programMap.KeyAtIndex(0);
    runtimeInfo.activeShaderProgram = runtimeInfo.programMap.ValueAtIndex(0);

#if __NEBULA_HTTP__
    //res->debugState = res->CreateState();
#endif
    return ResourceCache::Success;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceStreamCache::LoadStatus 
VkShaderCache::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
{
    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    // load effect from memory
    AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

    // catch any potential error coming from AnyFX
    if (!effect)
    {
        n_error("VkStreamShaderLoader::ReloadFromStream(): failed to load shader '%s'!",
            this->GetName(id).Value());
        return ResourceCache::Failed;
    }

    VkReflectionInfo& reflectionInfo = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId);
    VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    VkShaderRuntimeInfo& runtimeInfo = this->shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId);

    reflectionInfo.uniformBuffers.Clear();
    reflectionInfo.uniformBuffersByName.Clear();
    reflectionInfo.variables.Clear();
    reflectionInfo.variablesByName.Clear();

    // setup variables
    const std::vector<AnyFX::VariableBase*> variables = effect->GetVariables();
    for (size_t i = 0; i < variables.size(); i++)
    {
        AnyFX::VariableBase* var = variables[i];
        VkReflectionInfo::Variable refl;
        refl.name = var->name.c_str();
        refl.blockBinding = var->binding;
        refl.blockSet = var->set;
        refl.type = var->type;
        if (var->parentBlock)
            refl.blockName = var->parentBlock->name.c_str();

        reflectionInfo.variables.Append(refl);
        reflectionInfo.variablesByName.Add(refl.name, refl);
    }

    // setup varblocks (uniform buffers)
    const std::vector<AnyFX::VarblockBase*> varblocks = effect->GetVarblocks();
    for (size_t i = 0; i < varblocks.size(); i++)
    {
        AnyFX::VarblockBase* var = varblocks[i];
        VkReflectionInfo::UniformBuffer refl;
        refl.name = var->name.c_str();
        refl.binding = var->binding;
        refl.set = var->set;
        refl.byteSize = var->alignedSize;

        reflectionInfo.uniformBuffers.Append(refl);
        reflectionInfo.uniformBuffersByName.Add(refl.name, refl);
    }

    // setup shader variations from existing programs
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    for (IndexT i = 0; i < programs.size(); i++)
    {
        // get program object from shader subsystem
        VkShaderProgramAllocator& programAllocator = this->shaderAlloc.Get<Shader_ProgramAllocator>(id.resourceId);
        AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);
        CoreGraphics::ShaderFeature::Mask mask = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(program->GetAnnotationString("Mask").c_str());

        const ShaderProgramId& shaderProgramId = runtimeInfo.programMap[mask];

        // allocate new program object and set it up
        VkShaderProgramSetup(shaderProgramId.programId, this->GetName(id), program, setupInfo.pipelineLayout, this->shaderAlloc.Get<Shader_ProgramAllocator>(id.resourceId));

        // trigger a reload in the graphics device
        CoreGraphics::ReloadShaderProgram(shaderProgramId);
    }

    return ResourceCache::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderCache::Unload(const Resources::ResourceId res)
{
    VkShaderSetupInfo& setup = this->shaderAlloc.Get<Shader_SetupInfo>(res.resourceId);
    VkShaderProgramAllocator& programs = this->shaderAlloc.Get<Shader_ProgramAllocator>(res.resourceId);
    VkShaderRuntimeInfo& runtime = this->shaderAlloc.Get<Shader_RuntimeInfo>(res.resourceId);
    VkShaderCleanup(setup.dev, setup.immutableSamplers, setup.descriptorSetLayouts, setup.uniformBufferMap, setup.pipelineLayout);

    for (IndexT i = 0; i < runtime.programMap.Size(); i++)
    {
        VkShaderProgramSetupInfo& progSetup = programs.Get<ShaderProgram_SetupInfo>(runtime.programMap.ValueAtIndex(i).programId);
        VkShaderProgramRuntimeInfo& progRuntime = programs.Get<ShaderProgram_RuntimeInfo>(runtime.programMap.ValueAtIndex(i).programId);
        VkShaderProgramDiscard(progSetup, progRuntime, progRuntime.pipeline);
    }
    runtime.programMap.Clear();

    this->states[res.poolId] = Resources::Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderProgramId
VkShaderCache::GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
    VkShaderRuntimeInfo& runtime = this->shaderAlloc.Get<Shader_RuntimeInfo>(shaderId.resourceId);
    IndexT i = runtime.programMap.FindIndex(mask);
    if (i == InvalidIndex)  return CoreGraphics::InvalidShaderProgramId;
    else                    return runtime.programMap.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableId 
VkShaderCache::CreateResourceTable(const CoreGraphics::ShaderId id, const IndexT group, const uint overallocationSize)
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    IndexT idx = info.descriptorSetLayoutMap.FindIndex(group);
    if (idx == InvalidIndex) return CoreGraphics::InvalidResourceTableId;
    else
    {
        ResourceTableCreateInfo crInfo =
        {
            Util::Get<1>(info.descriptorSetLayouts[info.descriptorSetLayoutMap.ValueAtIndex(idx)]),
            overallocationSize
        };
        return CoreGraphics::CreateResourceTable(crInfo);
    }   
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId 
VkShaderCache::CreateConstantBuffer(const CoreGraphics::ShaderId id, const Util::StringAtom& name, CoreGraphics::BufferAccessMode mode)
{
    const auto& uniformBuffers = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffersByName;
    IndexT i = uniformBuffers.FindIndex(name);
    if (i != InvalidIndex)
    {
        const VkReflectionInfo::UniformBuffer& buffer = uniformBuffers.ValueAtIndex(i);
        if (buffer.byteSize == 0)
            return CoreGraphics::InvalidBufferId;

        BufferCreateInfo info;
        info.byteSize = buffer.byteSize;
        info.name = name;
        info.mode = mode;
        info.usageFlags = CoreGraphics::ConstantBuffer;
        return CoreGraphics::CreateBuffer(info);
    }
    else
        return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
VkShaderCache::CreateConstantBuffer(const CoreGraphics::ShaderId id, const IndexT cbIndex, CoreGraphics::BufferAccessMode mode)
{
    const auto& uniformBuffers = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers;
    const VkReflectionInfo::UniformBuffer& buffer = uniformBuffers[cbIndex];
    if (buffer.byteSize > 0)
    {
        BufferCreateInfo info;
        info.byteSize = buffer.byteSize;
        info.name = buffer.name;
        info.mode = mode;
        info.usageFlags = CoreGraphics::ConstantBuffer;
        return CoreGraphics::CreateBuffer(info);
    }
    else
        return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderCache::GetConstantBinding(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    IndexT index = info.constantBindings.FindIndex(name.Value());
    if (index == InvalidIndex)  return { INT32_MAX }; // invalid binding
    else                        return info.constantBindings.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderCache::GetConstantBinding(const CoreGraphics::ShaderId id, const IndexT cIndex) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    return info.constantBindings.ValueAtIndex(cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderCache::GetConstantBindingsCount(const CoreGraphics::ShaderId id) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    return info.constantBindings.Size();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
VkShaderCache::GetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group)
{
    const VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    uint layout = setupInfo.descriptorSetLayoutMap[group];
    return Util::Get<1>(setupInfo.descriptorSetLayouts[layout]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
VkShaderCache::GetResourcePipeline(const CoreGraphics::ShaderId id)
{
    return this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId).pipelineLayout;
}

//------------------------------------------------------------------------------
/**
    Use direct resource ids, not the State, Shader or Variable type ids
*/
const VkProgramReflectionInfo&
VkShaderCache::GetProgram(const CoreGraphics::ShaderProgramId shaderProgramId)
{
    return this->shaderAlloc.Get<Shader_ProgramAllocator>(shaderProgramId.shaderId).Get<ShaderProgram_ReflectionInfo>(shaderProgramId.programId);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderCache::GetConstantCount(const CoreGraphics::ShaderId id) const
{
    return this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables.Size();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
VkShaderCache::GetConstantType(const CoreGraphics::ShaderId id, const IndexT i) const
{
    const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables[i];
    switch (var.type)
    {
    case AnyFX::Double:
    case AnyFX::Float:
        return FloatVariableType;
    case AnyFX::Short:
    case AnyFX::Integer:
    case AnyFX::UInteger:
        return IntVariableType;
    case AnyFX::Bool:
        return BoolVariableType;
    case AnyFX::Float3:
    case AnyFX::Float4:
    case AnyFX::Double3:
    case AnyFX::Double4:
    case AnyFX::Integer3:
    case AnyFX::Integer4:
    case AnyFX::UInteger3:
    case AnyFX::UInteger4:
    case AnyFX::Short3:
    case AnyFX::Short4:
    case AnyFX::Bool3:
    case AnyFX::Bool4:
        return VectorVariableType;
    case AnyFX::Float2:
    case AnyFX::Double2:
    case AnyFX::Integer2:
    case AnyFX::UInteger2:
    case AnyFX::Short2:
    case AnyFX::Bool2:
        return Vector2VariableType;
    case AnyFX::Matrix2x2:
    case AnyFX::Matrix2x3:
    case AnyFX::Matrix2x4:
    case AnyFX::Matrix3x2:
    case AnyFX::Matrix3x3:
    case AnyFX::Matrix3x4:
    case AnyFX::Matrix4x2:
    case AnyFX::Matrix4x3:
    case AnyFX::Matrix4x4:
        return MatrixVariableType;
        break;
    case AnyFX::Image1D:
    case AnyFX::Image1DArray:
    case AnyFX::Image2D:
    case AnyFX::Image2DArray:
    case AnyFX::Image2DMS:
    case AnyFX::Image2DMSArray:
    case AnyFX::Image3D:
    case AnyFX::ImageCube:
    case AnyFX::ImageCubeArray:
        return ImageReadWriteVariableType;
    case AnyFX::Sampler1D:
    case AnyFX::Sampler1DArray:
    case AnyFX::Sampler2D:
    case AnyFX::Sampler2DArray:
    case AnyFX::Sampler2DMS:
    case AnyFX::Sampler2DMSArray:
    case AnyFX::Sampler3D:
    case AnyFX::SamplerCube:
    case AnyFX::SamplerCubeArray:
        return SamplerVariableType;
    case AnyFX::Texture1D:
    case AnyFX::Texture1DArray:
    case AnyFX::Texture2D:
    case AnyFX::Texture2DArray:
    case AnyFX::Texture2DMS:
    case AnyFX::Texture2DMSArray:
    case AnyFX::Texture3D:
    case AnyFX::TextureCube:
    case AnyFX::TextureCubeArray:
        return TextureVariableType;
    case AnyFX::TextureHandle:
        return TextureVariableType;
    case AnyFX::ImageHandle:
        return ImageReadWriteVariableType;
        break;
    case AnyFX::SamplerHandle:
        return SamplerVariableType;
    default:
        return ConstantBufferVariableType;
    }
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
VkShaderCache::GetConstantType(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName[name];
    switch (var.type)
    {
    case AnyFX::Double:
    case AnyFX::Float:
        return FloatVariableType;
    case AnyFX::Short:
    case AnyFX::Integer:
    case AnyFX::UInteger:
        return IntVariableType;
    case AnyFX::Bool:
        return BoolVariableType;
    case AnyFX::Float3:
    case AnyFX::Float4:
    case AnyFX::Double3:
    case AnyFX::Double4:
    case AnyFX::Integer3:
    case AnyFX::Integer4:
    case AnyFX::UInteger3:
    case AnyFX::UInteger4:
    case AnyFX::Short3:
    case AnyFX::Short4:
    case AnyFX::Bool3:
    case AnyFX::Bool4:
        return VectorVariableType;
    case AnyFX::Float2:
    case AnyFX::Double2:
    case AnyFX::Integer2:
    case AnyFX::UInteger2:
    case AnyFX::Short2:
    case AnyFX::Bool2:
        return Vector2VariableType;
    case AnyFX::Matrix2x2:
    case AnyFX::Matrix2x3:
    case AnyFX::Matrix2x4:
    case AnyFX::Matrix3x2:
    case AnyFX::Matrix3x3:
    case AnyFX::Matrix3x4:
    case AnyFX::Matrix4x2:
    case AnyFX::Matrix4x3:
    case AnyFX::Matrix4x4:
        return MatrixVariableType;
        break;
    case AnyFX::Image1D:
    case AnyFX::Image1DArray:
    case AnyFX::Image2D:
    case AnyFX::Image2DArray:
    case AnyFX::Image2DMS:
    case AnyFX::Image2DMSArray:
    case AnyFX::Image3D:
    case AnyFX::ImageCube:
    case AnyFX::ImageCubeArray:
        return ImageReadWriteVariableType;
    case AnyFX::Sampler1D:
    case AnyFX::Sampler1DArray:
    case AnyFX::Sampler2D:
    case AnyFX::Sampler2DArray:
    case AnyFX::Sampler2DMS:
    case AnyFX::Sampler2DMSArray:
    case AnyFX::Sampler3D:
    case AnyFX::SamplerCube:
    case AnyFX::SamplerCubeArray:
        return SamplerVariableType;
    case AnyFX::Texture1D:
    case AnyFX::Texture1DArray:
    case AnyFX::Texture2D:
    case AnyFX::Texture2DArray:
    case AnyFX::Texture2DMS:
    case AnyFX::Texture2DMSArray:
    case AnyFX::Texture3D:
    case AnyFX::TextureCube:
    case AnyFX::TextureCubeArray:
        return TextureVariableType;
    case AnyFX::TextureHandle:
        return TextureHandleType;
    case AnyFX::ImageHandle:
        return ImageHandleType;
        break;
    case AnyFX::SamplerHandle:
        return SamplerHandleType;
    default:
        return ConstantBufferVariableType;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderCache::GetConstantBlockName(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName[name];
    return var.blockName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderCache::GetConstantBlockName(const CoreGraphics::ShaderId id, const IndexT cIndex)
{
    const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables[cIndex];
    return var.blockName;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderCache::GetConstantName(const CoreGraphics::ShaderId id, const IndexT i) const
{
    const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variables[i];
    return var.name;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderCache::GetConstantGroup(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    IndexT idx = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.FindIndex(name);
    if (idx != InvalidIndex)
    {
        const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.ValueAtIndex(idx);
        return var.blockSet;
    }
    else
        return -1;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderCache::GetConstantSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    IndexT idx = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.FindIndex(name);
    if (idx != InvalidIndex)
    {
        const VkReflectionInfo::Variable& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).variablesByName.ValueAtIndex(idx);
        return var.blockBinding;
    }
    else
        return -1;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderCache::GetConstantBufferCount(const CoreGraphics::ShaderId id) const
{
    return this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers.Size();
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderCache::GetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i) const
{
    const VkReflectionInfo::UniformBuffer& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderCache::GetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i) const
{
    const VkReflectionInfo::UniformBuffer& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.name;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderCache::GetConstantBufferResourceSlot(const CoreGraphics::ShaderId id, const IndexT i) const
{
    const VkReflectionInfo::UniformBuffer& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.binding;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderCache::GetConstantBufferResourceGroup(const CoreGraphics::ShaderId id, const IndexT i) const
{
    const VkReflectionInfo::UniformBuffer& var = this->shaderAlloc.Get<Shader_ReflectionInfo>(id.resourceId).uniformBuffers[i];
    return var.set;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderCache::GetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<Shader_SetupInfo>(id.resourceId);
    IndexT index = info.resourceIndexMap.FindIndex(name);
    if (index == InvalidIndex)  return index;
    else                        return info.resourceIndexMap.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
VkShaderCache::GetPrograms(const CoreGraphics::ShaderId id)
{
    return this->shaderAlloc.Get<Shader_RuntimeInfo>(id.resourceId).programMap;
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
VkShaderCache::GetProgramName(CoreGraphics::ShaderProgramId id)
{
    return this->shaderAlloc.Get<Shader_ProgramAllocator>(id.shaderId).Get<0>(id.programId).name;
}

} // namespace Vulkan
