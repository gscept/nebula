#pragma once
//------------------------------------------------------------------------------
/**
    A material type declares the draw steps and associated shaders

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/hashtable.h"
#include "coregraphics/batchgroup.h"
#include "coregraphics/shader.h"
#include "memory/arenaallocator.h"
#include "util/variant.h"
#include "util/fixedarray.h"
#include "util/arraystack.h"
#include "material.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"

namespace Materials
{

struct ShaderConfigVariant
{
    struct TextureHandleTuple;

    /// Nullptr constructor
    ShaderConfigVariant()
        : type({ Type::Invalid })
        , mem(nullptr)
    {};

    /// Nullptr constructor
    ShaderConfigVariant(std::nullptr_t)
        : type({ Type::Invalid })
        , mem(nullptr)
    {}

    /// Construct from float
    ShaderConfigVariant(const TextureHandleTuple& tex)
    {
        this->SetType(Type::TextureHandle);
        this->Set(tex);
    };
    /// Construct from float
    ShaderConfigVariant(const float f)
    {
        this->SetType(Type::Float);
        this->Set(f);
    };
    /// Construct from float2
    ShaderConfigVariant(const Math::vec2 v2)
    {
        this->SetType(Type::Vec2);
        this->Set(v2);
    };
    /// Construct from float4
    ShaderConfigVariant(const Math::vec4 v4)
    {
        this->SetType(Type::Vec4);
        this->Set(v4);
    };
    /// Construct from int
    ShaderConfigVariant(const int i)
    {
        this->SetType(Type::Int);
        this->Set(i);
    };
    /// Construct from uint
    ShaderConfigVariant(const uint ui)
    {
        this->SetType(Type::UInt);
        this->Set(ui);
    };
    /// Construct from bool
    ShaderConfigVariant(const bool b)
    {
        this->SetType(Type::Bool);
        this->Set(b);
    };
    /// Construct from bool
    ShaderConfigVariant(const Math::mat4 mat)
    {
        this->SetType(Type::Mat4);
        this->Set(mat);
    };

    enum class Type : byte
    {
        Invalid
        , TextureHandle
        , Float
        , Vec2
        , Vec4
        , Int
        , UInt
        , Bool
        , Mat4
    };
    struct InternalType
    {
        ShaderConfigVariant::Type type : 8;
        bool needsDeref : 8;
    } type;
    void* mem;

    static ShaderConfigVariant::Type StringToType(const Util::String& str)
    {
        if ("textureHandle" == str)    return Type::TextureHandle;
        else if ("float" == str)       return Type::Float;
        else if ("vec2" == str)        return Type::Vec2;
        else if ("vec4" == str)        return Type::Vec4;
        else if ("color" == str)       return Type::Vec4; // NOT A BUG!else if ("int" == str)         return Type::Int;
        else if ("int" == str)         return Type::Int;
        else if ("uint" == str)        return Type::UInt;
        else if ("bool" == str)        return Type::Bool;
        else if ("mat4" == str)        return Type::Mat4;

        return Type::Invalid;
    }

    struct TextureHandleTuple
    {
        uint64 resource;
        uint32 handle;
    };

    static uint32_t TypeToSize(const InternalType type)
    {
        switch (type.type)
        {
            case Type::TextureHandle:
                return sizeof(TextureHandleTuple);
                break;
            case Type::Float:
                return sizeof(float);
                break;
            case Type::Vec2:
                return sizeof(Math::vec2);
                break;
            case Type::Vec4:
                return sizeof(Math::vec4);
                break;
            case Type::Int:
                return sizeof(int32);
                break;
            case Type::UInt:
                return sizeof(uint32);
                break;
            case Type::Bool:
                return sizeof(bool);
                break;
            case Type::Mat4:
                return sizeof(Math::mat4);
                break;
        }
        return 0xFFFFFFFF;
    }

    /// Set type
    void SetType(const Type& type)
    {
        this->type.type = type;
        switch (type)
        {
#if __X86__
            // If building on 32 bit, make sure that vec2 is also 
            case Type::Vec2:
#endif
            case Type::TextureHandle:
            case Type::Mat4:
            case Type::Vec4:
                this->type.needsDeref = true;
                break;
            default:
                this->type.needsDeref = false;
                break;
        }
    }

    /// Get type
    const Type GetType() const
    {
        return this->type.type;
    }

    /// Get pointer
    const void* Get() const
    {
        // If the data is stored in a pointer, and not the pointer value it self, we need to deref the pointer
        return this->type.needsDeref ? this->mem : reinterpret_cast<const void*>(&this->mem);
    }

    /// Get
    template <typename T> const T& Get() const 
    {
        return this->type.needsDeref ? *reinterpret_cast<T*>(this->mem) : reinterpret_cast<const T&>(this->mem);
    }
    /// Set
    template <typename T> void Set(const T& data)
    { 
        auto size = TypeToSize(this->type);
        switch (this->type.needsDeref)
        {
            case true:
                memcpy(this->mem, &data, size);
                return;
            case false:
                memcpy(reinterpret_cast<void*>(&this->mem), &data, size);
                return;
        }
    }
};

struct ShaderConfigTexture
{
    Util::String name;
    CoreGraphics::TextureId defaultValue;
    CoreGraphics::TextureType type;
    bool system : 1;
};

struct ShaderConfigBatchTexture
{
    IndexT slot;
};

struct ShaderConfigConstant
{
    Util::String name;
    ShaderConfigVariant def, min, max;
    bool system : 1;
};

struct ShaderConfigBatchConstant
{
    IndexT offset, slot, group;
};

typedef IndexT BatchIndex;


class ShaderConfig
{
public:

    /// constructor
    ShaderConfig();
    /// destructor
    ~ShaderConfig();

    /// setup after loading
    void Setup();

    /// create a surface from type
    MaterialId CreateMaterial();
    /// destroy surface
    void DestroyMaterial(MaterialId mat);

    /// create an instance of a surface
    MaterialInstanceId CreateMaterialInstance(const MaterialId id);
    /// destroy instance of a surface
    void DestroyMaterialInstance(const MaterialInstanceId id);

    /// get constant index
    IndexT GetMaterialConstantIndex(const Util::StringAtom& name);
    /// get texture index
    IndexT GetMaterialTextureIndex(const Util::StringAtom& name);

    /// get default value for constant
    const ShaderConfigVariant GetMaterialConstantDefault(const MaterialId sur, IndexT idx);
    /// get default value for texture
    const CoreGraphics::TextureId GetMaterialTextureDefault(const MaterialId sur, IndexT idx);

    /// set constant in surface (applies to all instances)
    void SetMaterialConstant(const MaterialId sur, const IndexT idx, const ShaderConfigVariant& value);
    /// set texture in surface (applies to all instances)
    void SetMaterialTexture(const MaterialId sur, const IndexT idx, const CoreGraphics::TextureId tex);

    /// Get batch index from surface config
    BatchIndex GetBatchIndex(const CoreGraphics::BatchGroup::Code batch);
    /// Get the size of the per-instance contant buffer
    SizeT GetInstanceBufferSize(const MaterialInstanceId sur, const BatchIndex batch);
    /// Allocate constant memory this frame
    CoreGraphics::ConstantBufferOffset AllocateInstanceConstants(const MaterialInstanceId sur, const BatchIndex batch);

    /// get name
    const Util::String& GetName();
    /// get hash code 
    const uint32_t HashCode() const;

    /// Bind shader for a batch and return the batch index
    IndexT BindShader(const CoreGraphics::CmdBufferId buf, CoreGraphics::BatchGroup::Code batch);
    /// Apply a material
    void ApplyMaterial(const CoreGraphics::CmdBufferId buf, IndexT index, const MaterialId id);
    /// Apply a material instance
    void ApplyMaterialInstance(const CoreGraphics::CmdBufferId buf, IndexT index, const MaterialInstanceId id);


private:
    friend class ShaderConfigServer;
    friend class MaterialCache;
    friend bool ShaderConfigBeginBatch(ShaderConfig*, CoreGraphics::BatchGroup::Code);
    friend void MaterialApply(ShaderConfig*, const MaterialId);
    friend void MaterialInstanceApply(ShaderConfig*, const MaterialInstanceId);
    friend void ShaderConfigEndBatch(ShaderConfig*);

    Util::HashTable<CoreGraphics::BatchGroup::Code, BatchIndex> batchToIndexMap;

    Util::Dictionary<Util::StringAtom, IndexT> textureLookup;
    Util::Dictionary<Util::StringAtom, IndexT> constantLookup;
    Util::Array<CoreGraphics::ShaderProgramId> programs;
    Util::Array<ShaderConfigTexture> textures;
    Util::Array<ShaderConfigConstant> constants;
    Util::Array<Util::Array<ShaderConfigBatchTexture>> texturesByBatch;
    Util::Array<Util::Array<ShaderConfigBatchConstant>> constantsByBatch;
    bool isVirtual;
    Util::String name;
    Util::String description;
    Util::String group;
    uint vertexType;

    IndexT uniqueId;
    static IndexT ShaderConfigUniqueIdCounter;

    CoreGraphics::BatchGroup::Code currentBatch;
    IndexT currentBatchIndex;

    // the reason whe have an instance type is because it doesn't need the default value
    struct MaterialInstanceConstant
    {
        IndexT binding;
    };

    struct MaterialConstant
    {
        ShaderConfigVariant defaultValue;
        IndexT bufferIndex;
        bool instanceConstant : 1;
        IndexT binding;
        CoreGraphics::BufferId buffer;
        
        MaterialConstant()
            : buffer(CoreGraphics::InvalidBufferId)
            //, mem(nullptr)
        {}
    };

    struct MaterialTexture
    {
        CoreGraphics::TextureId defaultValue;
        IndexT slot;
    };

    enum MaterialMembers
    {
        MaterialTable,
        InstanceTable,
        MaterialBuffers,
        InstanceBuffers,
        Textures,
        Constants,
    };

    /// this will cause somewhat bad cache coherency, since the states across all passes are stored tightly/
    /// however, between two passes, the memory is still likely to have been nuked
    Ids::IdAllocator<
        Util::FixedArray<CoreGraphics::ResourceTableId>,                                // surface level resource table, mapped batch -> table
        Util::FixedArray<CoreGraphics::ResourceTableId>,                                // instance level resource table, mapped batch -> table
        Util::FixedArray<Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId>>>,     // surface level constant buffers, mapped batch -> buffers
        Util::FixedArray<Util::Tuple<IndexT, SizeT>>,                                   // instance level instance buffer, mapped batch -> memory + size
        Util::FixedArray<Util::Array<MaterialTexture>>,                                 // textures
        Util::FixedArray<Util::Array<MaterialConstant>>                                 // constants
    > materialAllocator;

    enum MaterialInstanceMembers
    {
        MaterialInstanceOffsets
    };
    Ids::IdAllocator<
        uint
    > materialInstanceAllocator;
};


//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
ShaderConfig::GetName()
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
ShaderConfig::HashCode() const
{
    return (uint32_t)this->uniqueId;
}

} // namespace Materials
