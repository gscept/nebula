//------------------------------------------------------------------------------
// vkstreamshaderloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderpool.h"
#include "vkshader.h"
#include "effectfactory.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/shaderserver.h"

using namespace CoreGraphics;
using namespace IO;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderPool, 'VKSL', Resources::ResourceStreamPool);

//------------------------------------------------------------------------------
/**
*/

VkShaderPool::VkShaderPool() :
    shaderAlloc(0x00FFFFFF),
    activeShaderProgram(Ids::InvalidId64),
    activeMask(0) 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/

VkShaderPool::~VkShaderPool()
{

}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkShaderPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
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
        n_error("VkShaderPool::LoadFromStream(): failed to load shader '%s'!",
            this->GetName(id).Value());
        return ResourcePool::Failed;
    }

    VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<1>(id.resourceId);
    VkShaderRuntimeInfo& runtimeInfo = this->shaderAlloc.Get<2>(id.resourceId);

    this->shaderAlloc.Get<0>(id.resourceId) = effect;
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

    // setup shader variations
    const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
    for (uint i = 0; i < programs.size(); i++)
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

    // set active variation
    runtimeInfo.activeMask = runtimeInfo.programMap.KeyAtIndex(0);
    runtimeInfo.activeShaderProgram = runtimeInfo.programMap.ValueAtIndex(0);

#if __NEBULA_HTTP__
    //res->debugState = res->CreateState();
#endif
    return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceStreamPool::LoadStatus 
VkShaderPool::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
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
        return ResourcePool::Failed;
    }

    if (this->shaderAlloc.Get<0>(id.resourceId))
        delete this->shaderAlloc.Get<0>(id.resourceId);

    this->shaderAlloc.Get<0>(id.resourceId) = effect;
    VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<1>(id.resourceId);
    VkShaderRuntimeInfo& runtimeInfo = this->shaderAlloc.Get<2>(id.resourceId);

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

    return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::Unload(const Resources::ResourceId res)
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
VkShaderPool::GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
    VkShaderRuntimeInfo& runtime = this->shaderAlloc.Get<2>(shaderId.resourceId);
    IndexT i = runtime.programMap.FindIndex(mask);
    if (i == InvalidIndex)  return CoreGraphics::ShaderProgramId::Invalid();
    else                    return runtime.programMap.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableId 
