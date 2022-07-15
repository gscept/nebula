#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/config.h
    
    Compile time configuration options for the CoreGraphics subsystem.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "core/rttimacros.h"

#define NEBULA_WHOLE_BUFFER_SIZE (-1)
namespace CoreGraphics
{
typedef uint ConstantBufferOffset;

union InputAssemblyKey
{
    struct
    {
        uint topo : 4;
        bool primRestart : 1;
    };
    byte key;

    void operator=(const InputAssemblyKey& rhs) { this->key = rhs.key; }
    bool operator==(const InputAssemblyKey& rhs) const { return this->key == rhs.key; }
    bool operator!=(const InputAssemblyKey& rhs) const { return this->key != rhs.key; }
    bool operator>(const InputAssemblyKey& rhs) const { return this->key > rhs.key; }
    bool operator<(const InputAssemblyKey& rhs) const { return this->key < rhs.key; }
};

enum IdType
{
    BufferIdType,
    TextureIdType,
    TextureViewIdType,
    VertexLayoutIdType,
    ShaderIdType,
    ShaderProgramIdType,
    ShaderStateIdType,
    ShaderInstanceIdType,
    ShaderConstantIdType,
    CommandBufferIdType,
    CommandBufferPoolIdType,
    MeshIdType,
    EventIdType,
    BarrierIdType,
    SemaphoreIdType,
    FenceIdType,
    WindowIdType,
    PassIdType,
    AnimResourceIdType,
    ResourceTableIdType,
    ResourceTableLayoutIdType,
    ResourcePipelineIdType,
    SamplerIdType,
    MaterialIdType,
    SubmissionContextIdType,
    ImageIdType
};

enum QueueType
{
    GraphicsQueueType,
    ComputeQueueType,
    TransferQueueType,
    SparseQueueType,

    NumQueueTypes,

