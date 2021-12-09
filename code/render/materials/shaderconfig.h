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
    /// Nullptr constructor
    ShaderConfigVariant()
        : type(Type::Invalid)
        , mem(nullptr)
    {};
    /// Nullptr constructor
    ShaderConfigVariant(nullptr_t)
        : type(Type::Invalid)
        , mem(nullptr)
    {}

    enum class Type
    {
        Invalid
        , Handle
        , Float
        , Float2
        , Float4
        , Int
        , UInt
        , Bool
        , Float4x4
    };
    ShaderConfigVariant::Type type;
    void* mem;

    static ShaderConfigVariant::Type StringToType(const Util::String& str)
    {
        if ("handle" == str)           return Type::Handle;
        else if ("float" == str)       return Type::Float;
        else if ("vec2" == str)        return Type::Float2;
        else if ("vec4" == str)        return Type::Float4;
        else if ("color" == str)       return Type::Float4; // NOT A BUG!else if ("int" == str)         return Type::Int;
        else if ("int" == str)         return Type::Int;
        else if ("uint" == str)        return Type::UInt;
        else if ("bool" == str)        return Type::Bool;
        else if ("mat4" == str)        return Type::Float4x4;

        return Type::Invalid;
    }

    static uint32_t TypeToSize(const Type type)
    {
        switch (type)
        {
            case Type::Handle:
                return sizeof(uint64_t);
                break;
            case Type::Float:
                return sizeof(float);
                break;
            case Type::Float2:
                return sizeof(Math::vec2);
                break;
            case Type::Float4:
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
            case Type::Float4x4:
                return sizeof(Math::mat4);
                break;
        }
    }

    /// Get
    template <typename T> T& Get() { return *reinterpret_cast<T*>(mem); }
    /// Set
    template <typename T> void Set(const T& data) { memcpy(this->mem, &data, TypeToSize(this->size)); }
};

struct ShaderConfigTexture
{
    Util::String name;
    CoreGraphics::TextureId defaultValue;
    CoreGraphics::TextureType type;
    bool system : 1;

    IndexT slot;
};

struct ShaderConfigConstant
{
    Util::String name;
    ShaderConfigVariant def, min, max;
    /*
    Util::Variant defaultValue;
    Util::Variant min;
    Util::Variant max;
    Util::Variant::Type type;
    */
    bool system : 1;

    IndexT offset;
    IndexT slot;
    IndexT group;
};

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
    MaterialId CreateSurface();
    /// destroy surface
    void DestroySurface(MaterialId mat);

    /// create an instance of a surface
    MaterialInstanceId CreateSurfaceInstance(const MaterialId id);
    /// destroy instance of a surface
    void DestroySurfaceInstance(const MaterialInstanceId id);

    /// get constant index
    IndexT GetSurfaceConstantIndex(const MaterialId sur, const Util::StringAtom& name);
    /// get texture index
    IndexT GetSurfaceTextureIndex(const MaterialId sur, const Util::StringAtom& name);
    /// get surface constant instance index
    IndexT GetSurfaceConstantInstanceIndex(const MaterialInstanceId sur, const Util::StringAtom& name);

    /// get default value for constant
    const ShaderConfigVariant GetSurfaceConstantDefault(const MaterialId sur, IndexT idx);
    /// get default value for texture
    const CoreGraphics::TextureId GetSurfaceTextureDefault(const MaterialId sur, IndexT idx);

    /// set constant in surface (applies to all instances)
    void SetSurfaceConstant(const MaterialId sur, const IndexT idx, const ShaderConfigVariant& value);
    /// set texture in surface (applies to all instances)
    void SetSurfaceTexture(const MaterialId sur, const IndexT idx, const CoreGraphics::TextureId tex);

    /// set instance constant
    void SetSurfaceInstanceConstant(const MaterialInstanceId sur, const IndexT idx, const Util::Variant& value);

    /// get name
    const Util::String& GetName();
    /// get hash code 
    const uint32_t HashCode() const;

private:
    friend class ShaderConfigServer;
    friend class MaterialCache;
    friend bool ShaderConfigBeginBatch(ShaderConfig*, CoreGraphics::BatchGroup::Code);
    friend void MaterialApply(ShaderConfig*, const MaterialId);
    friend void MaterialInstanceApply(ShaderConfig*, const MaterialInstanceId);
    friend void ShaderConfigEndBatch(ShaderConfig*);

    /// apply type-specific material state
    bool BeginBatch(CoreGraphics::BatchGroup::Code batch);
    /// apply surface-level material state
    void ApplySurface(const MaterialId id);
    /// apply specific material instance, using the same batch as 
    void ApplyInstance(const MaterialInstanceId mat);
    /// end surface-level material state
    void EndSurface();
    /// end batch
    void EndBatch();

    Util::HashTable<CoreGraphics::BatchGroup::Code, IndexT> batchToIndexMap;

    Util::Array<CoreGraphics::ShaderProgramId> programs;
    Util::Dictionary<Util::StringAtom, ShaderConfigTexture> textures;
    Util::Dictionary<Util::StringAtom, ShaderConfigConstant> constants;
    Util::Array<Util::Dictionary<Util::StringAtom, ShaderConfigTexture>> texturesByBatch;
    Util::Array<Util::Dictionary<Util::StringAtom, ShaderConfigConstant>> constantsByBatch;
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
    struct SurfaceInstanceConstant
    {
        IndexT binding;
        void* mem;
    };

    struct SurfaceConstant
    {
        ShaderConfigVariant defaultValue;
        IndexT bufferIndex;
        bool instanceConstant : 1;
        IndexT binding;
        union
        {
            CoreGraphics::BufferId buffer;
            void*                  mem;
        };
        
        SurfaceConstant()
            : buffer(CoreGraphics::InvalidBufferId)
            //, mem(nullptr)
        {}
    };

    struct SurfaceTexture
    {
        CoreGraphics::TextureId defaultValue;
        IndexT slot;
    };

    enum SurfaceMembers
    {
        SurfaceTable,
        InstanceTable,
        SurfaceBuffers,
        InstanceBuffers,
        Textures,
        Constants,
        TextureMap,
        ConstantMap
    };

    /// this will cause somewhat bad cache coherency, since the states across all passes are stored tightly/
    /// however, between two passes, the memory is still likely to have been nuked
    Ids::IdAllocator<
        Util::FixedArray<CoreGraphics::ResourceTableId>,                                // surface level resource table, mapped batch -> table
        Util::FixedArray<CoreGraphics::ResourceTableId>,                                // instance level resource table, mapped batch -> table
        Util::FixedArray<Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId>>>,     // surface level constant buffers, mapped batch -> buffers
        Util::FixedArray<Util::Array<Util::Tuple<IndexT, void*, SizeT>>>,               // instance level instance buffers, mapped batch -> memory + size
        Util::FixedArray<Util::Array<SurfaceTexture>>,                                  // textures
        Util::FixedArray<Util::Array<SurfaceConstant>>,                                 // constants
        Util::Dictionary<Util::StringAtom, IndexT>,                                     // name to resource map
        Util::Dictionary<Util::StringAtom, IndexT>                                      // name to constant map
    > surfaceAllocator;


    enum SurfaceInstanceMembers
    {
        SurfaceInstanceConstants,
        SurfaceInstanceOffsets,
    };
    Ids::IdAllocator<
        Util::FixedArray<Util::FixedArray<SurfaceInstanceConstant>>, // copy of surface constants
        Util::FixedArray<uint>
    > surfaceInstanceAllocator;
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