VkShaderPool::CreateResourceTable(const CoreGraphics::ShaderId id, const IndexT group)
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(id.resourceId);
    IndexT idx = info.descriptorSetLayoutMap.FindIndex(group);
    if (idx == InvalidIndex) return CoreGraphics::ResourceTableId::Invalid();
    else
    {
        ResourceTableCreateInfo crInfo =
        {
            Util::Get<1>(info.descriptorSetLayouts[info.descriptorSetLayoutMap.ValueAtIndex(idx)])
        };
        return CoreGraphics::CreateResourceTable(crInfo);
    }   
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId 
VkShaderPool::CreateConstantBuffer(const CoreGraphics::ShaderId id, const Util::StringAtom& name, CoreGraphics::BufferAccessMode mode)
{
    AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVarblock(name.Value());
    if (var->alignedSize > 0)
    {
        BufferCreateInfo info;
        info.byteSize = var->alignedSize;
        info.name = name;
        info.mode = mode;
        info.usageFlags = CoreGraphics::ConstantBuffer;
        return CoreGraphics::CreateBuffer(info);
    }
    else
        return CoreGraphics::BufferId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
VkShaderPool::CreateConstantBuffer(const CoreGraphics::ShaderId id, const IndexT cbIndex, CoreGraphics::BufferAccessMode mode)
{
    AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVarblock(cbIndex);
    if (var->alignedSize > 0)
    {
        BufferCreateInfo info;
        info.byteSize = var->alignedSize;
        info.name = var->name.c_str();
        info.mode = mode;
        info.usageFlags = CoreGraphics::ConstantBuffer;
        return CoreGraphics::CreateBuffer(info);
    }
    else
        return CoreGraphics::BufferId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderPool::GetConstantBinding(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(id.resourceId);
    IndexT index = info.constantBindings.FindIndex(name.Value());
    if (index == InvalidIndex)  return { INT32_MAX }; // invalid binding
    else                        return info.constantBindings.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderPool::GetConstantBinding(const CoreGraphics::ShaderId id, const IndexT cIndex) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(id.resourceId);
    return info.constantBindings.ValueAtIndex(cIndex);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT VkShaderPool::GetConstantBindingsCount(const CoreGraphics::ShaderId id) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(id.resourceId);
    return info.constantBindings.Size();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
VkShaderPool::GetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group)
{
    return Util::Get<1>(this->shaderAlloc.Get<1>(id.resourceId).descriptorSetLayouts[group]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
VkShaderPool::GetResourcePipeline(const CoreGraphics::ShaderId id)
{
    return this->shaderAlloc.Get<1>(id.resourceId).pipelineLayout;
}

//------------------------------------------------------------------------------
/**
    Use direct resource ids, not the State, Shader or Variable type ids
*/
AnyFX::VkProgram*
VkShaderPool::GetProgram(const CoreGraphics::ShaderProgramId shaderProgramId)
{
    return this->shaderAlloc.Get<3>(shaderProgramId.shaderId).Get<1>(shaderProgramId.programId);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderPool::GetConstantCount(const CoreGraphics::ShaderId id) const
{
    return this->shaderAlloc.Get<0>(id.resourceId)->GetNumVariables();
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
VkShaderPool::GetConstantType(const CoreGraphics::ShaderId id, const IndexT i) const
{
    AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariable(i);
    switch (var->type)
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
VkShaderPool::GetConstantType(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariable(name.Value());
    switch (var->type)
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
VkShaderPool::GetConstantBlockName(const CoreGraphics::ShaderId id, const Util::StringAtom& name)
{
    AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariable(name.Value());
    return var->parentBlock->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderPool::GetConstantBlockName(const CoreGraphics::ShaderId id, const IndexT cIndex)
{
    AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariable(cIndex);
    return var->parentBlock->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderPool::GetConstantName(const CoreGraphics::ShaderId id, const IndexT i) const
{
    AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariable(i);
    return var->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderPool::GetConstantGroup(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const unsigned idx = this->shaderAlloc.Get<0>(id.resourceId)->FindVariable(name.Value());
    if (idx != UINT_MAX)
    {
        AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariableFromMap(idx);
        return var->parentBlock->set;
    }
    else
        return -1;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderPool::GetConstantSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const unsigned idx = this->shaderAlloc.Get<0>(id.resourceId)->FindVariable(name.Value());
    if (idx != UINT_MAX)
    {
        AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVariableFromMap(idx);
        return var->parentBlock->binding;
    }
    else
        return -1;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderPool::GetConstantBufferCount(const CoreGraphics::ShaderId id) const
{
    return this->shaderAlloc.Get<0>(id.resourceId)->GetNumVarblocks();
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderPool::GetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i) const
{
    AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVarblock(i);
    return var->byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderPool::GetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i) const
{
    AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVarblock(i);
    return var->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderPool::GetConstantBufferResourceSlot(const CoreGraphics::ShaderId id, const IndexT i) const
{
    AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVarblock(i);
    return var->binding;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
VkShaderPool::GetConstantBufferResourceGroup(const CoreGraphics::ShaderId id, const IndexT i) const
{
    AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.resourceId)->GetVarblock(i);
    return var->set;
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderPool::GetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
    const VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(id.resourceId);
    IndexT index = info.resourceIndexMap.FindIndex(name);
    if (index == InvalidIndex)  return index;
    else                        return info.resourceIndexMap.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
VkShaderPool::GetPrograms(const CoreGraphics::ShaderId id)
{
    return this->shaderAlloc.Get<2>(id.resourceId).programMap;
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
VkShaderPool::GetProgramName(CoreGraphics::ShaderProgramId id)
{
    return this->shaderAlloc.Get<3>(id.shaderId).Get<0>(id.programId).name;
}

} // namespace Vulkan
