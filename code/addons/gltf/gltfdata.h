#pragma once
//------------------------------------------------------------------------------
/**
    @file gltf/gltfdata.h

    GLTF container type, loosely adapted from fx-gltf
    https://github.com/jessey-git/fx-gltf

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "util/trivialarray.h"
#include "util/dictionary.h"
#include "math/matrix44.h"
#include "math/float4.h"
#include "math/quaternion.h"
#include "math/vector.h"
#include "util/string.h"
#include "util/blob.h"
#include "io/stream.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/primitivetopology.h"
namespace pjson
{
    class value_variant;
    class document;
}

namespace Gltf
{
struct GltfBase
{    
    // serialized extensions and extras
    Util::String extensions;
    Util::String extras;
    virtual bool IsEmpty() const
    {
        return false;
    }
};

struct Accessor : GltfBase
{
    //FIXME these should be mapped to nebula types directly
    enum class ComponentType : uint16_t
    {
        None = 0,
        Byte = 5120,
        UnsignedByte = 5121,
        Short = 5122,
        UnsignedShort = 5123,
        UnsignedInt = 5125,
        Float = 5126
    };

    enum class Type : uint8_t
    {
        None,
        Scalar,
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat3,
        Mat4
    };

    struct Sparse : GltfBase
    {
        struct Indices : GltfBase
        {
            uint32_t bufferView;
            uint32_t byteOffset;
            ComponentType componentType{ ComponentType::None };          
        };

        struct Values : GltfBase
        {
            uint32_t bufferView;
            uint32_t byteOffset;            
        };

        int32_t count{ 0 };
        Indices indices;
        Values values;
        
        bool IsEmpty() const
        {
            return count == 0;
        }
    };

    int32_t bufferView{ -1 };
    uint32_t byteOffset{ 0 };
    uint32_t count{ 0 };
    bool normalized{ false };

    ComponentType componentType{ ComponentType::None };
    Type type{ Type::None };

    Base::VertexComponentBase::Format format;
    Sparse sparse;

    Util::String name;
    Util::Array<float> max;
    Util::Array<float> min;    
};

struct Animation : GltfBase
{
    struct Channel : GltfBase
    {
        struct Target : GltfBase
        {
            int32_t node{ -1 };
            Util::String path;            
        };

        int32_t sampler{ -1 };
        Target target;        
    };

    struct Sampler : GltfBase
    {
        enum class Type
        {
            Linear,
            Step,
            CubicSpline
        };

        int32_t input{ -1 };
        int32_t output{ -1 };

        Type interpolation{ Sampler::Type::Linear };
        
    };

    Util::String name;
    Util::Array<Channel> channels;
    Util::Array<Sampler> samplers;
    
};

struct Buffer : GltfBase
{    
    Util::String name;
    Util::String uri;
    Util::Blob data;    
    Util::String mimeType;
    bool embedded;
    // loads buffer into data blob, uses either file or embedded buffer
    void Load(Util::String const & folder);
    // encodes data blob into either uri string or saves as external file
    void Save(Util::String const & folder);
};

struct BufferView : GltfBase
{
    enum class TargetType : uint16_t
    {
        None = 0,
        ArrayBuffer = 34962,
        ElementArrayBuffer = 34963
    };

    Util::String name;

    int32_t buffer{ -1 };
    uint32_t byteOffset;
    uint32_t byteLength;
    uint32_t byteStride;

    TargetType target{ TargetType::None };    
};

struct Camera : GltfBase
{
    enum class Type
    {
        None,
        Orthographic,
        Perspective
    };

    struct Orthographic : GltfBase
    {
        float xmag;
        float ymag;
        float zfar;
        float znear;
    };

    struct Perspective : GltfBase
    {
        float aspectRatio;
        float yfov;
        float zfar;
        float znear;
    };

    Util::String name;
    Type type{ Type::None };

    Orthographic orthographic;
    Perspective perspective;    
};

struct Image : GltfBase
{
    enum Type
    {
        Png,
        Jpg
    };

    int32_t bufferView{ -1 };

    Util::String name;
    Util::String uri;    
    bool embedded{ false };
    Util::Blob data;
    Type type;        
};

struct Material : GltfBase
{
    enum class AlphaMode : uint8_t
    {
        Opaque,
        Mask,
        Blend
    };

    struct Texture : GltfBase
    {
        int32_t index{ -1 };
        int32_t texCoord;        

        bool empty() const noexcept
        {
            return index == -1;
        }
    };

    struct NormalTexture : Texture
    {
        float scale{ 1.0f };
    };

    struct OcclusionTexture : Texture
    {
        float strength{ 1.0f };
    };

    struct PBRMetallicRoughness : GltfBase
    {
        Math::float4 baseColorFactor = { Math::float4(1.0f) };
        Texture baseColorTexture;

        float roughnessFactor{ 1.0f };
        float metallicFactor{ 1.0f };
        Texture metallicRoughnessTexture;        

        bool IsEmpty() const
        {
            return baseColorTexture.empty() && metallicRoughnessTexture.empty() && metallicFactor == 1.0f && roughnessFactor == 1.0f && baseColorFactor == Math::float4(1.0f);
        }
    };

    float alphaCutoff{ 0.5f };
    AlphaMode alphaMode{ AlphaMode::Opaque };

    bool doubleSided{ false };

    NormalTexture normalTexture;
    OcclusionTexture occlusionTexture;
    PBRMetallicRoughness pbrMetallicRoughness;

    Texture emissiveTexture;
    Math::vector emissiveFactor = { Math::vector(0.0f) };

    Util::String name;
};

struct Primitive : GltfBase
{
    //FIXME these should be mapped to native nebula types
    enum class Mode : uint8_t
    {
        Points = 0,
        Lines = 1,
        LineLoop = 2,
        LineStrip = 3,
        Triangles = 4,
        TriangleStrip = 5,
        TriangleFan = 6
    };
    //FIXME these should be mapped to native nebula types (and handled smarter...)
    enum class Attribute : uint8_t
    {
        Position = 0,
        Normal = 1,
        Tangent = 2,
        TexCoord0 = 3, TexCoord1 = 4, TexCoord2 = 5, TexCoord3 = 6, TexCoord4 = 7, TexCoord5 = 8, TexCoord6 = 9, TexCoord7 = 10,
        Color0 = 11, Color1 = 12, Color2 = 13, Color3 = 14, Color4 = 15, Color5 = 16, Color6 = 17, Color7 = 18,        
        Joints0	= 19, Joints1 = 20, Joints2 = 21, Joints3 = 22, Joints4 = 23, Joints5 = 24, Joints6 = 25, Joints7 = 26,
        Weights0 = 27, Weights1 = 28, Weights2 = 29, Weights3 = 30, Weights4 = 31, Weights5 = 32, Weights6 = 33, Weights7 = 34
    };
    int32_t indices{ -1 };
    int32_t material{ -1 };

    Mode mode{ Mode::Triangles };
    CoreGraphics::PrimitiveTopology::Code nebulaMode;


    // dictionary with attribute to accessor index mapping
    Util::Dictionary<Attribute,uint32_t> attributes;
    Util::Dictionary<Base::VertexComponentBase::SemanticName, uint32_t> nebulaAttributes;
    Util::Array<Util::Dictionary<Attribute, uint32_t>> targets;
};

struct Mesh : GltfBase
{
    Util::String name;

    Util::Array<float> weights;
    Util::Array<Primitive> primitives;
};

struct Node : GltfBase
{
    Util::String name;

    enum class Type : uint8_t
    {
        Transform,
        Camera,
        Mesh,
        Skin
    };
    Type type;
    int32_t camera{ -1 };
    int32_t mesh{ -1 };
    int32_t skin{ -1 };

    bool hasTRS{ false };
    Math::matrix44 matrix;
    Math::quaternion rotation;
    Math::vector scale{ Math::vector(1.0f) };
    Math::point translation;

    Util::Array<int32_t> children;
    Util::Array<float> weights;    
};

struct Sampler : GltfBase
{
    enum class MagFilter : uint16_t
    {
        None,
        Nearest = 9728,
        Linear = 9729
    };

    enum class MinFilter : uint16_t
    {
        None,
        Nearest = 9728,
        Linear = 9729,
        NearestMipMapNearest = 9984,
        LinearMipMapNearest = 9985,
        NearestMipMapLinear = 9986,
        LinearMipMapLinear = 9987
    };

    enum class WrappingMode : uint16_t
    {
        ClampToEdge = 33071,
        MirroredRepeat = 33648,
        Repeat = 10497
    };

    Util::String name;

    MagFilter magFilter{ MagFilter::None };
    MinFilter minFilter{ MinFilter::None };

    WrappingMode wrapS{ WrappingMode::Repeat };
    WrappingMode wrapT{ WrappingMode::Repeat };

    bool IsEmpty() const
    {
        return name.IsEmpty() && magFilter == MagFilter::None && minFilter == MinFilter::None && wrapS == WrappingMode::Repeat && wrapT == WrappingMode::Repeat && this->extensions == nullptr && this->extras == nullptr;
    }
};

struct Scene : GltfBase
{
    Util::String name;

    Util::Array<uint32_t> nodes;
};

struct Skin : GltfBase
{
    int32_t inverseBindMatrices{ -1 };
    int32_t skeleton{ -1 };

    Util::String name;
    Util::Array<uint32_t> joints;    
};

struct Texture : GltfBase
{
    Util::String name;

    int32_t sampler{ -1 };
    int32_t source{ -1 };    
};

struct Asset : GltfBase
{
    Util::String copyright;
    Util::String generator;
    Util::String version;
    Util::String minVersion;
};

struct Document : public GltfBase
{       
    Asset asset;
    Util::Array<Accessor> accessors;
    Util::Array<Animation> animations;
    Util::Array<Buffer> buffers;
    Util::Array<BufferView> bufferViews;
    Util::Array<Camera> cameras;
    Util::Array<Image> images;
    Util::Array<Material> materials;
    Util::Array<Mesh> meshes;
    Util::Array<Node> nodes;
    Util::Array<Sampler> samplers;
    Util::Array<Scene> scenes;
    Util::Array<Skin> skins;
    Util::Array<Texture> textures;

    int32_t scene{ -1 };

    Util::Array<Util::String> extensionsUsed;
    Util::Array<Util::String> extensionsRequired;        

    void SerializeText(const IO::URI & uri) const;
    void SerializeBinary(const IO::URI & uri) const;
    bool Deserialize(const IO::URI & uri);
    bool Deserialize(Ptr<IO::Stream> const & stream);
};

}
