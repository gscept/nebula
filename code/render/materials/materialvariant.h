#pragma once
//------------------------------------------------------------------------------
/**
    Material special version of variant

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
namespace Materials
{

struct MaterialVariant
{
    struct TextureHandleTuple;

    /// Nullptr constructor
    MaterialVariant()
        : type({ Type::Invalid })
        , mem(nullptr)
    {};

    /// Copy constructor
    MaterialVariant(const MaterialVariant& rhs)
    {
        this->SetType(rhs.type.type);
        this->mem = rhs.mem;
    }

    /// Assign operator
    void operator=(const MaterialVariant& rhs)
    {
        this->SetType(rhs.type.type);
        this->mem = rhs.mem;
    }

    /// Move constructor
    MaterialVariant(MaterialVariant&& rhs)
    {
        this->SetType(rhs.type.type);
        rhs.SetType(Type::Invalid);
        this->mem = rhs.mem;
        rhs.mem = nullptr;
    }

    /// Move operator
    void operator=(MaterialVariant&& rhs)
    {
        this->SetType(rhs.type.type);
        rhs.SetType(Type::Invalid);
        this->mem = rhs.mem;
        rhs.mem = nullptr;
    }

    /// Nullptr constructor
    MaterialVariant(std::nullptr_t)
        : type({ Type::Invalid })
        , mem(nullptr)
    {}

    /// Construct from float
    MaterialVariant(const TextureHandleTuple& tex)
    {
        this->SetType(Type::TextureHandle);
        this->Set(tex);
    };
    /// Construct from float
    MaterialVariant(const float f)
    {
        this->SetType(Type::Float);
        this->Set(f);
    };
    /// Construct from float2
    MaterialVariant(const Math::vec2 v2)
    {
        this->SetType(Type::Vec2);
        this->Set(v2);
    };
    /// Construct from float4
    MaterialVariant(const Math::vec4 v4)
    {
        this->SetType(Type::Vec4);
        this->Set(v4);
    };
    /// Construct from int
    MaterialVariant(const int i)
    {
        this->SetType(Type::Int);
        this->Set(i);
    };
    /// Construct from uint
    MaterialVariant(const uint ui)
    {
        this->SetType(Type::UInt);
        this->Set(ui);
    };
    /// Construct from bool
    MaterialVariant(const bool b)
    {
        this->SetType(Type::Bool);
        this->Set(b);
    };
    /// Construct from bool
    MaterialVariant(const Math::mat4 mat)
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
        MaterialVariant::Type type : 8;
        bool needsDeref : 8;
    } type;
    void* mem;
    SizeT size;

    static MaterialVariant::Type StringToType(const Util::String& str)
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
            default: break;
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
        memcpy(this->type.needsDeref ? this->mem : reinterpret_cast<void*>(&this->mem), &data, size);
    }

    /// Set
    template <typename T> void Set(const T& data, void* mem)
    {
        this->size = sizeof(T);
        this->mem = mem;
        memcpy(this->type.needsDeref ? this->mem : reinterpret_cast<void*>(&this->mem), &data, this->size);
    }

    /// Set texture
    template <> void Set(const TextureHandleTuple& data, void* mem)
    {
        this->size = sizeof(TextureHandleTuple);
        this->mem = mem;
        memcpy(this->type.needsDeref ? this->mem : reinterpret_cast<void*>(&this->mem), &data, this->size);
    }
};

} // namespace Materials