    InvalidQueueType
};

enum QueryType
{
    OcclusionQueryType,
    StatisticsQueryType,
    TimestampsQueryType,
    NumQueryTypes
};

enum ShaderVisibility
{
    InvalidVisibility           = 0,
    VertexShaderVisibility      = 1 << 0,
    HullShaderVisibility        = 1 << 2,
    DomainShaderVisibility      = 1 << 3,
    GeometryShaderVisibility    = 1 << 4,
    PixelShaderVisibility       = 1 << 5,
    AllGraphicsVisibility       = VertexShaderVisibility | HullShaderVisibility | DomainShaderVisibility | GeometryShaderVisibility | PixelShaderVisibility,
    ComputeShaderVisibility     = 1 << 6,
    AllVisibility               = VertexShaderVisibility | HullShaderVisibility | DomainShaderVisibility | GeometryShaderVisibility | PixelShaderVisibility | ComputeShaderVisibility
};
__ImplementEnumBitOperators(CoreGraphics::ShaderVisibility);

enum class ImageAspect
{
    ColorBits = (1 << 0),
    DepthBits = (1 << 1),
    StencilBits = (1 << 2),
    MetaBits = (1 << 3),
    Plane0Bits = (1 << 4),
    Plane1Bits = (1 << 5),
    Plane2Bits = (1 << 6)
};
__ImplementEnumBitOperators(CoreGraphics::ImageAspect);
__ImplementEnumComparisonOperators(CoreGraphics::ImageAspect);

enum class ImageLayout
{
    Undefined,
    General,
    ColorRenderTexture,
    DepthStencilRenderTexture,
    DepthStencilRead,
    ShaderRead,
    TransferSource,
    TransferDestination,
    Preinitialized,
    Present
};

enum ShaderPipeline
{
    InvalidPipeline,
    GraphicsPipeline,
    ComputePipeline     // Compute pipeline is not the compute queue, it's just resources available for compute shaders
};

//------------------------------------------------------------------------------
/**
*/
inline ShaderVisibility
ShaderVisibilityFromString(const Util::String& str)
{
    Util::Array<Util::String> components = str.Tokenize("|");
    CoreGraphics::ShaderVisibility ret = InvalidVisibility;
    IndexT i;
    for (i = 0; i < components.Size(); i++)
    {
        const Util::String& component = components[i];
        if (component == "VS")      ret |= VertexShaderVisibility;
        else if (component == "HS") ret |= HullShaderVisibility;
        else if (component == "DS") ret |= DomainShaderVisibility;
        else if (component == "GS") ret |= GeometryShaderVisibility;
        else if (component == "PS") ret |= PixelShaderVisibility;
        else if (component == "CS") ret |= ComputeShaderVisibility;
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline QueueType
QueueTypeFromString(const Util::String& str)
{
    if (str == "Graphics")      return GraphicsQueueType;
    else if (str == "Compute")  return ComputeQueueType;
    else if (str == "Transfer") return TransferQueueType;
    else if (str == "Sparse")   return SparseQueueType;
    return GraphicsQueueType;
}

//------------------------------------------------------------------------------
/**
*/
inline const char*
QueueNameFromQueueType(const QueueType type)
{
    switch (type)
    {
    case GraphicsQueueType:
        return "Graphics";
    case ComputeQueueType:
        return "Compute";
    case TransferQueueType:
        return "Transfer";
    case SparseQueueType:
        return "Sparse";
    default:
        return "Graphics";
    }
}


enum class BarrierDomain
{
    Global,
    Pass
};

enum class PipelineStage
{
    InvalidStage,
    Top,                // Top of pipe
    Bottom,             // Bottom of pipe
    Indirect,           // Indirect dispatch/draw fetching stage
    Index,              // Index fetch (automatically vertex shader)
    Vertex,             // Vertex fetch stage (automatically vertex shader)
    UniformGraphics,    // Uniform read on graphics queue
    UniformCompute,     // Uniform read on compute queue
    InputAttachment,    // Input attachment read (automatically pixel shader)
    ReadOnlyAccess = InputAttachment, // All of the above enums are read-only
    VertexShaderRead,       
    VertexShaderWrite,
    HullShaderRead,
    HullShaderWrite,
    DomainShaderRead,
    DomainShaderWrite,
    GeometryShaderRead,
    GeometryShaderWrite,
    PixelShaderRead,
    PixelShaderWrite,
    GraphicsShadersRead,
    GraphicsShadersWrite,
    ComputeShaderRead,
    ComputeShaderWrite,
    AllShadersRead,
    AllShadersWrite,
    ColorRead,              // Color output read
    ColorWrite,             // Color output write
    DepthStencilRead,       // Depth-Stencil output read
    DepthStencilWrite,      // Depth-Stencil output write
    TransferRead,           // Memory transfering read
    TransferWrite,          // Memory transfering write
    HostRead,               // Host operations read
    HostWrite,              // Host operations write
    MemoryRead,             // Memory operations read
    MemoryWrite,            // Memory operations write
    ImageInitial,           // Special pipeline stage for initial images
    Present                 // Special pipeline stage for present images
};

__ImplementEnumBitOperators(PipelineStage);

//------------------------------------------------------------------------------
/**
*/
inline PipelineStage
PipelineStageFromString(const Util::String& str)
{
    if (str == "Top")                           return PipelineStage::Top;
    else if (str == "Bottom")                   return PipelineStage::Bottom;
    else if (str == "IndirectRead")             return PipelineStage::Indirect;
    else if (str == "IndexRead")                return PipelineStage::Index;
    else if (str == "VertexRead")               return PipelineStage::Vertex;
    else if (str == "UniformGraphicsRead")      return PipelineStage::UniformGraphics;
    else if (str == "UniformComputeRead")       return PipelineStage::UniformCompute;
    else if (str == "InputAttachmentRead")      return PipelineStage::InputAttachment;
    else if (str == "VertexShaderRead")         return PipelineStage::VertexShaderRead;
    else if (str == "VertexShaderWrite")        return PipelineStage::VertexShaderWrite;
    else if (str == "HullShaderRead")           return PipelineStage::HullShaderRead;
    else if (str == "HullShaderWrite")          return PipelineStage::HullShaderWrite;
    else if (str == "DomainShaderRead")         return PipelineStage::DomainShaderRead;
    else if (str == "DomainShaderWrite")        return PipelineStage::DomainShaderWrite;
    else if (str == "GeometryShaderRead")       return PipelineStage::GeometryShaderRead;
    else if (str == "GeometryShaderWrite")      return PipelineStage::GeometryShaderWrite;
    else if (str == "PixelShaderRead")          return PipelineStage::PixelShaderRead;
    else if (str == "PixelShaderWrite")         return PipelineStage::PixelShaderWrite;
    else if (str == "ComputeShaderRead")        return PipelineStage::ComputeShaderRead;
    else if (str == "ComputeShaderWrite")       return PipelineStage::ComputeShaderWrite;
    else if (str == "ColorAttachmentRead")      return PipelineStage::ColorRead;
    else if (str == "ColorAttachmentWrite")     return PipelineStage::ColorWrite;
    else if (str == "DepthAttachmentRead")      return PipelineStage::DepthStencilRead;
    else if (str == "DepthAttachmentWrite")     return PipelineStage::DepthStencilWrite;
    else if (str == "TransferRead")             return PipelineStage::TransferRead;
    else if (str == "TransferWrite")            return PipelineStage::TransferWrite;
    else if (str == "HostRead")                 return PipelineStage::HostRead;
    else if (str == "HostWrite")                return PipelineStage::HostWrite;
    else if (str == "MemoryRead")               return PipelineStage::MemoryRead;
    else if (str == "MemoryWrite")              return PipelineStage::MemoryWrite;
    else if (str == "Present")                  return PipelineStage::Present;
    else
    {
        n_error("Invalid pipeline stage '%s'\n", str.AsCharPtr());
        return PipelineStage::InvalidStage;
    }
}

} // namespace CoreGraphics


#define SHADER_POSTEFFECT_DEFAULT_FEATURE_MASK "Alt0"

#if !PUBLIC_BUILD
#define NEBULA_GRAPHICS_DEBUG 1
#define NEBULA_MARKER_BLUE Math::vec4(0.8f, 0.8f, 1.0f, 1.0f)
#define NEBULA_MARKER_RED Math::vec4(1.0f, 0.8f, 0.8f, 1.0f)
#define NEBULA_MARKER_GREEN Math::vec4(0.8f, 1.0f, 0.8f, 1.0f)
#define NEBULA_MARKER_DARK_GREEN Math::vec4(0.6f, 0.8f, 0.6f, 1.0f)
#define NEBULA_MARKER_DARK_DARK_GREEN Math::vec4(0.5f, 0.7f, 0.5f, 1.0f)
#define NEBULA_MARKER_PINK Math::vec4(1.0f, 0.8f, 0.9f, 1.0f)
#define NEBULA_MARKER_PURPLE Math::vec4(0.9f, 0.7f, 0.9f, 1.0f)
#define NEBULA_MARKER_ORANGE Math::vec4(1.0f, 0.9f, 0.8f, 1.0f)
#define NEBULA_MARKER_TURQOISE Math::vec4(0.8f, 0.9f, 1.0f, 1.0f)
#define NEBULA_MARKER_GRAY Math::vec4(0.9f, 0.9f, 0.9f, 1.0f)
#define NEBULA_MARKER_BLACK Math::vec4(0.001f)
#define NEBULA_MARKER_WHITE Math::vec4(1)

#define NEBULA_MARKER_COMPUTE NEBULA_MARKER_BLUE
#define NEBULA_MARKER_GRAPHICS NEBULA_MARKER_GREEN
#define NEBULA_MARKER_TRANSFER NEBULA_MARKER_RED

#endif

//------------------------------------------------------------------------------
#if __VULKAN__
    #define COREGRAPHICS_TRIANGLE_FRONT_FACE_CCW (1)
    // define the same descriptor set slots as we do in the shaders
    #define NEBULA_TICK_GROUP 0             // set per tick (once for all views) by the system
    #define NEBULA_FRAME_GROUP 1            // set per frame (once per view) by the system
    #define NEBULA_PASS_GROUP 2             // set per pass by the system
    #define NEBULA_BATCH_GROUP 3            // set per batch (material settings or system stuff)
    #define NEBULA_INSTANCE_GROUP 4         // set a batch-internal copy of some specific settings
    #define NEBULA_SYSTEM_GROUP 5           // set a batch-internal copy of some specific settings
    #define NEBULA_DYNAMIC_OFFSET_GROUP 6   // set once per shader and is offset for each instance
    #define NEBULA_NUM_GROUPS (NEBULA_DYNAMIC_OFFSET_GROUP + 1)

    #define MAX_INPUT_ATTACHMENTS 32

    #define SHADER_MODEL_5 (1)
    #ifdef _DEBUG
        #define NEBULA_VULKAN_DEBUG (1)
    #else
        #define NEBULA_VULKAN_DEBUG (0)
    #endif
    #define PROJECTION_HANDEDNESS_LH (0)
#if __X64__
    #define VK_DEVICE_SIZE_CONV(x) uint64_t(x)
#else
    #define VK_DEVICE_SIZE_CONV(x) uint32_t(x)
#endif
#endif
//------------------------------------------------------------------------------
