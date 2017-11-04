#pragma once
//------------------------------------------------------------------------------
/**
	The CoreGraphics header includes a collection of static variables for loaders 
	and other pointers useful for direct access within the CoreGraphcis subsystem,
	as well as short-hands for binding certain graphics resources.

	It also contains specific query and update functions for the different subsystems
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "base/gpuresourcebase.h"
namespace CoreGraphics
{
class ShaderPool;
class TexturePool;
class MemoryVertexBufferPool;
class MemoryIndexBufferPool;
class MeshPool;
class ModelPool;
class VertexSignaturePool;
class VertexComponent;

extern ShaderPool* shaderPool;
extern TexturePool* texturePool;
extern MemoryVertexBufferPool* vboPool;
extern MemoryIndexBufferPool* iboPool;
extern MeshPool* meshPool;
extern ModelPool* modelPool;
extern VertexSignaturePool* layoutPool;

typedef Ids::Id64 ShaderId;				// resource id, 24 rightmost bits are the actual shader id
typedef Ids::Id64 ShaderStateId;		// shader + state
typedef Ids::Id64 ShaderProgramId;		// shader + program
typedef Ids::Id32 ShaderVariableId;		// variable id within state, must keep track of state since state id is 64 bit
typedef Ids::Id32 ProgramId;

typedef Ids::Id64 VertexBufferId;
typedef Ids::Id64 IndexBufferId;

/// bind vertex layout (before mesh is bound)
void BindVertexLayout(const Resources::ResourceId);
/// bind mesh (vertex and optional index buffers)
void BindMesh(const Resources::ResourceId id);
/// select primitive group for previous BindMesh call
void BindPrimitiveGroup(const IndexT primgroup);
/// bind shader
void BindShader(const Resources::ResourceId id, const ShaderFeature::Mask& program);
/// bind vertex buffer resource individually
void BindVertexBuffer(const Resources::ResourceId id, const IndexT slot, const IndexT vertexOffset);
/// bind index buffer resource individually
void BindIndexBuffer(const Resources::ResourceId id);

/// create a new vertex buffer
Resources::ResourceId CreateVertexBuffer(
	Resources::ResourceName name, 
	Util::StringAtom tag, 
	Base::GpuResourceBase::Access access, 
	Base::GpuResourceBase::Usage usage, 
	SizeT numVerts, 
	const Util::Array<VertexComponent>& comps, 
	void* data, 
	SizeT dataSize);

} // namespace CoreGraphics

namespace Shaders
{
/// get number of shader variables from shader resource
int VariableCount(const CoreGraphics::ShaderId id);
/// get number of variable blocks
int VariableBlockCount(const CoreGraphics::ShaderId id);
/// get shader id from state id
CoreGraphics::ShaderId GetShaderId(const CoreGraphics::ShaderId id);
/// set int value
void SetInt(const CoreGraphics::ShaderStateId id, int value);
/// set int array values
void SetIntArray(const CoreGraphics::ShaderStateId id, const int* values, SizeT count);
/// set float value
void SetFloat(const CoreGraphics::ShaderStateId id, float value);
/// set float array values
void SetFloatArray(const CoreGraphics::ShaderStateId id, const float* values, SizeT count);
/// set vector value
void SetFloat2(const CoreGraphics::ShaderStateId id, const Math::float2& value);
/// set vector array values
void SetFloat2Array(const CoreGraphics::ShaderStateId id, const Math::float2* values, SizeT count);
/// set vector value
void SetFloat4(const CoreGraphics::ShaderStateId id, const Math::float4& value);
/// set vector array values
void SetFloat4Array(const CoreGraphics::ShaderStateId id, const Math::float4* values, SizeT count);
/// set matrix value
void SetMatrix(const CoreGraphics::ShaderStateId id, const Math::matrix44& value);
/// set matrix array values
void SetMatrixArray(const CoreGraphics::ShaderStateId id, const Math::matrix44* values, SizeT count);
/// set bool value
void SetBool(const CoreGraphics::ShaderStateId id, bool value);
/// set bool array values
void SetBoolArray(const CoreGraphics::ShaderStateId id, const bool* values, SizeT count);
/// set texture value
void SetTexture(const CoreGraphics::ShaderStateId id, Util::Array<VkWriteDescriptorSet>& writes, const Resources::ResourceId tex);
/// set constant buffer
void SetConstantBuffer(const CoreGraphics::ShaderStateId id, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ConstantBuffer>& buf);
/// set shader read-write image
void SetShaderReadWriteTexture(const CoreGraphics::ShaderStateId id, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex);
/// set shader read-write as texture
void SetShaderReadWriteTexture(const CoreGraphics::ShaderStateId id, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::Texture>& tex);
/// set shader read-write buffer
void SetShaderReadWriteBuffer(const CoreGraphics::ShaderStateId id, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);

/// returns true if shader variable has an offset into any uniform buffer for the shader state
const bool IsActive(const CoreGraphics::ShaderVariableId id);
}